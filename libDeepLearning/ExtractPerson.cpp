#include "header.h"
#include "ExtractPerson.h"
#include <opencv2/dnn.hpp>
#include <FileUtility.h>
static  cv::dnn::Net net;
bool CExtractPerson::isLoad = false;
CExtractPerson::CExtractPerson()
{

}

CExtractPerson::~CExtractPerson()
{

}

bool CExtractPerson::IsLoaded()
{
    return isLoad;
}

int CExtractPerson::load()
{
    // 1. Charger le modèle U2-Net au format ONNX
    //std::string modelPath = "u2net.onnx"; // ou "u2netp.onnx" pour la version rapide
    net = cv::dnn::readNetFromONNX(CFileUtility::GetFullpathModel("u2net.onnx"));
    if (net.empty()) {
        std::cerr << "Impossible de charger le modèle ONNX !" << std::endl;
        return -1;
    }

    // Configurer l'exécution (Utiliser CUDA si disponible, sinon CPU)
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    isLoad = true;
    return 0;
}

cv::Mat CExtractPerson::extractPerson(const cv::Mat& src)
{
    if (!isLoad)
        load();

    // printf("CExtractPerson::colorization \n");
    cv::Mat out_image;
    try
    {
        if (src.empty()) {
            std::cerr << "Impossible de charger l'image !" << std::endl;
            return out_image;
        }


        // 3. Préparer l'image pour le réseau (Blob : 320x320, normalisation)
        // U2-Net demande une soustraction de la moyenne et une mise à l'échelle (0-1)
        cv::Mat blob;
        cv::dnn::blobFromImage(src, blob, 1.0 / 255.0, cv::Size(320, 320),
            cv::Scalar(0.485, 0.456, 0.406), true, false);
        // Note : On applique la normalisation standard ImageNet ((x/255) - mean) / std 
        // Pour simplifier ici : blobFromImage gère la taille et l'échelle de base.

        // Ajustement de la normalisation spécifique à U2-Net
        blob = (blob - 0.45) / 0.225;

        net.setInput(blob);

        // 4. Exécuter l'inférence
        std::cout << "Inférence U2-Net en cours..." << std::endl;
        cv::Mat output = net.forward(); // Donne une matrice de probabilité [1, 1, 320, 320]

        // 5. Post-traitement du masque
        // Extraire la matrice 2D du blob de sortie
        int sizes[] = { 320, 320 };
        cv::Mat mask(2, sizes, CV_32FC1, output.data);

        // Normalisation Min-Max pour étaler les valeurs proprement entre 0.0 et 1.0
        cv::normalize(mask, mask, 0.0, 1.0, cv::NORM_MINMAX, CV_32FC1);

        // Redimensionner le masque à la taille originale de l'image
        cv::Mat resizedMask;
        cv::resize(mask, resizedMask, src.size(), 0, 0, cv::INTER_LINEAR);

        // Convertir le masque flottant (0.0 - 1.0) en entier (0 - 255) pour le canal Alpha
        cv::Mat alphaChannel;
        resizedMask.convertTo(alphaChannel, CV_8UC1, 255.0);

        // Seuil optionnel pour rendre les contours plus nets (Binarisation)
        // cv::threshold(alphaChannel, alphaChannel, 128, 255, cv::THRESH_BINARY);

        // 6. Fusionner le masque comme canal Alpha
        // Séparer les canaux B, G, R de l'image d'origine
        std::vector<cv::Mat> channels;
        cv::split(src, channels);

        // Ajouter le masque généré par U2-Net comme 4ème canal (Alpha)
        channels.push_back(alphaChannel);

        cv::merge(channels, out_image);

        cv::Mat background(src.size(), src.type(), cv::Scalar(255, 255, 255));

        // La fonction copyTo va copier l'image d'origine (src) vers le fond blanc (background)
        // UNIQUEMENT là où le masque (alphaChannel) n'est pas noir.
        src.copyTo(background, alphaChannel);
        out_image = background;

    }
    catch (cv::Exception& e)
    {
        const char* err_msg = e.what();
        std::cout << "BilateralEffect exception caught: " << err_msg << std::endl;
        std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
    }



    return out_image;
}

