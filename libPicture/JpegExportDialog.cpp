#include <header.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>
#include <wx/sizer.h>
#include <wx/xrc/xmlres.h>
#include <ImageLoadingFormat.h>
#include <algorithm>
#include <vector>

#include "JpegExportDialog.h"
#include <libPicture.h>
#include <ximage.h>
#include <ximajpg.h>

using namespace Regards::Picture;

namespace
{
    // Marge (en pixels) autour du wxStaticBitmap dans son panneau.
    // Doit rester identique à la valeur passée au sizer.
    constexpr int PREVIEW_BORDER = 2;
}

CJpegExportDialog::CJpegExportDialog(CImageLoadingFormat* bitmap)
{
    if (bitmap == nullptr)
        return;

    m_image = bitmap;

    if (!CreateControlsFromXrc()) return;

    GenerateJpegPreview();
    UpdatePreview();

    CentreOnParent();
}

// -------------------------------------------------------------
// Chargement XRC
// -------------------------------------------------------------
bool CJpegExportDialog::CreateControlsFromXrc() {
    if (!wxXmlResource::Get()->LoadDialog(this, GetParent(),
        "JpegExportDialog")) {
        wxLogError("Unable to load JpegExportDialog.xrc");
        return false;
    }

    m_originalPreviewPanel = XRCCTRL(*this, "originalPreviewPanel", wxPanel);
    m_jpegPreviewPanel = XRCCTRL(*this, "jpegPreviewPanel", wxPanel);
    m_imageInfo = XRCCTRL(*this, "imageInfo", wxStaticText);
    m_originalSizeLabel = XRCCTRL(*this, "originalSize", wxStaticText);
    m_jpegSizeLabel = XRCCTRL(*this, "jpegSize", wxStaticText);
    m_qualityLabel = XRCCTRL(*this, "qualityLabel", wxStaticText);
    m_qualitySlider = XRCCTRL(*this, "qualitySlider", wxSlider);
    m_codecOptions = XRCCTRL(*this, "ID_RBCODEC", wxRadioBox);

    // FIX TECHNIQUE : Attention, dans votre code d'origine, m_filePath n'est plus déclaré 
    // dans le fichier .h mais était resté dans cette vérification de pointeurs. 
    // Retrait de la vérification de m_filePath pour éviter un crash au chargement.
    if (!m_originalPreviewPanel || !m_jpegPreviewPanel || !m_qualitySlider || !m_codecOptions) {
        wxLogError("Controls XRC missing");
        return false;
    }

    // Les panneaux ne doivent pas être contraints par la taille du bitmap
    // qu'ils contiennent (sinon ils ne peuvent plus rétrécir et le layout
    // oscille à chaque mise à jour).
    m_originalPreviewPanel->SetMinSize(wxSize(50, 50));
    m_jpegPreviewPanel->SetMinSize(wxSize(50, 50));

    // Création des contrôles bitmap dans les panneaux XRC.
    m_originalPreview = new wxStaticBitmap(m_originalPreviewPanel, wxID_ANY, wxBitmap(1, 1));
    m_compressedPreview = new wxStaticBitmap(m_jpegPreviewPanel, wxID_ANY, wxBitmap(1, 1));

    // CORRECTION : plus de wxEXPAND. Un wxStaticBitmap étendu dessine son bitmap
    // en haut à gauche de son rectangle ; avec wxALIGN_CENTER, c'est le sizer
    // qui centre le contrôle (qui a la taille exacte du bitmap).
    auto* originalSizer = new wxBoxSizer(wxVERTICAL);
    originalSizer->Add(m_originalPreview, 1, wxALIGN_CENTER | wxALL, PREVIEW_BORDER);
    m_originalPreviewPanel->SetSizer(originalSizer);

    auto* jpegSizer = new wxBoxSizer(wxVERTICAL);
    jpegSizer->Add(m_compressedPreview, 1, wxALIGN_CENTER | wxALL, PREVIEW_BORDER);
    m_jpegPreviewPanel->SetSizer(jpegSizer);

    m_imageInfo->SetLabel(wxString::Format("Original Image : %d x %d pixels",
        m_image->GetWidth(), m_image->GetHeight()));

    m_qualitySlider->SetValue(JPEG_DEFAULT_QUALITY);
    UpdateQualityLabel();

    // -------------------------------------------------------------
    // LIAISON DES ÉVÉNEMENTS
    // -------------------------------------------------------------

    // Événement du Slider (Qualité)
    m_qualitySlider->Bind(wxEVT_SLIDER, &CJpegExportDialog::OnQualityChanged, this);

    // AJOUT : Événement du RadioBox (Changement d'algorithme de compression)
    m_codecOptions->Bind(wxEVT_RADIOBOX, &CJpegExportDialog::OnQualityChanged, this);

    XRCCTRL(*this, "wxID_OK", wxButton)
        ->Bind(wxEVT_BUTTON, &CJpegExportDialog::OnOk, this);

    XRCCTRL(*this, "wxID_CANCEL", wxButton)
        ->Bind(wxEVT_BUTTON, &CJpegExportDialog::OnCancel, this);

    Bind(wxEVT_SIZE, &CJpegExportDialog::OnResize, this);

    return true;
}



void CJpegExportDialog::OnOk(wxCommandEvent& event)
{
    EndModal(wxID_OK);
}

void CJpegExportDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}


// -------------------------------------------------------------
// Génération JPEG en mémoire
// (ne touche PAS à l'affichage : c'est le rôle de UpdatePreview)
// -------------------------------------------------------------

void CJpegExportDialog::GenerateJpegPreview()
{

    CxImage cxImg = m_image->GetCxImage();

    const int quality = std::clamp(m_qualitySlider->GetValue(), 0, 100);
    cxImg.SetJpegQuality(static_cast<BYTE>(quality));

    const int codecOption = m_codecOptions->GetSelection();

    // Application des drapeaux de codec d'après les spécifications de ximajpg.h
    switch (codecOption) {
    case 0: // Baseline
        cxImg.SetCodecOption(CxImageJPG::ENCODE_BASELINE, CXIMAGE_FORMAT_JPG);
        break;

    case 1: // Arithmetic
        cxImg.SetCodecOption(CxImageJPG::ENCODE_ARITHMETIC, CXIMAGE_FORMAT_JPG);
        break;

    case 2: // Grayscale
        cxImg.SetCodecOption(CxImageJPG::ENCODE_GRAYSCALE, CXIMAGE_FORMAT_JPG);
        // Note: CxImageJPG gère ENCODE_GRAYSCALE en interne, mais vous pouvez aussi appeler cxImg.GrayScale();
        break;

    case 3: // Optimize
        cxImg.SetCodecOption(CxImageJPG::ENCODE_OPTIMIZE, CXIMAGE_FORMAT_JPG);
        break;

    case 4: // Progressive
        cxImg.SetCodecOption(CxImageJPG::ENCODE_PROGRESSIVE, CXIMAGE_FORMAT_JPG);
        break;

    case 5: // Lossless
        cxImg.SetCodecOption(CxImageJPG::ENCODE_LOSSLESS, CXIMAGE_FORMAT_JPG);
        break;

    case 6: // Smoothing
        cxImg.SetCodecOption(CxImageJPG::ENCODE_SMOOTHING, CXIMAGE_FORMAT_JPG);
        break;

    case 7: // Subsample 422
        cxImg.SetCodecOption(CxImageJPG::ENCODE_SUBSAMPLE_422, CXIMAGE_FORMAT_JPG);
        break;

    case 8: // Subsample 444
        cxImg.SetCodecOption(CxImageJPG::ENCODE_SUBSAMPLE_444, CXIMAGE_FORMAT_JPG);
        break;

    default:
        break;
    }

    // Encodage mémoire
    BYTE* buffer = nullptr;
    int32_t bufferSize = 0;

    if (!cxImg.Encode(buffer, bufferSize, CXIMAGE_FORMAT_JPG)) return;

    m_jpegSize = static_cast<size_t>(bufferSize);

    // Décodage pour la prévisualisation
    CxImage decodedImg;
    if (decodedImg.Decode(buffer, bufferSize, CXIMAGE_FORMAT_JPG)) {

        const int w = decodedImg.GetWidth();
        const int h = decodedImg.GetHeight();
        const int bpp = decodedImg.GetBpp();

        // Un JPEG encodé en niveaux de gris (option GrayScale) est décodé par
        // CxImage en 8 bits (1 octet/pixel). L'ancienne boucle lisait 3 octets
        // par pixel => lecture hors limites. Dans ce cas, on copie la valeur
        // de gris 3 fois (R = G = B).
        if (bpp == 8 || bpp == 24) {
            m_jpegPreview.Create(w, h, false);
            unsigned char* dstRgb = m_jpegPreview.GetData();

            for (int y = 0; y < h; ++y) {
                unsigned char* srcLine = decodedImg.GetBits(h - 1 - y);
                unsigned char* dstLine = &dstRgb[y * w * 3];

                if (bpp == 8) {
                    for (int x = 0; x < w; ++x) {
                        const unsigned char gray = srcLine[x];
                        dstLine[x * 3 + 0] = gray; // R
                        dstLine[x * 3 + 1] = gray; // G
                        dstLine[x * 3 + 2] = gray; // B
                    }
                }
                else {
                    for (int x = 0; x < w; ++x) {
                        dstLine[x * 3 + 0] = srcLine[x * 3 + 2]; // R
                        dstLine[x * 3 + 1] = srcLine[x * 3 + 1]; // G
                        dstLine[x * 3 + 2] = srcLine[x * 3 + 0]; // B
                    }
                }
            }
        }
    }

    cxImg.FreeMemory(buffer);

    // CORRECTION : suppression de l'ancien bloc "adaptation + SetBitmap +
    // Refresh/Update" qui était ici. Il affichait l'image une première fois
    // (calculée sur la taille du wxStaticBitmap, pas du panneau), puis
    // UpdatePreview() l'affichait une seconde fois avec une autre taille
    // => déplacement visible de l'image. L'affichage est désormais fait
    // uniquement dans UpdatePreview().
}

// -------------------------------------------------------------
// Prévisualisation
// -------------------------------------------------------------

void CJpegExportDialog::UpdatePreview() {
    if (!m_originalPreview || !m_compressedPreview) return;

    auto resizeImage = [](const wxImage& source, wxWindow* target) -> wxBitmap {
        if (!source.IsOk()) return wxBitmap(1, 1);

        // On retire la marge du sizer, sinon bitmap + marge > panneau.
        wxSize size = target->GetClientSize();
        size.x -= 2 * PREVIEW_BORDER;
        size.y -= 2 * PREVIEW_BORDER;

        if (size.x <= 0 || size.y <= 0) return wxBitmap(1, 1);

        const double scale =
            std::min(static_cast<double>(size.x) / source.GetWidth(),
                static_cast<double>(size.y) / source.GetHeight());

        const int width = std::max(1, static_cast<int>(source.GetWidth() * scale));

        const int height =
            std::max(1, static_cast<int>(source.GetHeight() * scale));

        wxImage resized = source.Copy();

        resized.Rescale(width, height, wxIMAGE_QUALITY_HIGH);

        return wxBitmap(resized);
        };

    // Freeze/Thaw : un seul repaint pour les deux panneaux (pas d'état intermédiaire visible)
    Freeze();

    m_originalPreview->SetBitmap(resizeImage(m_image->GetwxImage(), m_originalPreviewPanel));

    m_compressedPreview->SetBitmap(
        resizeImage(m_jpegPreview, m_jpegPreviewPanel));

    m_originalSizeLabel->SetLabel(wxString::Format(
        "%d x %d pixels", m_image->GetWidth(), m_image->GetHeight()));

    m_jpegSizeLabel->SetLabel(wxString::Format(
        "JPEG : %.2f Ko", static_cast<double>(m_jpegSize) / 1024.0));

    // Le sizer qui centre le bitmap est celui des panneaux : c'est eux qu'il faut relayouter.
    m_originalPreviewPanel->Layout();
    m_jpegPreviewPanel->Layout();

    Thaw();
}

// -------------------------------------------------------------
// Qualité
// -------------------------------------------------------------

void CJpegExportDialog::UpdateQualityLabel() {
    m_qualityLabel->SetLabel(
        wxString::Format("Quality : %d %%", m_qualitySlider->GetValue()));
}

void CJpegExportDialog::OnQualityChanged(wxCommandEvent& event) {
    UpdateQualityLabel();

    GenerateJpegPreview();
    UpdatePreview();

    event.Skip();
}



// -------------------------------------------------------------
// Resize
// -------------------------------------------------------------

void CJpegExportDialog::OnResize(wxSizeEvent& event) {
    UpdatePreview();

    event.Skip();
}

int CJpegExportDialog::GetQuality() const {
    return m_qualitySlider ? m_qualitySlider->GetValue() : JPEG_DEFAULT_QUALITY;
}