
#include <header.h>
#include "LayerCompositorOpenCL.h"




CLayerCompositorOpenCL::CLayerCompositorOpenCL(COpenCLContext* openCLContext)
{
    if (cv::ocl::haveOpenCL())
        cv::ocl::setUseOpenCL(true);

    this->openCLContext = openCLContext;
}

CLayerCompositorOpenCL::~CLayerCompositorOpenCL()
{
    Release();
}

void CLayerCompositorOpenCL::Release()
{
    // Les UMat libèrent leurs ressources par comptage de références.
}


bool CLayerCompositorOpenCL::Compose(
    const cv::UMat& background,
    const std::vector<LayerPassOpenCL>& layers,
    cv::UMat& target,
    int width,
    int height)
{
    if (!IsValid() || background.empty() || width <= 0 || height <= 0 || background.type() != CV_8UC4)
        return false;

    // 1. Filtrer les calques actifs pour éviter d'allouer de la mémoire vidéo inutilement
    std::vector<LayerPassOpenCL> activeLayers;
    for (const auto& layer : layers) {
        if (!layer.texture.empty() && layer.opacity > 0.0f) {
            activeLayers.push_back(layer);
        }
    }

    // S'il n'y a pas de calques, un simple redimensionnement ou copie suffit
    if (activeLayers.empty()) {
        if (background.size() == cv::Size(width, height)) background.copyTo(target);
        else cv::resize(background, target, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
        return true;
    }

    // 2. Préparer le conteneur de destination global (Contrat de sortie)
    if (target.empty() || target.cols != width || target.rows != height || target.type() != CV_8UC4) {
        target = cv::UMat(height, width, CV_8UC4);
    }

    // Ajuster le fond si nécessaire
    cv::UMat resizedBG = background;
    if (background.cols != width || background.rows != height) {
        cv::resize(background, resizedBG, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
    }

    // 3. Concaténer tous les calques actifs dans un unique Buffer GPU 3D/Continu
    // Cette étape élimine les 40 allocations OpenCL/UMat intermédiaires par frame.
    size_t layerSizeInBytes = (size_t)width * height * 4; // 4 canaux (BGRA) 8 bits
    cv::UMat megaBuffer(height * activeLayers.size(), width, CV_8UC4);

    std::vector<float> h_opacities;
    std::vector<int> h_modes;

    for (size_t i = 0; i < activeLayers.size(); ++i) {
        cv::UMat targetROI = megaBuffer(cv::Rect(0, i * height, width, height));

        if (activeLayers[i].texture.cols != width || activeLayers[i].texture.rows != height) {
            cv::resize(activeLayers[i].texture, targetROI, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
        }
        else {
            activeLayers[i].texture.copyTo(targetROI);
        }

        h_opacities.push_back(std::clamp(activeLayers[i].opacity, 0.0f, 1.0f));
        h_modes.push_back(activeLayers[i].blendMode);
    }

    // Envoi des métadonnées vers le GPU
    cv::Mat matOpacities(1, activeLayers.size(), CV_32FC1, h_opacities.data());
    cv::Mat matModes(1, activeLayers.size(), CV_32SC1, h_modes.data());
    cv::UMat gpuOpacities = matOpacities.getUMat(cv::ACCESS_READ);
    cv::UMat gpuModes = matModes.getUMat(cv::ACCESS_READ);

    // 4. Compilation à la volée du programme OpenCL via OpenCV (Mise en cache automatique par le driver)
    //cv::ocl::ProgramSource programSource("layer_compositor", kernelSource);
    cv::String errmsg;
    cv::String buildopt = "";
    cv::ocl::Program program = openCLContext->GetProgram("IDR_OPENCL_LAYERCOMPOSITOR");// cv::ocl::Context::getDefault().getProg(programSource, buildopt, errmsg);

    cv::ocl::Kernel kernel("compositeLayersKernel", program);
    if (kernel.empty()) {
        return false; // Échec de compilation du Kernel OpenCL
    }

    // 5. Configurer et exécuter les dimensions du Grid de threads (1 thread par pixel)
    size_t globalThreads[2] = { (size_t)width, (size_t)height };
    long layerStridePixels = (long)width * height;
    int numLayers = (int)activeLayers.size();

    // Injection des arguments mémoire dans l'exécuteur matériel
    kernel.args(
        cv::ocl::KernelArg::PtrReadOnly(resizedBG),
        cv::ocl::KernelArg::PtrReadOnly(megaBuffer),
        cv::ocl::KernelArg::PtrReadOnly(gpuOpacities),
        cv::ocl::KernelArg::PtrReadOnly(gpuModes),
        cv::ocl::KernelArg::PtrWriteOnly(target),
        numLayers,
        width,
        height,
        layerStridePixels
    );

    // Lancement du Kernel asynchrone sur le GPU
    return kernel.run(2, globalThreads, nullptr, true);
}

