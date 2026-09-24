
#include <header.h>
#include <LibResource.h>
#include <RGBAQuad.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <vector>

#include "Color.h"
#include "circle.h"

using namespace Regards::FiltreEffet;

namespace {
    constexpr double PI = 3.141592653589793238462643383279502884;

    constexpr int RGB_CHANNELS = 3;

    inline float GetDistance(const int x, const int y, const int centerX,
        const int centerY) {
        const int dx = x - centerX;
        const int dy = y - centerY;

        return std::sqrt(static_cast<float>(dx * dx + dy * dy));
    }

    inline uint8_t GetAlphaFromDistance(const float distance, const int radius) {
        if (radius <= 0 || distance >= radius) return 0;

        const float value = 255.0f - (distance / static_cast<float>(radius)) * 255.0f;

        return static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
    }

    inline void EnsureAlpha(wxImage& image) {
        if (!image.HasAlpha()) image.InitAlpha();
    }

    inline int ClampIndex(const int value, const int minValue, const int maxValue) {
        return std::clamp(value, minValue, maxValue);
    }
}  // namespace

std::map<int, wxImage> CCircle::listOfCircle;
std::mutex CCircle::circleMutex;

void CCircle::CleanCircle() {
    std::lock_guard<std::mutex> lock(circleMutex);

    listOfCircle.clear();
}

#include <wx/dcmemory.h>
#include <wx/graphics.h>

wxImage CCircle::GetCircle(const int& rayon) {
    if (rayon <= 0) return {};

    {
        std::lock_guard<std::mutex> lock(circleMutex);

        const auto it = listOfCircle.find(rayon);

        if (it != listOfCircle.end()) return it->second.Copy();
    }

    // 1. Création d'un bitmap RGB 24 bits aux dimensions exactes
    wxBitmap bitmap(rayon, rayon, 24);
    wxMemoryDC memDC(bitmap);

    // 2. Utilisation directe de wxGraphicsContext (natif et ultra-précis en 3.3)
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(memDC));

    if (gc) {
        // Activation explicite de l'anti-aliasing de haute qualité
        gc->SetAntialiasMode(wxAntialiasMode::wxANTIALIAS_DEFAULT);

        // Remplissage du fond en BLANC pur (zone transparente)
        gc->SetBrush(*wxWHITE_BRUSH);
        gc->DrawRectangle(0, 0, rayon, rayon);

        // Dessin du disque en NOIR pur (zone active pour vos filtres)
        gc->SetBrush(*wxBLACK_BRUSH);
        gc->SetPen(*wxTRANSPARENT_PEN);

        // DrawEllipse prend les coordonnées de la boîte englobante (0, 0, largeur, hauteur)
        gc->DrawEllipse(0, 0, rayon, rayon);
    }

    // Libération du contexte graphique et détachement du bitmap
    gc.reset();
    memDC.SelectObject(wxNullBitmap);

    // 3. Conversion en wxImage et initialisation de la couche Alpha
    wxImage image = bitmap.ConvertToImage();
    image.InitAlpha();

    uint8_t* data = image.GetData();
    uint8_t* alpha = image.GetAlpha();
    int totalPixels = rayon * rayon;

    // 4. Traitement parallèle pour générer l'Alpha et nettoyer les couleurs
    tbb::parallel_for(0, totalPixels, 1, [=](int i) {
        int idx = i * 3;
        uint8_t r = data[idx];
        uint8_t g = data[idx + 1];
        uint8_t b = data[idx + 2];

        // Si le pixel est blanc pur -> Extérieur (transparent)
        if (r == 255 && g == 255 && b == 255) {
            alpha[i] = 0;
        }
        else {
            // Conversion basée sur la luminance pour conserver le lissage parfait des bords
            float luminance = (r + g + b) / 3.0f;
            alpha[i] = static_cast<uint8_t>(255.0f - luminance);

            // Remise à zéro des couleurs (Noir pur) pour valider vos conditions 'if (data[pixel] == 0)'
            data[idx] = 0;
            data[idx + 1] = 0;
            data[idx + 2] = 0;
        }
        });

    {
        std::lock_guard<std::mutex> lock(circleMutex);

        const auto [it, inserted] = listOfCircle.emplace(rayon, image);

        if (!inserted) return it->second.Copy();
    }

    return image.Copy();
}

/*
wxImage CCircle::GetCircle(const int& rayon) {
    if (rayon <= 0) return {};

    {
        std::lock_guard<std::mutex> lock(circleMutex);

        const auto it = listOfCircle.find(rayon);

        if (it != listOfCircle.end()) return it->second.Copy();
    }

    wxImage image =
        CLibResource::CreatePictureFromSVG("IDB_CIRCLE", rayon, rayon);

    if (!image.IsOk()) return {};

    EnsureAlpha(image);

    {
        std::lock_guard<std::mutex> lock(circleMutex);

        const auto [it, inserted] = listOfCircle.emplace(rayon, image);

        if (!inserted) return it->second.Copy();
    }

    return image.Copy();
}
*/

wxImage CCircle::GenerateCircle(const CRgbaquad& color, const int& taille,
    const float& alphaValue) {
    if (taille <= 0) return {};

    wxImage image = GetCircle(taille * 2);

    if (!image.IsOk()) return {};

    EnsureAlpha(image);

    uint8_t* alpha = image.GetAlpha();

    uint8_t* data = image.GetData();

    if (alpha == nullptr || data == nullptr) {
        return {};
    }

    const int width = image.GetWidth();

    const int height = image.GetHeight();

    const float alphaFactor = std::clamp(alphaValue, 0.0f, 1.0f);

    const uint8_t red = color.GetRed();

    const uint8_t green = color.GetGreen();

    const uint8_t blue = color.GetBlue();

    tbb::parallel_for(0, height, 1, [=](const int y) {
        const int rowOffset = y * width;

        for (int x = 0; x < width; ++x) {
            const int index = rowOffset + x;

            const int pixel = index * RGB_CHANNELS;

            if (data[pixel] == 0 && data[pixel + 1] == 0 && data[pixel + 2] == 0) {
                alpha[index] =
                    static_cast<uint8_t>(alpha[index] * (1.0f - alphaFactor));
            }
        }
        });

    image.Replace(0, 0, 0, red, green, blue);

    return image;
}

wxImage CCircle::GradientTransparent(const CRgbaquad& color, const int& taille,
    const float& alphaValue) {
    if (taille <= 0) return {};

    wxImage image = GetCircle(taille);

    if (!image.IsOk()) return {};

    EnsureAlpha(image);

    uint8_t* alpha = image.GetAlpha();

    uint8_t* data = image.GetData();

    if (alpha == nullptr || data == nullptr) {
        return {};
    }

    const int width = image.GetWidth();

    const int height = image.GetHeight();

    const int radius = taille / 2;

    if (radius <= 0) return image;

    const float alphaFactor = std::clamp(alphaValue, 0.0f, 1.0f);

    const uint8_t red = color.GetRed();

    const uint8_t green = color.GetGreen();

    const uint8_t blue = color.GetBlue();

    tbb::parallel_for(0, height, 1, [=](const int y) {
        const int rowOffset = y * width;

        for (int x = 0; x < width; ++x) {
            const int index = rowOffset + x;

            const int pixel = index * RGB_CHANNELS;

            if (data[pixel] != 0 || data[pixel + 1] != 0 || data[pixel + 2] != 0) {
                continue;
            }

            const float distance = GetDistance(x, y, radius, radius);

            uint8_t resultAlpha = GetAlphaFromDistance(distance, radius);

            resultAlpha = static_cast<uint8_t>(resultAlpha * (1.0f - alphaFactor));

            alpha[index] = resultAlpha;
        }
        });

    image.Replace(0, 0, 0, red, green, blue);

    return image;
}

wxImage CCircle::Burst(const int& taille, const int& color,
    const int& intensity, const int& colorIntensity) {
    if (taille <= 0) return {};

    constexpr float y1 = 0.6f;
    constexpr float x1 = 0.5f;

    const float a = (y1 - 1.0f) / (x1 * (x1 - 1.0f));

    const float b = 1.0f - a;

    std::vector<CRgbaquad> listColor(static_cast<size_t>(taille + 1));

    for (int i = 0; i <= taille; ++i) {
        const float k = static_cast<float>(i) / static_cast<float>(taille);

        float alpha = a * k * k + b * k;

        alpha = std::max(0.0f, alpha);

        HSB value = { color, static_cast<long>(colorIntensity * alpha), 100 };

        CRgbaquad rgb;

        CColor::HSBToRGB(value, rgb);

        rgb.SetAlpha(
            static_cast<uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f)));

        listColor[i] = rgb;
    }

    wxImage image = GetCircle(taille);

    if (!image.IsOk()) return {};

    EnsureAlpha(image);

    uint8_t* alpha = image.GetAlpha();

    uint8_t* data = image.GetData();

    if (alpha == nullptr || data == nullptr) {
        return {};
    }

    const int width = image.GetWidth();

    const int height = image.GetHeight();

    const int radius = taille / 2;

    tbb::parallel_for(0, height, 1, [=, &listColor](const int y) {
        const int rowOffset = y * width;

        for (int x = 0; x < width; ++x) {
            const int index = rowOffset + x;

            const int pixel = index * RGB_CHANNELS;

            if (data[pixel] != 0 || data[pixel + 1] != 0 || data[pixel + 2] != 0) {
                continue;
            }

            const float distance = GetDistance(x, y, radius, radius);

            const int distanceIndex = static_cast<int>(distance);

            alpha[index] = 0;

            if (distance <= radius) {
                alpha[index] = GetAlphaFromDistance(distance, radius);
            }

            if (distanceIndex >= 0 &&
                distanceIndex < static_cast<int>(listColor.size())) {
                const CRgbaquad& rgb = listColor[distanceIndex];

                data[pixel] = rgb.GetRed();

                data[pixel + 1] = rgb.GetGreen();

                data[pixel + 2] = rgb.GetBlue();
            }
        }
        });

    return image;
}

wxImage CCircle::HaloGradient(const int& taille, const int& widthValue,
    const float& alphaValue) {
    if (taille <= 0 || widthValue <= 0) {
        return {};
    }

    const int radius = taille / 2;

    const int width = std::min(widthValue, radius);

    if (width <= 0) return {};

    const int number = 360 / width;

    std::vector<CRgbaquad> listColor(static_cast<size_t>(width + 1));

    for (int i = taille - width; i <= taille; ++i) {
        const int j = taille - i;

        HSB value = { number * j, 50, 100 };

        CColor::HSBToRGB(value, listColor[j]);
    }

    wxImage image = GetCircle(taille);

    if (!image.IsOk()) return {};

    EnsureAlpha(image);

    uint8_t* alpha = image.GetAlpha();

    uint8_t* data = image.GetData();

    if (alpha == nullptr || data == nullptr) {
        return {};
    }

    const int imageWidth = image.GetWidth();

    const int imageHeight = image.GetHeight();

    const float alphaFactor = std::clamp(alphaValue, 0.0f, 1.0f);

    tbb::parallel_for(0, imageHeight, 1, [=, &listColor](const int y) {
        const int rowOffset = y * imageWidth;

        for (int x = 0; x < imageWidth; ++x) {
            const int index = rowOffset + x;

            const int pixel = index * RGB_CHANNELS;

            if (data[pixel] != 0 || data[pixel + 1] != 0 || data[pixel + 2] != 0) {
                continue;
            }

            const float distance = GetDistance(x, y, radius, radius);

            const int distanceInt = static_cast<int>(distance);

            alpha[index] = 0;

            if (distanceInt <= radius && distanceInt >= radius - width) {
                const int position =
                    ClampIndex(width - (radius - distanceInt), 0, width);

                const CRgbaquad& rgb = listColor[position];

                data[pixel] = rgb.GetRed();

                data[pixel + 1] = rgb.GetGreen();

                data[pixel + 2] = rgb.GetBlue();

                alpha[index] = static_cast<uint8_t>(
                    std::clamp(255.0f * (1.0f - alphaFactor), 0.0f, 255.0f));
            }
        }
        });

    return image;
}

wxImage CCircle::Halo(const int& color, const int& colorIntensity,
    const int& taille, const int& widthValue,
    const float& alphaValue, const int& centre) {
    if (taille <= 0 || widthValue <= 0) {
        return {};
    }

    const int radius = taille / 2;

    const int width = std::min(widthValue, taille);

    wxImage image = GetCircle(taille);

    if (!image.IsOk()) return {};

    EnsureAlpha(image);

    std::vector<CRgbaquad> listColorCenter;

    std::vector<CRgbaquad> listColorOut(static_cast<size_t>(width + 1));

    const float alphaFactor = std::clamp(alphaValue, 0.0f, 1.0f);

    if (centre) {
        const int centerCount = std::max(0, taille - width);

        listColorCenter.resize(static_cast<size_t>(centerCount));

        constexpr float y1 = 0.3f;
        constexpr float x1 = 0.5f;

        const float a = (y1 - 1.0f) / (x1 * (x1 - 1.0f));

        const float b = 1.0f - a;

        for (int i = 0; i < centerCount; ++i) {
            float value = static_cast<float>(i) / static_cast<float>(taille);

            value = a * value * value + b * value;

            value = std::max(0.0f, value);

            HSB hsb = { color, static_cast<long>(value * colorIntensity), 100 };

            CColor::HSBToRGB(hsb, listColorCenter[i]);
        }
    }

    constexpr double center = 100.0;

    for (int i = taille - width; i <= taille; ++i) {
        float fAlpha;

        const int j = 100 - i;

        if (j < 0) {
            fAlpha = 1.0f;
        }
        else {
            double intensity = static_cast<double>(i) / center;

            // Protection de asin().
            intensity = std::clamp(intensity, -1.0, 1.0);

            const double m = (std::asin(intensity) * 90.0) / PI;

            intensity = std::exp(-m * m * 0.006) * 0.50 + std::exp(-m * 0.03) * 0.50;

            fAlpha = static_cast<float>(1.0 - intensity);

            fAlpha = std::clamp(fAlpha, 0.0f, 1.0f);
        }

        HSB value = { color, static_cast<long>(fAlpha * colorIntensity), 100 };

        CColor::HSBToRGB(value,
            listColorOut[static_cast<size_t>(i - (taille - width))]);
    }

    uint8_t* alpha = image.GetAlpha();

    uint8_t* data = image.GetData();

    if (alpha == nullptr || data == nullptr) {
        return {};
    }

    const int imageWidth = image.GetWidth();

    const int imageHeight = image.GetHeight();

    tbb::parallel_for(
        0, imageHeight, 1, [=, &listColorCenter, &listColorOut](const int y) {
            const int rowOffset = y * imageWidth;

            for (int x = 0; x < imageWidth; ++x) {
                const int index = rowOffset + x;

                const int pixel = index * RGB_CHANNELS;

                if (data[pixel] != 0 || data[pixel + 1] != 0 ||
                    data[pixel + 2] != 0) {
                    continue;
                }

                const float distance = GetDistance(x, y, radius, radius);

                const int distanceInt = static_cast<int>(distance);

                alpha[index] = 0;

                if (distanceInt < radius - width && centre) {
                    if (distanceInt >= 0 &&
                        distanceInt < static_cast<int>(listColorCenter.size())) {
                        const CRgbaquad& rgb = listColorCenter[distanceInt];

                        data[pixel] = rgb.GetRed();

                        data[pixel + 1] = rgb.GetGreen();

                        data[pixel + 2] = rgb.GetBlue();

                        alpha[index] =
                            static_cast<uint8_t>(255.0f * (1.0f - alphaFactor));
                    }
                }
                else if (distanceInt <= radius && distanceInt >= radius - width) {
                    const int position =
                        ClampIndex(width - (radius - distanceInt), 0, width);

                    const CRgbaquad& rgb = listColorOut[position];

                    data[pixel] = rgb.GetRed();

                    data[pixel + 1] = rgb.GetGreen();

                    data[pixel + 2] = rgb.GetBlue();

                    alpha[index] = static_cast<uint8_t>(255.0f * (1.0f - alphaFactor));
                }
                else {
                    data[pixel] = 255;
                    data[pixel + 1] = 255;
                    data[pixel + 2] = 255;
                }
            }
        });

    return image;
}