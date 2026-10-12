#include <header.h>
#include "OpenCLFilter.h"
#include "utility.h"
#include <opencv2/xphoto.hpp>
#include <FileUtility.h>
#include <mutex>
#include <ParamInit.h>
#include <RegardsConfigParam.h>
#include <opencv2/core/ocl.hpp>
#include <opencv2/dnn_superres.hpp>

#include <wx/filename.h>
#include "OpenCLKernelBuilder.h"
#include <appcontext.h>
extern AppContext application_context;

using namespace Regards::OpenCL;
using namespace cv;

using namespace dnn;
using namespace dnn_superres;

#define OPENCV_METHOD


#define EDSR 0
#define ESPCN 1
#define FSRCNN 2
#define LapSRN 3

std::atomic<bool> isDnnUsed{ false };
std::mutex muDnnSuperResImpl;
int numTexture = -1;


int COpenCLFilter::numTexture = -1;

class ParameterReleaseGuard
{
public:

	explicit ParameterReleaseGuard(std::vector<COpenCLParameter*>& params)
		: params_(params)
	{}

	~ParameterReleaseGuard()
	{
		for (auto* p : params_)
		{
			if (!p->GetNoDelete())
				p->Release();
		}
	}

private:

	std::vector<COpenCLParameter*>& params_;
};

template<typename F>
void ExecuteSafe(F&& func)
{
	try
	{
		func();
	}
	catch (const cv::Exception& e)
	{
		LogError(e.what());
	}
}

template<typename F>
void ExecuteSafeOpenCL(cv::UMat& inputData, F&& func)
{
    try
    {
        const bool convert = inputData.channels() == 3;

        if (convert)
        {
            cv::UMat source;

            cv::cvtColor(
                inputData,
                source,
                cv::COLOR_BGR2BGRA);

            // Synchronise la queue OpenCL d'OpenCV avant que
            // notre propre queue OpenCL n'utilise le cl_mem.
            cv::ocl::finish();

            cv::UMat result = func(source);

            // Synchronise le kernel OpenCL avant de repasser
            // dans les opérations OpenCV.
            cv::ocl::finish();

            cv::cvtColor(
                result,
                inputData,
                cv::COLOR_BGRA2BGR);

            cv::ocl::finish();
        }
        else
        {
            cv::UMat result = func(inputData);

            cv::ocl::finish();

            result.copyTo(inputData);

        }
    }
    catch (const cv::Exception& e)
    {
        LogError(e.what());
    }
}

template<typename F>
void ExecuteSafeOpenCL3Channels(cv::UMat& inputData, F&& func)
{
	try
	{
		if (inputData.empty())
			return;

		const bool wasBGRA = inputData.channels() == 4;

		if (wasBGRA)
		{
			cv::UMat alpha;
			cv::UMat bgr;
			cv::UMat restoredAlpha;

			// 1. Isolation propre du canal Alpha (index 3) et conversion en BGR
			cv::extractChannel(inputData, alpha, 3);
			cv::cvtColor(inputData, bgr, cv::COLOR_BGRA2BGR);

			// Important : On synchronise la queue OpenCV avant d'exécuter le traitement utilisateur
			cv::ocl::finish();

			// 2. Exécution du traitement sur les 3 canaux (BGR)
			cv::UMat resultBGR = func(bgr);

			if (resultBGR.empty())
				return;

			// On s'assure que le kernel utilisateur a terminé ses calculs OpenCL
			cv::ocl::finish();

			// 3. Redimensionnement de l'Alpha si le traitement a modifié la taille de l'image
			if (alpha.size() == resultBGR.size())
			{
				restoredAlpha = alpha;
			}
			else
			{
				cv::resize(
					alpha,
					restoredAlpha,
					resultBGR.size(),
					0.0,
					0.0,
					cv::INTER_LINEAR);

				// Synchronisation après le resize OpenCL de l'Alpha
				cv::ocl::finish();
			}

			// 4. Re-reconstruction de l'image BGRA
			cv::UMat resultBGRA;
			cv::cvtColor(resultBGR, resultBGRA, cv::COLOR_BGR2BGRA);

			cv::ocl::finish();

			// Réinsertion sécurisée du canal Alpha
			cv::insertChannel(restoredAlpha, resultBGRA, 3);

			cv::ocl::finish();

			// Affectation par référence d'en-tête (évite les bugs de réallocation mémoire sur un ROI)
			inputData = resultBGRA;
		}
		else
		{
			// Traitement direct pour le 3 canaux (BGR)
			cv::UMat resultBGR = func(inputData);
			cv::ocl::finish();

			// Affectation sécurisée
			inputData = resultBGR;
		}
	}
	catch (const cv::Exception& e)
	{
		LogError(e.what());
	}
}


template<typename F>
cv::UMat ExecuteSafeOpenCLWithUMatOutput(cv::UMat& inputData, bool bgraOutput, F&& func)
{
	cv::UMat dest;
	try
	{
		const bool convert = inputData.channels() == 3;

		if (convert)
		{
			
			cv::UMat source;
			cv::cvtColor(inputData, source, cv::COLOR_BGR2BGRA);
			cv::UMat result = func(source);
			cv::ocl::finish();
			cv::cvtColor(result, dest, COLOR_BGRA2BGR);
			cv::ocl::finish();
		}
		else
		{
			dest = func(inputData);
			cv::ocl::finish();
		}

	}
	catch (const cv::Exception& e)
	{
		LogError(e.what());
	}
	return dest;
}

class CSuperSampling
{
public:
	CSuperSampling()
	{
	};

	~CSuperSampling()
	{
	};
	string GenerateModelPath(string modelName, int scale);
	bool TestIfMethodIsValid(int method, int scale);
	UMat upscaleImage(UMat img, int method, int scale);

private:
	DnnSuperResImpl sr;
	int oldscale = -1;
	int oldmethod = -1;
};

string CSuperSampling::GenerateModelPath(string modelName, int scale)
{

	wxFileName path = wxFileName(CFileUtility::GetResourcesFolderPath(), wxEmptyString);
	path.AppendDir("model");
	path.SetFullName(modelName + "_x" + to_string(scale) + ".pb");

	return path.GetFullPath().utf8_string();
}

bool CSuperSampling::TestIfMethodIsValid(int method, int scale)
{
	if (method == EDSR && (scale == 2 || scale == 3 || scale == 4))
	{
		return true;
	}
	if (method == ESPCN && (scale == 2 || scale == 3 || scale == 4))
	{
		return true;
	}
	if (method == FSRCNN && (scale == 2 || scale == 3 || scale == 4))
	{
		return true;
	}
	if (method == LapSRN && (scale == 2 || scale == 4 || scale == 8))
	{
		return true;
	}
	return false;
}

UMat CSuperSampling::upscaleImage(UMat img, int method, int scale)
{
	
	isDnnUsed = true;
	UMat outputImage;

	if (oldscale != scale || oldmethod != method)
	{
		try
		{
			switch (method)
			{
			case EDSR:
				{
					string algorithm = "edsr";
					sr.readModel(GenerateModelPath("EDSR", scale));
					sr.setModel(algorithm, scale);
				}
				break;

			case ESPCN:
				{
					string algorithm = "espcn";
					sr.readModel(GenerateModelPath("ESPCN", scale));
					sr.setModel(algorithm, scale);
				}
				break;
			case FSRCNN:
				{
					string algorithm = "fsrcnn";
					sr.readModel(GenerateModelPath("FSRCNN", scale));
					sr.setModel(algorithm, scale);
				}
				break;
			case LapSRN:
				{
					string algorithm = "lapsrn";
					sr.readModel(GenerateModelPath("LapSRN", scale));
					sr.setModel(algorithm, scale);
				}
				break;
			}

			sr.setPreferableTarget(DNN_TARGET_OPENCL);
			sr.upsample(img, outputImage);

			//muDnnSuperResImpl.unlock();
		}
		catch (Exception& e)
		{
			const char* err_msg = e.what();
			std::cout << "CSuperSampling::exception caught: " << err_msg << std::endl;
			std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
		}
	}
	else
	{
		try
		{
			sr.upsample(img, outputImage);
		}
		catch (Exception& e)
		{
			const char* err_msg = e.what();
			std::cout << "CSuperSampling::exception caught: " << err_msg << std::endl;
			std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
		}
	}

	oldscale = scale;
	oldmethod = method;
	isDnnUsed = false;
	return outputImage;
}


COpenCLFilter::COpenCLFilter(COpenCLContext* openCLContext)
	: openCLContext(openCLContext)
{
	bool useMemory = (ocl::Device::getDefault().type() == CL_DEVICE_TYPE_GPU) ? false : true;
	flag = useMemory ? CL_MEM_USE_HOST_PTR : CL_MEM_COPY_HOST_PTR;
	hq3d = nullptr;
    superSampling = std::make_unique<CSuperSampling>();
	resizer = std::make_unique<COpenCLAvirResizer>(openCLContext);
	
}

COpenCLFilter::~COpenCLFilter()
{
    openclMemTempMap.clear();
}

void COpenCLFilter::DetailEnhance(UMat& inputData, const double& sigma_s, const double& sigma_r)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
	{
		UMat dest;

		cv::detailEnhance(image, dest, sigma_s, sigma_r);

		return dest;
	});
}

void COpenCLFilter::EdgePreservingFilter(UMat& inputData, const int& flags, const double& sigma_s, const double& sigma_r)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			UMat dest;
			edgePreservingFilter(image, dest, flags, sigma_s, sigma_r);
			return dest;
		});
}

void COpenCLFilter::PencilSketch(UMat& inputData, const double& sigma_s, const double& sigma_r, const double& shade_factor)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{

			UMat img1;
			UMat dest;
			pencilSketch(image, img1, dest, sigma_s, sigma_r, shade_factor);
			return dest;
		});
}

void COpenCLFilter::Stylization(UMat& inputData, const double& sigma_s, const double& sigma_r)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
	{
		UMat dest;
		stylization(image, dest, sigma_s, sigma_r);
		return dest;
	});
}

void COpenCLFilter::BilateralEffect(UMat& inputData, const int& fSize, const int& sigmaX, const int& sigmaP)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
		UMat dest;
		bilateralFilter(image, dest, fSize, sigmaX, sigmaP, BORDER_DEFAULT);
		return dest;
		});
}


void COpenCLFilter::NlMeans(UMat& inputData, const int& h, const int& hColor, const int& templateWindowSize,
                            const int& searchWindowSize)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			UMat ycbcr;
			UMat yChannel;
			UMat yChannelOut;
			UMat dest;

			cvtColor(image, ycbcr, COLOR_BGR2YCrCb);

			// Extract the Y channel (UNE SEULE FOIS)
			extractChannel(ycbcr, yChannel, 0);

			fastNlMeansDenoising(yChannel, yChannelOut, h, templateWindowSize, searchWindowSize);

			// Merge back
			insertChannel(yChannelOut, ycbcr, 0);

			// convert back to RGB
			cvtColor(ycbcr, dest, COLOR_YCrCb2BGR);

			return dest;
		});
}


void COpenCLFilter::Bm3d(UMat& inputData, const float& fSigma)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			UMat ycbcr;
			UMat yChannel;
			UMat yChannelOut;
			UMat dest;

			cvtColor(image, ycbcr, COLOR_BGR2YUV);

			// Extract the Y channel
			extractChannel(ycbcr, yChannel, 0);

			xphoto::bm3dDenoising(yChannel, yChannelOut, fSigma);

			// Merge the the color planes back into an Lab image
			insertChannel(yChannelOut, ycbcr, 0);

			// convert back to RGB
			cvtColor(ycbcr, dest, COLOR_YUV2BGR);

			// Temporary Mat not reused, so release from memory.
			yChannel.release();
			ycbcr.release();
			yChannelOut.release();

			return dest;
		});
}



void COpenCLFilter::BrightnessAndContrastAuto(UMat& inputData, float clipHistPercent)
{
	// ExecuteSafeOpenCL3Channels va dépouiller l'Alpha en amont et passer 
	// une image 'image' STRICTEMENT en 3 canaux (BGR) à la fonction ci-dessous.
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			cv::UMat dest;
			int histSize = 256;
			float alpha, beta;
			double minGray = 0, maxGray = 0;

			// 1. L'image étant garantie en 3 canaux (BGR) grâce au helper,
			//    on convertit en YUV et on extrait le canal Y de luminance sans risque de crash.
			cv::UMat yuvFrame;
			cv::cvtColor(image, yuvFrame, COLOR_BGR2YUV);
			cv::ocl::finish();

			cv::UMat yChannel;
			cv::extractChannel(yuvFrame, yChannel, 0);
			cv::ocl::finish();

			// 2. Calcul des seuils min et max
			if (clipHistPercent == 0)
			{
				// Conserver la plage dynamique complète disponible
				minMaxLoc(yChannel, &minGray, &maxGray);
			}
			else
			{
				UMat h;
				std::vector<int> channels = { 0 }; // Analyse uniquement le canal 0 (Y)
				std::vector<int> hsize = { 256 };  // Quantification sur 256 niveaux
				std::vector<float> hranges = { 0, 256 };

				calcHist(std::vector<UMat>(1, yChannel), channels, noArray(), h, hsize, hranges);

				Mat hist;
				h.copyTo(hist);

				// Calcul de la distribution cumulative de l'histogramme
				std::vector<float> accumulator(histSize);
				accumulator[0] = hist.at<float>(0);
				for (int i = 1; i < histSize; i++)
				{
					accumulator[i] = accumulator[i - 1] + hist.at<float>(i);
				}

				// Localisation des points de coupure selon le pourcentage demandé
				float max = accumulator.back();
				clipHistPercent *= (max / 100.0f); // Conversion du pourcentage en valeur absolue
				clipHistPercent /= 2.0f;           // Répartition sur les deux extrémités (gauche et droite)

				// Coupure à gauche (min)
				minGray = 0;
				while (minGray < histSize && accumulator[minGray] < clipHistPercent)
					minGray++;

				// Coupure à droite (max)
				maxGray = histSize - 1;
				while (maxGray >= 0 && accumulator[maxGray] >= (max - clipHistPercent))
				{
					maxGray--;
					if (maxGray == 0)
						break;
				}
			}

			// 3. Calcul de la mise à l'échelle (Alpha) et du décalage (Beta)
			float inputRange = maxGray - minGray;
			if (inputRange <= 0) inputRange = 1.0f; // Sécurité anti-division par zéro

			alpha = (histSize - 1) / inputRange; // Alpha étend la plage actuelle vers la plage 0-255
			beta = -minGray * alpha;             // Beta décale la plage pour que minGray commence à 0

			// 4. Application de la transformation linéaire (sur l'image BGR à 3 canaux)
			convertScaleAbs(image, dest, alpha, beta);
			cv::ocl::finish();

			// On retourne l'UMat à 3 canaux (BGR). 
			// C'est ExecuteSafeOpenCL3Channels qui se chargera de lui recoller l'Alpha d'origine.
			return dest;
		});
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void COpenCLFilter::Fusion(UMat& inputData, const UMat& secondPictureData, const float& pourcentage)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			UMat dst;
			float beta = (1.0 - pourcentage);
			addWeighted(image, pourcentage, secondPictureData, beta, 0.0, dst);
			return dst;
		});
}


void COpenCLFilter::SharpenMasking(const float& sharpness, UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			cv::UMat dest;
			UMat cvDestBgra;
			double sigma = 1;
			cv::GaussianBlur(image, cvDestBgra, Size(), sigma, sigma);

			double poids = sharpness; // Ajustez cette valeur pour augmenter/diminuer la netteté
			double alpha = 1.0 + poids;
			double beta = -poids;

			cv::addWeighted(image, alpha, cvDestBgra, beta, 0, dest);
			return dest;
		});

	
}

void COpenCLFilter::PhotoFiltre(const CRgbaquad& clValue, const int& intensity, UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			float coeff = static_cast<float>(intensity) / 100.0f;
			float diff = 1.0f - coeff;
			UMat out;
			UMat out_one;
			out_one = image.mul(diff);

			auto color = Scalar(clValue.GetBlue(), clValue.GetGreen(), clValue.GetRed());
			Scalar out_two = color * coeff;

			add(out_one, out_two, out);

			return out;
		});
}

void COpenCLFilter::RGBFilter(const int& red, const int& green, const int& blue, UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			UMat out;
			auto color = Scalar(blue, green, red);
			add(image, color, out);
			return out;
		});
}

void COpenCLFilter::FiltreMosaic(UMat& inputData, const int& tailleBloc)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{

			// 3. Calculer les dimensions de la version réduite
			int largeurReduite = std::max(1, image.cols / tailleBloc);
			int hauteurReduite = std::max(1, image.rows / tailleBloc);

			cv::UMat petiteImage;
			cv::UMat mosaique;

			// 4. Étape de réduction (downscaling)
			// On utilise INTER_LINEAR pour faire une moyenne propre des blocs de pixels
			cv::resize(image, petiteImage, cv::Size(largeurReduite, hauteurReduite), 0, 0, cv::INTER_LINEAR);

			// 5. Étape d'agrandissement (upscaling) à la taille d'origine
			// ATTENTION : INTER_NEAREST est crucial ici pour garder l'effet de gros blocs nets
			cv::resize(petiteImage, mosaique, image.size(), 0, 0, cv::INTER_NEAREST);

			return mosaique;

		});
}

void COpenCLFilter::Blur(const int& radius, UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			cv::UMat dest;
			blur(image, dest, Size(radius, radius));
			return dest;
		});
}

void COpenCLFilter::GaussianBlur(const int& radius, const int& boxSize, UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			cv::UMat dest;
			cv::GaussianBlur(image, dest, Size(boxSize, boxSize), radius);
			return dest;
		});
}

void COpenCLFilter::MotionBlurCompute(const vector<double>& kernelMotion, const vector<wxPoint>& offsets,
                                      const int& size,
                                      UMat& inputData)
{
	
	ExecuteSafeOpenCL(inputData,
		[&](cv::UMat& image)
		{
			auto clBuffer = static_cast<cl_mem>(image.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			vector<float> kernel;
			for (auto i = 0; i < kernelMotion.size(); i++)
				kernel.push_back(kernelMotion[i]);

			vector<int> offsetsMotion;
			for (auto i = 0, j = 0; i < offsets.size(); i++, j += 2)
			{
				offsetsMotion.push_back(offsets[i].x);
				offsetsMotion.push_back(offsets[i].y);
			}

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows)
				.FloatArray("kernelMotion", openCLContext->GetContext(), kernel.data(), size, flag)
				.IntArray("offsets", openCLContext->GetContext(), offsetsMotion.data(), size * 2, flag)
				.Int("kernelSize", size);


			auto params = builder.GetParameters();

			cv::UMat dest = ExecuteOpenCLCode(
				"IDR_OPENCL_MOTIONBLUR",
				"MotionBlur",
				params,
				inputData.cols,
				inputData.rows);

			return dest;
		});

}

void COpenCLFilter::Emboss(UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			// Construct kernel (all entries initialized to 0)
			Mat kernel(3, 3, CV_32F, Scalar(0));
			kernel.at<float>(0, 0) = -1.0;
			kernel.at<float>(2, 2) = 1.0;
			UMat dest;
			filter2D(image, dest, image.depth(), kernel);
			return dest;
		});
}

void COpenCLFilter::Sharpen(UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			// Construct kernel (all entries initialized to 0)
			// Construct kernel (all entries initialized to 0)
			Mat kernel(3, 3, CV_32F, Scalar(0));
			// assigns kernel values
			kernel.at<float>(1, 1) = 5.0;
			kernel.at<float>(0, 1) = -1.0;
			kernel.at<float>(2, 1) = -1.0;
			kernel.at<float>(1, 0) = -1.0;
			kernel.at<float>(1, 2) = -1.0;
			UMat dest;
			filter2D(image, dest, image.depth(), kernel);
			return dest;
		});
}

void COpenCLFilter::SharpenStrong(UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{

			Mat kernel(3, 3, CV_32F, Scalar(0));
			kernel.at<float>(0, 0) = -1.0;
			kernel.at<float>(0, 1) = -1.0;
			kernel.at<float>(0, 2) = -1.0;
			kernel.at<float>(1, 0) = -1.0;
			kernel.at<float>(1, 1) = 9.0;
			kernel.at<float>(1, 2) = -1.0;
			kernel.at<float>(2, 0) = -1.0;
			kernel.at<float>(2, 1) = -1.0;
			kernel.at<float>(2, 2) = -1.0;
			UMat dest;
			filter2D(image, dest, image.depth(), kernel);
			return dest;
		});
}

void COpenCLFilter::Edge(UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			UMat dest;
			cvtColor(image, dest, COLOR_BGR2GRAY);

			Mat img_blur;
			cv::GaussianBlur(dest, img_blur, Size(3, 3), 0, 0);
			UMat edges;
			Canny(img_blur, edges, 100, 200, 3, false);

			cvtColor(edges, dest, COLOR_GRAY2BGR);
			return dest;
		});
}

void COpenCLFilter::FiltreConvolution(const wxString& programName, const wxString& functionName, UMat& inputData)
{

	ExecuteSafeOpenCL(inputData,
		[&](cv::UMat& cvDestBgra)
		{

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows);

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode(
				programName, functionName,
				params,
				inputData.cols,
				inputData.rows);

			});

}

void COpenCLFilter::ErodeDilate(const wxString& functionName, UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			UMat dest;
			if (functionName == "Erode")
				erode(image, dest, Mat());
			else if (functionName == "Dilate")
				dilate(image, dest, Mat());
			return dest;
		});
}

void COpenCLFilter::Posterize(const float& level, const float& gamma, UMat& inputData)
{
	ExecuteSafeOpenCL(inputData,
		[&](cv::UMat& cvDestBgra)
		{

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows)
				.Int("level", level);

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode(
				"IDR_OPENCL_COLOR", "Posterisation",
				params,
				inputData.cols,
				inputData.rows);

		});
}


void COpenCLFilter::LensDistortion(const float& strength, UMat& inputData)
{
	ExecuteSafeOpenCL(inputData,
		[&](cv::UMat& cvDestBgra)
		{

			double _strength = static_cast<double>(strength) / 100;
			double correctionRadius = sqrt(pow(inputData.rows, 2) + pow(inputData.cols, 2)) / _strength;

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows)
				.Float("correctionRadius", correctionRadius);

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode(
				"IDR_OPENCL_DISTORTION", "Distortion",
				params,
				inputData.cols,
				inputData.rows);

		});
}


void COpenCLFilter::Solarize(const long& threshold, UMat& inputData)
{
	
	ExecuteSafeOpenCL(inputData,
		[&](cv::UMat& cvDestBgra)
		{

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows)
				.Int("threshold", static_cast<int>(threshold));

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode(
				"IDR_OPENCL_COLOR", "Solarization",
				params,
				inputData.cols,
				inputData.rows);

		});
}

void COpenCLFilter::Median(UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			cv::UMat dest;
			medianBlur(image, dest, 3);
			return dest;
		});
}

void COpenCLFilter::Noise(UMat& inputData)
{
	ExecuteSafeOpenCL(inputData,
		[&](cv::UMat& cvDestBgra)
		{

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows);

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode(
				"IDR_OPENCL_NOISE", "Noise",
				params,
				inputData.cols,
				inputData.rows);

		});
}


void COpenCLFilter::Flip(const wxString& functionName, UMat& inputData)
{
	
	if (functionName == "FlipVertical")
	{
		flip(inputData, inputData, 0);
	}
	else
	{
		flip(inputData, inputData, 1);
	}
}

void COpenCLFilter::Swirl(const float& radius, const float& angle, UMat& inputData)
{
	ExecuteSafeOpenCL(inputData,
		[&](cv::UMat& cvDestBgra)
		{

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows)
				.Float("radius", radius)
				.Float("angle", angle);

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode(
				"IDR_OPENCL_SWIRL", "Swirl",
				params,
				inputData.cols,
				inputData.rows);

		});
}


cv::UMat COpenCLFilter::Interpolation(const int& widthOut, const int& heightOut, cv::UMat& inputData, const int& method, int width, int height, int flipH, int flipV, int angle, bool bgraOutput)
{

	return ExecuteSafeOpenCLWithUMatOutput(inputData, bgraOutput,
		[&](cv::UMat& cvDestBgra)
		{

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows)
				.Int("widthOut", widthOut)
				.Int("heightOut", heightOut)
				.Int("flipH", flipH)
				.Int("flipV", flipV)
				.Int("angle", angle)
				.Int("type", method);

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode("IDR_OPENCL_INTERPOLATION", "Interpolation", params, widthOut, heightOut);

		});
}


cv::UMat COpenCLFilter::Interpolation(const int& widthOut, const int& heightOut, cv::UMat& inputData, int width, int height, const int& method, bool bgraOutput)
{
	return ExecuteSafeOpenCLWithUMatOutput(inputData, bgraOutput,
		[&](cv::UMat& cvDestBgra)
		{

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows)
				.Int("widthOut", widthOut)
				.Int("heightOut", heightOut)
				.Int("type", method);

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode("IDR_OPENCL_INTERPOLATION", "InterpolationDirect", params, widthOut, heightOut);

		});
}

cv::UMat COpenCLFilter::Interpolation(const int& widthOut, const int& heightOut, const wxRect& rc, const int& method, cv::UMat& inputData, int width, int height, int flipH, int flipV, int angle, bool bgraOutput)
{
	return ExecuteSafeOpenCLWithUMatOutput(inputData, bgraOutput,
		[&](cv::UMat& cvDestBgra)
		{

			auto clBuffer = static_cast<cl_mem>(cvDestBgra.handle(ACCESS_READ));

			OpenCLKernelBuilder builder;

			builder
				.Image("input", clBuffer)
				.Int("width", inputData.cols)
				.Int("height", inputData.rows)
				.Int("widthOut", widthOut)
				.Int("heightOut", heightOut)
				.Float("left", rc.x)
				.Float("top", rc.y)
				.Float("bitmapWidth", rc.width)
				.Float("bitmapHeight", rc.height)
				.Int("flipH", flipH)
				.Int("flipV", flipV)
				.Int("angle", angle)
				.Int("type", method);

			auto params = builder.GetParameters();

			return ExecuteOpenCLCode("IDR_OPENCL_INTERPOLATION", "InterpolationZone", params, widthOut, heightOut);

		});
}

void COpenCLFilter::BrightnessAndContrast(const double& brightness, const double& contrast, UMat& inputData)
{
	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			UMat cvDest;
			convertScaleAbs(image, cvDest, contrast / 100.0f, brightness);
			return cvDest;	
		});
}


void COpenCLFilter::ColorEffect(const wxString& functionName, UMat& inputData)
{

	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{

			UMat cvDest;
			cv::UMat dest;

			if (functionName == "Sepia")
			{
				Mat kernel =
					(cv::Mat_<float>(3, 3)
						<<
						0.272, 0.534, 0.131,
						0.349, 0.686, 0.168,
						0.393, 0.769, 0.189);

				cv::transform(image, dest, kernel);
			}
			else if (functionName == "Negatif")
			{
				bitwise_not(image, dest);
			}
			else if (functionName == "NoirEtBlanc")
			{
				cvtColor(image, cvDest, COLOR_BGR2GRAY);
				threshold(cvDest, cvDest, 127, 255, THRESH_BINARY);
				cvtColor(cvDest, dest, COLOR_GRAY2BGR);
			}
			else if (functionName == "GrayLevel")
			{
				cvtColor(image, cvDest, COLOR_BGR2GRAY);
				cvtColor(cvDest, dest, COLOR_GRAY2BGR);
			}
			return dest;
		});
}

uint8_t * COpenCLFilter::HQDn3D(const double& LumSpac, const double& temporalLumaDefault, const double& temporalSpatialLumaDefault,
	Mat& inputData)
{
	uint8_t* dataOut = nullptr;

	ExecuteSafe([&]
		{

			if (hq3d == nullptr)
				hq3d = std::make_unique<Chqdn3d>(inputData.cols, inputData.rows, LumSpac, temporalLumaDefault, temporalSpatialLumaDefault);
			else if (oldLevelDenoise != LumSpac || inputData.cols != oldwidthDenoise || inputData.rows != oldheightDenoise)
			{
				hq3d.reset(new Chqdn3d(inputData.cols, inputData.rows, LumSpac, temporalLumaDefault, temporalSpatialLumaDefault));
			}
			dataOut = hq3d->ApplyDenoise3D(inputData.data, inputData.cols, inputData.rows);
		});

	return dataOut;
}

void COpenCLFilter::HQDn3D(const double& LumSpac, const double& temporalLumaDefault, const double& temporalSpatialLumaDefault,
                           UMat& inputData)
{
	
	if (hq3d == nullptr)
		hq3d = std::make_unique<Chqdn3d>(inputData.cols, inputData.rows, LumSpac, temporalLumaDefault, temporalSpatialLumaDefault);
	else if (oldLevelDenoise != LumSpac || inputData.cols != oldwidthDenoise || inputData.rows != oldheightDenoise)
	{
		hq3d.reset(new Chqdn3d(inputData.cols, inputData.rows, LumSpac, temporalLumaDefault, temporalSpatialLumaDefault));
	}

	ExecuteSafeOpenCL3Channels(inputData,
		[&](cv::UMat& image)
		{
			return hq3d->ApplyDenoise3D(image);
		});

}

void COpenCLFilter::Rotate(const wxString& functionName, const int& widthOut, const int& heightOut, const double& angle,
	UMat& inputData, const cv::Scalar& bgColor, const bool& preview)
{
	ExecuteSafe([&]
		{
			// 1. Calculs communs (Centre, Boîte englobante et Matrice initiale)
			const Point2f center((inputData.cols - 1) / 2.0, (inputData.rows - 1) / 2.0);
			const Rect2f bbox = RotatedRect(Point2f(), inputData.size(), angle).boundingRect2f();

			Mat rot;
			Size targetSize;

			if (preview)
			{
				// Mode Preview : Réduction de l'image pour qu'elle rentre dans le cadre d'origine
				double scaleX = (bbox.width > 0) ? inputData.cols / static_cast<double>(bbox.width) : 1.0;
				double scaleY = (bbox.height > 0) ? inputData.rows / static_cast<double>(bbox.height) : 1.0;
				double scale = std::min({ 1.0, scaleX, scaleY });

				rot = getRotationMatrix2D(center, angle, scale);
				targetSize = inputData.size();
			}
			else
			{
				// Mode Normal : Agrandissement du cadre (boîte englobante complète)
				rot = getRotationMatrix2D(center, angle, 1.0);
				targetSize = bbox.size();

				// Ajustement de la matrice de transformation pour le nouveau cadre
				rot.at<double>(0, 2) += bbox.width / 2.0 - inputData.cols / 2.0;
				rot.at<double>(1, 2) += bbox.height / 2.0 - inputData.rows / 2.0;
			}

			// 2. Application de la transformation affine commune
			UMat cvDest;
			warpAffine(inputData, cvDest, rot, targetSize, cv::INTER_LINEAR, cv::BORDER_CONSTANT, bgColor);
			cvDest.copyTo(inputData);
		});
}



Rect COpenCLFilter::CalculRect(int widthIn, int heightIn, int widthOut, int heightOut, int flipH, int flipV, int angle,
	float ratioX, float ratioY, int x, int y, float left, float top)
{
	// Calcul initial des positions
	float posX = static_cast<float>(x) * ratioX + left * ratioX;
	float posY = static_cast<float>(y) * ratioY + top * ratioY;

	// Gestion des rotations
	switch (angle)
	{
	case 90:
		std::swap(posX, posY);
		posX = widthIn - posX - 1;
		break;
	case 180:
		posX = widthIn - posX - 1;
		posY = heightIn - posY - 1;
		break;
	case 270:
		std::swap(posX, posY);
		posY = heightIn - posY - 1;
		break;
	}

	// Gestion des flips
	if (flipH == 1)
	{
		posX = widthIn - posX - 1;
	}
	if (flipV == 1)
	{
		posY = heightIn - posY - 1;
	}

	// Création du rectangle avec des coordonnées absolues
	return Rect(abs(posX), abs(posY), 0, 0);
}

void COpenCLFilter::ExecuteOpenCLCode(const wxString& programName, const wxString& functionName,
	vector<COpenCLParameter*>& vecParam, const int& width, const int& height,
	cl_mem& outBuffer)
{
	ParameterReleaseGuard releaseGuard(vecParam);

	ExecuteSafe([&]
		{
			//ocl::Context context = openCLContext->GetExecutionContext().getContext();
			ocl::Program program = openCLContext->GetProgram(programName);

			ocl::Kernel kernel(functionName, program);

			// Définition du premier argument (outBuffer)
			cl_int err = clSetKernelArg(static_cast<cl_kernel>(kernel.ptr()), 0, sizeof(cl_mem), &outBuffer);
			if (err != CL_SUCCESS)
			{
				throw std::runtime_error("Failed to set kernel argument for outBuffer.");
			}

			// Ajout des autres arguments
			int numArg = 1;
			for (COpenCLParameter* parameter : vecParam)
			{
				parameter->Add(static_cast<cl_kernel>(kernel.ptr()), numArg++);
			}

			// Configuration et exécution du kernel
			size_t global_work_size[2] = { static_cast<size_t>(width), static_cast<size_t>(height) };
			bool success = kernel.run(2, global_work_size, nullptr, true);

			if (!success)
			{
				throw std::runtime_error("Failed to execute OpenCL kernel.");
			}
		});
}

void COpenCLFilter::ClearCache()
{
	// Parcourir chaque élément du cache pour libérer physiquement les buffers GPU
	for (auto& pair : openclMemTempMap)
	{
		if (pair.second)
		{
			pair.second->cl_image = nullptr;
			pair.second->openclMem.release();
		}
	}
	// Vider complètement la map
	openclMemTempMap.clear();
}

UMat COpenCLFilter::ExecuteOpenCLCode(const wxString& programName, const wxString& functionName,
	vector<COpenCLParameter*>& vecParam, const int& width, const int& height)
{
	auto& memInfoPtr = openclMemTempMap[functionName];
	if (!memInfoPtr)
	{
		memInfoPtr = std::make_unique<OpenCLMemoryTemp>();
	}

	OpenCLMemoryTemp* memInfo = memInfoPtr.get();

	// === OPTIMISATION SÉCURISÉE DE LA MÉMOIRE GPU ===
	if (memInfo->openclMem.empty() ||
		memInfo->openclMem.cols != width ||
		memInfo->openclMem.rows != height)
	{
		// Sous Linux, forcer la libération de l'ancienne UMat et de son handle cl_mem
		// AVANT de faire le .create() évite la fragmentation de la VRAM.
		memInfo->cl_image = nullptr;
		memInfo->openclMem.release(); 

		memInfo->openclMem.create(height, width, CV_8UC4);
		memInfo->cl_image = static_cast<cl_mem>(memInfo->openclMem.handle(ACCESS_WRITE));
	}

	if (memInfo->cl_image == nullptr)
	{
		memInfo->cl_image = static_cast<cl_mem>(memInfo->openclMem.handle(ACCESS_WRITE));
	}

	// Exécute le code OpenCL
	ExecuteOpenCLCode(programName, functionName, vecParam, width, height, memInfo->cl_image);

	return memInfo->openclMem;
}

cv::UMat COpenCLFilter::Interpolation(const int& widthOut, const int& heightOut, const wxRect& rc, const int& method,
	UMat& inputData, int flipH, int flipV, int angle, int ratio, bool bgraOutput)
{
	cv::UMat cvDestBgra;
	bool _useSuperResolution = false;
	CRegardsConfigParam* regardsParam = CParamInit::getInstance();
	int superDnn = regardsParam->GetSuperResolutionType();
	int useSuperResolution = regardsParam->GetUseSuperResolution();
	if (useSuperResolution && superSampling->TestIfMethodIsValid(superDnn, (ratio / 100)) && !isDnnUsed)
		_useSuperResolution = true;

	try
	{
		float ratioX = static_cast<float>(inputData.cols) / rc.width;
		float ratioY = static_cast<float>(inputData.rows) / rc.height;
		if (angle == 90 || angle == 270)
		{
			ratioX = static_cast<float>(inputData.cols) / static_cast<float>(rc.height);
			ratioY = static_cast<float>(inputData.rows) / static_cast<float>(rc.width);
		}

		Rect rectGlobal;
		Rect rect_begin = CalculRect(inputData.cols, inputData.rows, widthOut, heightOut, flipH, flipV, angle, ratioX,
			ratioY, 0, 0, rc.x, rc.y);
		Rect rect_end = CalculRect(inputData.cols, inputData.rows, widthOut, heightOut, flipH, flipV, angle, ratioX,
			ratioY, widthOut, heightOut, rc.x, rc.y);
		rectGlobal.x = rect_begin.x;
		rectGlobal.y = rect_begin.y;
		rectGlobal.width = rect_end.x;
		rectGlobal.height = rect_end.y;
		if (rectGlobal.x > rectGlobal.width)
		{
			int x_end = rectGlobal.x;
			int x = rectGlobal.width;
			rectGlobal.x = x;
			rectGlobal.width = x_end - x;
		}
		else
		{
			rectGlobal.width -= rectGlobal.x;
		}

		if (rectGlobal.y > rectGlobal.height)
		{
			int y_end = rectGlobal.y;
			int y = rectGlobal.height;
			rectGlobal.y = y;
			rectGlobal.height = y_end - y;
		}
		else
		{
			rectGlobal.height -= rectGlobal.y;
		}

		if ((rectGlobal.height + rectGlobal.y) > inputData.rows)
		{
			rectGlobal.height = inputData.rows - rectGlobal.y;
		}
		if ((rectGlobal.width + rectGlobal.x) > inputData.cols)
		{
			rectGlobal.width = inputData.cols - rectGlobal.x;
		}

		inputData(rectGlobal).copyTo(cvDestBgra);

		if (angle == 270)
		{
			if (flipV && flipH)
				cv::rotate(cvDestBgra, cvDestBgra, ROTATE_90_CLOCKWISE);
			else if (flipV || flipH)
				cv::rotate(cvDestBgra, cvDestBgra, ROTATE_90_COUNTERCLOCKWISE);
			else
				cv::rotate(cvDestBgra, cvDestBgra, ROTATE_90_CLOCKWISE);
		}
		else if (angle == 90)
		{
			if (flipV && flipH)
				cv::rotate(cvDestBgra, cvDestBgra, ROTATE_90_COUNTERCLOCKWISE);
			else if (flipV || flipH)
				cv::rotate(cvDestBgra, cvDestBgra, ROTATE_90_CLOCKWISE);
			else
				cv::rotate(cvDestBgra, cvDestBgra, ROTATE_90_COUNTERCLOCKWISE);
		}
		else if (angle == 180)
		{
			cv::rotate(cvDestBgra, cvDestBgra, ROTATE_180);
		}

		if (_useSuperResolution)
		{
			if (cvDestBgra.channels() == 4)
			{
				cv::UMat alpha;
				cv::UMat picture;
				cv::UMat restoredAlpha;

				// 1. Sauvegarder la couche Alpha (canal 3) et convertir le reste en BGR
				cv::extractChannel(cvDestBgra, alpha, 3);
				cv::cvtColor(cvDestBgra, picture, cv::COLOR_BGRA2BGR);

				// 2. Appliquer le Super-Sampling sur les 3 canaux BGR
				picture = superSampling->upscaleImage(picture, superDnn, (ratio / 100));

				// 3. Adapter la taille de la couche Alpha à la nouvelle résolution de l'image
				cv::resize(
					alpha,
					restoredAlpha,
					picture.size(),
					0.0,
					0.0,
					cv::INTER_LINEAR);

				// 4. Reconstruire l'image finale en BGRA avec l'Alpha mis à l'échelle
				cv::cvtColor(picture, cvDestBgra, cv::COLOR_BGR2BGRA);
				cv::insertChannel(restoredAlpha, cvDestBgra, 3);
			}
			else
			{
				// Traitement standard direct pour du 3 canaux natif
				cvDestBgra = superSampling->upscaleImage(cvDestBgra, superDnn, (ratio / 100));
			}

		}
		else if (method == 7) // AVIR INTERPOLATION
		{
			if (!resizer->IsInit())
				resizer->Init();

			bool result = resizer->IsOk();
			if (result)
			{
				SAvirResizeParams params;
				params.linearizeGamma = true;
				params.sharpen = true;
				params.sharpenAmount = 0.35f;
				params.dither = false;
				params.peakValue = 1.0f;

				cv::UMat dstRGBA;
				dstRGBA.create(heightOut, widthOut, CV_8UC4);

				if (cvDestBgra.channels() == 3)
				{
					cv::UMat bitmapMatrix;
					cvtColor(cvDestBgra, bitmapMatrix, cv::COLOR_BGR2BGRA);

					result = resizer->Resize(bitmapMatrix, dstRGBA, params);

					if (result)
					{
						dstRGBA.copyTo(cvDestBgra);
						// Gestion du retour au format 3 channels d'origine si bgraOutput n'est pas requis
						if (!bgraOutput)
							cvtColor(cvDestBgra, cvDestBgra, COLOR_BGRA2BGR);
					}
				}
				else
				{
					result = resizer->Resize(cvDestBgra, dstRGBA, params);
					if (result)
						dstRGBA.copyTo(cvDestBgra);
				}
			}

			if (!result)
				resize(cvDestBgra, cvDestBgra, Size(widthOut, heightOut), method - 1);
		}
		else if (method > 7)
		{
			int localMethod = method - 7;

			if (cvDestBgra.channels() == 3)
			{
				cv::UMat bitmapMatrix;
				cvtColor(cvDestBgra, bitmapMatrix, cv::COLOR_BGR2BGRA);

				cvDestBgra = Interpolation(widthOut, heightOut, bitmapMatrix, bitmapMatrix.cols, bitmapMatrix.rows, localMethod, bgraOutput);

				if (cvDestBgra.channels() == 3 && !bgraOutput)
					cvtColor(cvDestBgra, cvDestBgra, COLOR_BGRA2BGR);
			}
			else
			{
				cvDestBgra = Interpolation(widthOut, heightOut, cvDestBgra, cvDestBgra.cols, cvDestBgra.rows, localMethod, bgraOutput);
			}
		}
		else if (cvDestBgra.cols != widthOut || cvDestBgra.rows != heightOut)
		{
			resize(cvDestBgra, cvDestBgra, Size(widthOut, heightOut), method);
		}

		// Nettoyage de la duplication inutile du bloc resize(cvDestBgra...) de fin

		// Application des transformations finales de Flip
		if (flipH)
		{
			if (angle == 90 || angle == 270)
				flip(cvDestBgra, cvDestBgra, 0);
			else
				flip(cvDestBgra, cvDestBgra, 1);
		}
		if (flipV)
		{
			if (angle == 90 || angle == 270)
				flip(cvDestBgra, cvDestBgra, 1);
			else
				flip(cvDestBgra, cvDestBgra, 0);
		}
	}
	catch (Exception& e)
	{
		const char* err_msg = e.what();
		std::cout << "COpenCLFilter::Interpolation exception caught: " << err_msg << std::endl;
		std::cout << "width : " << widthOut << "height : " << heightOut << std::endl;
		cv::Mat image(widthOut, heightOut, CV_8UC3, cv::Scalar(0, 0, 0));
		image.copyTo(cvDestBgra);
	}

	// Uniquement la conversion finale conditionnelle d'origine sur les 3 channels
	if (bgraOutput && cvDestBgra.channels() == 3)
		cvtColor(cvDestBgra, cvDestBgra, COLOR_BGR2BGRA);

	return cvDestBgra;
}
