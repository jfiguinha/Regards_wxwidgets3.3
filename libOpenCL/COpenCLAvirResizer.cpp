#include "COpenCLAvirResizer.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace Regards::OpenCL;

namespace
{
	constexpr double AVIR_PI = 3.14159265358979323846;
	constexpr double AVIR_PId2 = AVIR_PI * 0.5;
}

COpenCLAvirResizer::COpenCLAvirResizer(cl_context context, cl_command_queue queue, cl_device_id device)
	: m_context(context)
	, m_queue(queue)
	, m_device(device)
{
}

COpenCLAvirResizer::~COpenCLAvirResizer()
{
	ReleaseKernels();

	if (m_program)
		clReleaseProgram(m_program);
}

void COpenCLAvirResizer::ReleaseKernels()
{
	cl_kernel* kernels[] = {
		&m_kLinearize, &m_kDelinearize, &m_kResizeH, &m_kResizeV,
		&m_kSharpenH, &m_kSharpenV, &m_kDitherRound, &m_kDitherErrDiff
	};

	for (cl_kernel* k : kernels)
	{
		if (*k)
		{
			clReleaseKernel(*k);
			*k = nullptr;
		}
	}
}

cl_kernel COpenCLAvirResizer::CreateKernel(const char* name)
{
	cl_int err = CL_SUCCESS;
	cl_kernel k = clCreateKernel(m_program, name, &err);
	if (err != CL_SUCCESS)
	{
		fprintf(stderr, "COpenCLAvirResizer: echec creation kernel '%s' (err=%d)\n", name, err);
		return nullptr;
	}
	return k;
}

bool COpenCLAvirResizer::Init(const char* clSourcePath)
{
	std::ifstream file(clSourcePath, std::ios::binary);
	if (!file)
	{
		fprintf(stderr, "COpenCLAvirResizer: impossible d'ouvrir '%s'\n", clSourcePath);
		return false;
	}

	std::ostringstream ss;
	ss << file.rdbuf();
	const std::string src = ss.str();
	const char* srcPtr = src.c_str();
	size_t srcLen = src.size();

	cl_int err = CL_SUCCESS;
	m_program = clCreateProgramWithSource(m_context, 1, &srcPtr, &srcLen, &err);
	if (err != CL_SUCCESS)
		return false;

	err = clBuildProgram(m_program, 1, &m_device, "-cl-fast-relaxed-math", nullptr, nullptr);
	if (err != CL_SUCCESS)
	{
		size_t logSize = 0;
		clGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
		std::vector<char> log(logSize + 1, 0);
		clGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
		fprintf(stderr, "COpenCLAvirResizer: erreur de compilation :\n%s\n", log.data());
		return false;
	}

	m_kLinearize     = CreateKernel("k_linearize_srgb");
	m_kDelinearize   = CreateKernel("k_delinearize_srgb");
	m_kResizeH       = CreateKernel("k_resize_h");
	m_kResizeV       = CreateKernel("k_resize_v");
	m_kSharpenH      = CreateKernel("k_sharpen_h");
	m_kSharpenV      = CreateKernel("k_sharpen_v");
	m_kDitherRound   = CreateKernel("k_dither_round");
	m_kDitherErrDiff = CreateKernel("k_dither_errdiffusion");

	return m_kLinearize && m_kDelinearize && m_kResizeH && m_kResizeV &&
		m_kSharpenH && m_kSharpenV && m_kDitherRound && m_kDitherErrDiff;
}

// Fenetre Peaked Cosine d'AVIR, formule equivalente a
// CDSPWindowGenPeakedCosine mais evaluee directement pour un x continu
// (au lieu du generateur recursif a pas entiers d'AVIR, qui suppose un
// echantillonnage a la position entiere) :
//
//   w(x) = cos(pi/2 * x/len2) * (1 - (|x|/len2)^alpha),  pour |x| < len2
//
double COpenCLAvirResizer::PeakedCosineWindow(double x, double len2, double alpha)
{
	if (len2 <= 0.0)
		return 0.0;

	const double ax = std::fabs(x);
	if (ax >= len2)
		return 0.0;

	const double h = std::pow(ax / len2, alpha);
	return std::cos(AVIR_PId2 * x / len2) * (1.0 - h);
}

void COpenCLAvirResizer::BuildAxisFilters(
	int srcLen, int dstLen,
	const SAvirResizeParams& params,
	std::vector<SResizeTap>& outTaps,
	std::vector<float>& outCoefs) const
{
	const double scale = (double)dstLen / (double)srcLen;
	const bool downsizing = scale < 1.0;

	// Coupure reduite en cas de sous-echantillonnage, pour l'anti-repliement
	// (meme logique qu'AVIR : le filtre est a la fois interpolateur et
	// anti-aliasing).
	const double cutoff = AVIR_PI * (downsizing ? scale : 1.0) * params.cutoffMult;

	// La demi-longueur augmente avec le facteur de reduction : plus on
	// reduit, plus il faut de taps pour bien filtrer avant repliement.
	const double len2 = (params.baseTaps * 0.5) / (downsizing ? scale : 1.0);
	const int fl2 = (int)std::ceil(len2);
	const int tapCount = fl2 * 2 + 1;

	outTaps.resize(dstLen);
	outCoefs.clear();
	outCoefs.reserve((size_t)dstLen * tapCount);

	std::vector<double> tmp(tapCount);

	for (int i = 0; i < dstLen; ++i)
	{
		// Alignement centre-a-centre des grilles source/destination.
		const double srcPos = ((i + 0.5) / scale) - 0.5;
		const int srcPosInt = (int)std::floor(srcPos);
		const double frac = srcPos - srcPosInt;

		const int tapStart = srcPosInt - fl2;
		const size_t coefOffset = outCoefs.size();

		double sum = 0.0;

		for (int k = 0; k < tapCount; ++k)
		{
			// Distance continue (non entiere) entre le tap k et le centre
			// exact de l'echantillon source demande.
			const double t = (double)(k - fl2) - frac;

			double v;
			if (std::fabs(t) < 1e-9)
				v = cutoff / AVIR_PI;
			else
				v = std::sin(cutoff * t) / (AVIR_PI * t);

			v *= PeakedCosineWindow(t, len2, params.alpha);

			tmp[k] = v;
			sum += v;
		}

		// Normalisation a gain DC unitaire (preserve la luminosite globale).
		if (std::fabs(sum) > 1e-12)
		{
			const double inv = 1.0 / sum;
			for (double& v : tmp)
				v *= inv;
		}

		for (int k = 0; k < tapCount; ++k)
			outCoefs.push_back((float)tmp[k]);

		outTaps[i] = { tapStart, tapCount, (cl_int)coefOffset };
	}
}

std::vector<float> COpenCLAvirResizer::BuildSharpenKernel(int klen) const
{
	// Noyau passe-bas symetrique (le "flou" de reference pour l'unsharp
	// mask). Volontairement generique : ce n'est pas un portage du filtre
	// de correction d'AVIR (voir note en tete de fichier .h).
	const int half = klen / 2;
	const double len2 = half + 0.5;
	const double cutoff = AVIR_PId2;

	std::vector<double> lp(klen);
	double sum = 0.0;

	for (int k = 0; k < klen; ++k)
	{
		const double t = (double)(k - half);
		double v = (std::fabs(t) < 1e-9)
			? (cutoff / AVIR_PI)
			: (std::sin(cutoff * t) / (AVIR_PI * t));

		v *= PeakedCosineWindow(t, len2, 1.2);
		lp[k] = v;
		sum += v;
	}

	for (double& v : lp)
		v /= sum;

	std::vector<float> out(klen);
	for (int k = 0; k < klen; ++k)
		out[k] = (float)lp[k];

	return out;
}

bool COpenCLAvirResizer::Resize(
	const float* srcRGBA, int srcWidth, int srcHeight,
	float* dstRGBA, int dstWidth, int dstHeight,
	const SAvirResizeParams& params)
{
	if (!m_program || srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0)
		return false;

	cl_int err = CL_SUCCESS;

	const size_t srcCount = (size_t)srcWidth * srcHeight;
	const size_t midCount = (size_t)dstWidth * srcHeight;   // apres la passe H
	const size_t dstCount = (size_t)dstWidth * dstHeight;

	cl_mem bufSrc = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		srcCount * sizeof(cl_float4), (void*)srcRGBA, &err);
	if (err != CL_SUCCESS) return false;

	cl_mem bufMid = clCreateBuffer(m_context, CL_MEM_READ_WRITE, midCount * sizeof(cl_float4), nullptr, &err);
	if (err != CL_SUCCESS) { clReleaseMemObject(bufSrc); return false; }

	cl_mem bufDst = clCreateBuffer(m_context, CL_MEM_READ_WRITE, dstCount * sizeof(cl_float4), nullptr, &err);
	if (err != CL_SUCCESS) { clReleaseMemObject(bufSrc); clReleaseMemObject(bufMid); return false; }

	// --- gamma : linearisation ---
	if (params.linearizeGamma)
	{
		const int count = (int)srcCount;
		clSetKernelArg(m_kLinearize, 0, sizeof(cl_mem), &bufSrc);
		clSetKernelArg(m_kLinearize, 1, sizeof(int), &count);
		const size_t g = srcCount;
		clEnqueueNDRangeKernel(m_queue, m_kLinearize, 1, nullptr, &g, nullptr, 0, nullptr, nullptr);
	}

	// --- passe horizontale ---
	std::vector<SResizeTap> tapsH;
	std::vector<float> coefsH;
	BuildAxisFilters(srcWidth, dstWidth, params, tapsH, coefsH);

	cl_mem bufTapsH = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		tapsH.size() * sizeof(SResizeTap), tapsH.data(), &err);
	cl_mem bufCoefsH = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		coefsH.size() * sizeof(float), coefsH.data(), &err);

	{
		int sw = srcWidth, sh = srcHeight, dw = dstWidth;
		clSetKernelArg(m_kResizeH, 0, sizeof(cl_mem), &bufSrc);
		clSetKernelArg(m_kResizeH, 1, sizeof(int), &sw);
		clSetKernelArg(m_kResizeH, 2, sizeof(int), &sh);
		clSetKernelArg(m_kResizeH, 3, sizeof(cl_mem), &bufMid);
		clSetKernelArg(m_kResizeH, 4, sizeof(int), &dw);
		clSetKernelArg(m_kResizeH, 5, sizeof(cl_mem), &bufTapsH);
		clSetKernelArg(m_kResizeH, 6, sizeof(cl_mem), &bufCoefsH);
		const size_t g[2] = { (size_t)dstWidth, (size_t)srcHeight };
		clEnqueueNDRangeKernel(m_queue, m_kResizeH, 2, nullptr, g, nullptr, 0, nullptr, nullptr);
	}

	clReleaseMemObject(bufTapsH);
	clReleaseMemObject(bufCoefsH);

	// --- passe verticale ---
	std::vector<SResizeTap> tapsV;
	std::vector<float> coefsV;
	BuildAxisFilters(srcHeight, dstHeight, params, tapsV, coefsV);

	cl_mem bufTapsV = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		tapsV.size() * sizeof(SResizeTap), tapsV.data(), &err);
	cl_mem bufCoefsV = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		coefsV.size() * sizeof(float), coefsV.data(), &err);

	{
		int mw = dstWidth, mh = srcHeight, dh = dstHeight;
		clSetKernelArg(m_kResizeV, 0, sizeof(cl_mem), &bufMid);
		clSetKernelArg(m_kResizeV, 1, sizeof(int), &mw);
		clSetKernelArg(m_kResizeV, 2, sizeof(int), &mh);
		clSetKernelArg(m_kResizeV, 3, sizeof(cl_mem), &bufDst);
		clSetKernelArg(m_kResizeV, 4, sizeof(int), &dh);
		clSetKernelArg(m_kResizeV, 5, sizeof(cl_mem), &bufTapsV);
		clSetKernelArg(m_kResizeV, 6, sizeof(cl_mem), &bufCoefsV);
		const size_t g[2] = { (size_t)dstWidth, (size_t)dstHeight };
		clEnqueueNDRangeKernel(m_queue, m_kResizeV, 2, nullptr, g, nullptr, 0, nullptr, nullptr);
	}

	clReleaseMemObject(bufTapsV);
	clReleaseMemObject(bufCoefsV);
	clReleaseMemObject(bufMid);

	// --- nettete (optionnelle, seulement en agrandissement horizontal) ---
	const double kx = (double)dstWidth / (double)srcWidth;

	if (params.sharpen && kx > 1.0)
	{
		std::vector<float> sk = BuildSharpenKernel(5);
		cl_mem bufKernel = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			sk.size() * sizeof(float), sk.data(), &err);
		cl_mem bufTmp = clCreateBuffer(m_context, CL_MEM_READ_WRITE, dstCount * sizeof(cl_float4), nullptr, &err);

		int w = dstWidth, h = dstHeight, klen = (int)sk.size();
		float amount = params.sharpenAmount;

		clSetKernelArg(m_kSharpenH, 0, sizeof(cl_mem), &bufDst);
		clSetKernelArg(m_kSharpenH, 1, sizeof(int), &w);
		clSetKernelArg(m_kSharpenH, 2, sizeof(int), &h);
		clSetKernelArg(m_kSharpenH, 3, sizeof(cl_mem), &bufTmp);
		clSetKernelArg(m_kSharpenH, 4, sizeof(cl_mem), &bufKernel);
		clSetKernelArg(m_kSharpenH, 5, sizeof(int), &klen);
		clSetKernelArg(m_kSharpenH, 6, sizeof(float), &amount);
		const size_t g[2] = { (size_t)w, (size_t)h };
		clEnqueueNDRangeKernel(m_queue, m_kSharpenH, 2, nullptr, g, nullptr, 0, nullptr, nullptr);

		clSetKernelArg(m_kSharpenV, 0, sizeof(cl_mem), &bufTmp);
		clSetKernelArg(m_kSharpenV, 1, sizeof(int), &w);
		clSetKernelArg(m_kSharpenV, 2, sizeof(int), &h);
		clSetKernelArg(m_kSharpenV, 3, sizeof(cl_mem), &bufDst);
		clSetKernelArg(m_kSharpenV, 4, sizeof(cl_mem), &bufKernel);
		clSetKernelArg(m_kSharpenV, 5, sizeof(int), &klen);
		clSetKernelArg(m_kSharpenV, 6, sizeof(float), &amount);
		clEnqueueNDRangeKernel(m_queue, m_kSharpenV, 2, nullptr, g, nullptr, 0, nullptr, nullptr);

		clReleaseMemObject(bufKernel);
		clReleaseMemObject(bufTmp);
	}

	// --- gamma : retour en sRGB ---
	if (params.linearizeGamma)
	{
		const int count = (int)dstCount;
		clSetKernelArg(m_kDelinearize, 0, sizeof(cl_mem), &bufDst);
		clSetKernelArg(m_kDelinearize, 1, sizeof(int), &count);
		const size_t g = dstCount;
		clEnqueueNDRangeKernel(m_queue, m_kDelinearize, 1, nullptr, &g, nullptr, 0, nullptr, nullptr);
	}

	// --- dithering (optionnel) ---
	if (params.dither)
	{
		if (params.ditherErrorDiffusion)
		{
			cl_mem bufErr = clCreateBuffer(m_context, CL_MEM_READ_WRITE, dstCount * sizeof(cl_float4), nullptr, &err);
			const cl_float4 zero = { { 0.0f, 0.0f, 0.0f, 0.0f } };
			clEnqueueFillBuffer(m_queue, bufErr, &zero, sizeof(zero), 0, dstCount * sizeof(cl_float4), 0, nullptr, nullptr);

			int w = dstWidth, h = dstHeight;
			float peak = params.peakValue;
			clSetKernelArg(m_kDitherErrDiff, 0, sizeof(cl_mem), &bufDst);
			clSetKernelArg(m_kDitherErrDiff, 1, sizeof(int), &w);
			clSetKernelArg(m_kDitherErrDiff, 2, sizeof(int), &h);
			clSetKernelArg(m_kDitherErrDiff, 3, sizeof(cl_mem), &bufErr);
			clSetKernelArg(m_kDitherErrDiff, 4, sizeof(float), &peak);
			const size_t g = 1;
			clEnqueueNDRangeKernel(m_queue, m_kDitherErrDiff, 1, nullptr, &g, nullptr, 0, nullptr, nullptr);

			clReleaseMemObject(bufErr);
		}
		else
		{
			const int count = (int)dstCount;
			float peak = params.peakValue;
			clSetKernelArg(m_kDitherRound, 0, sizeof(cl_mem), &bufDst);
			clSetKernelArg(m_kDitherRound, 1, sizeof(int), &count);
			clSetKernelArg(m_kDitherRound, 2, sizeof(float), &peak);
			const size_t g = dstCount;
			clEnqueueNDRangeKernel(m_queue, m_kDitherRound, 1, nullptr, &g, nullptr, 0, nullptr, nullptr);
		}
	}

	err = clEnqueueReadBuffer(m_queue, bufDst, CL_TRUE, 0, dstCount * sizeof(cl_float4), dstRGBA, 0, nullptr, nullptr);

	clReleaseMemObject(bufSrc);
	clReleaseMemObject(bufDst);

	return err == CL_SUCCESS;
}
