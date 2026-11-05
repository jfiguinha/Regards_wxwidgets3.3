#include <header.h>
#include "AvifHeifExportDialog.h"
#include <wx/xrc/xmlres.h>
#include <wx/sizer.h>
#include <ImageLoadingFormat.h>
#include <algorithm>

// Headers libheif
#include <libheif/heif.h>

namespace
{
    constexpr int PREVIEW_BORDER = 2;
}

CAvifHeifExportDialog::CAvifHeifExportDialog(CImageLoadingFormat* bitmap, int formatChoice)
{
    m_formatChoice = formatChoice;

    if (bitmap == nullptr)
        return;

    m_image = bitmap;

    if (!CreateControlsFromXrc())
        return;

    GeneratePreview();
    UpdatePreview();
    CentreOnParent();
}

bool CAvifHeifExportDialog::CreateControlsFromXrc()
{
    if (!wxXmlResource::Get()->LoadDialog(this, GetParent(), "AvifHeifExportDialog")) {
        wxLogError("Unable to load AvifHeifExportDialog.xrc");
        return false;
    }

    m_originalPreviewPanel = XRCCTRL(*this, "originalPreviewPanel", wxPanel);
    m_compressedPreviewPanel = XRCCTRL(*this, "compressedPreviewPanel", wxPanel);
    m_imageInfo = XRCCTRL(*this, "imageInfo", wxStaticText);
    m_originalSizeLabel = XRCCTRL(*this, "originalSize", wxStaticText);
    m_compressedSizeLabel = XRCCTRL(*this, "compressedSize", wxStaticText);
    m_qualityLabel = XRCCTRL(*this, "qualityLabel", wxStaticText);
    m_speedLabel = XRCCTRL(*this, "speedLabel", wxStaticText);

    m_qualitySlider = XRCCTRL(*this, "qualitySlider", wxSlider);
    m_speedSlider = XRCCTRL(*this, "speedSlider", wxSlider);

    if (!m_originalPreviewPanel || !m_compressedPreviewPanel || !m_qualitySlider || !m_speedSlider) {
        wxLogError("Controls XRC missing");
        return false;
    }

    m_originalPreviewPanel->SetMinSize(wxSize(50, 50));
    m_compressedPreviewPanel->SetMinSize(wxSize(50, 50));

    m_originalPreview = new wxStaticBitmap(m_originalPreviewPanel, wxID_ANY, wxBitmap(1, 1));
    m_compressedPreview = new wxStaticBitmap(m_compressedPreviewPanel, wxID_ANY, wxBitmap(1, 1));

    auto* originalSizer = new wxBoxSizer(wxVERTICAL);
    originalSizer->Add(m_originalPreview, 1, wxALIGN_CENTER | wxALL, PREVIEW_BORDER);
    m_originalPreviewPanel->SetSizer(originalSizer);

    auto* compressedSizer = new wxBoxSizer(wxVERTICAL);
    compressedSizer->Add(m_compressedPreview, 1, wxALIGN_CENTER | wxALL, PREVIEW_BORDER);
    m_compressedPreviewPanel->SetSizer(compressedSizer);

    m_imageInfo->SetLabel(wxString::Format("Original Image : %d x %d pixels", m_image->GetWidth(), m_image->GetHeight()));

    m_qualitySlider->SetValue(DEFAULT_QUALITY);
    m_speedSlider->SetValue(DEFAULT_SPEED);
    UpdateLabels();

    // Événements
    m_qualitySlider->Bind(wxEVT_SLIDER, &CAvifHeifExportDialog::OnSettingsChanged, this);
    m_speedSlider->Bind(wxEVT_SLIDER, &CAvifHeifExportDialog::OnSettingsChanged, this);

    XRCCTRL(*this, "wxID_OK", wxButton)->Bind(wxEVT_BUTTON, &CAvifHeifExportDialog::OnOk, this);
    XRCCTRL(*this, "wxID_CANCEL", wxButton)->Bind(wxEVT_BUTTON, &CAvifHeifExportDialog::OnCancel, this);
    Bind(wxEVT_SIZE, &CAvifHeifExportDialog::OnResize, this);

    return true;
}

void CAvifHeifExportDialog::GeneratePreview()
{
    // 1. Récupération de l'image source brute au format wxImage
    wxImage srcImg = m_image->GetwxImage();
    if (!srcImg.IsOk()) return;

    int width = srcImg.GetWidth();
    int height = srcImg.GetHeight();

    // 2. Conversion wxImage -> heif_image
    heif_image* hImg = nullptr;
    heif_image_create(width, height, heif_colorspace_RGB, heif_chroma_interleaved_24bit, &hImg);
    heif_image_add_plane(hImg, heif_channel_interleaved, width, height, 24);

    int stride = 0;
    uint8_t* destData = heif_image_get_plane(hImg, heif_channel_interleaved, &stride);
    const unsigned char* srcData = srcImg.GetData();

    // Copie ligne par ligne pour respecter le stride potentiel de la libheif
    for (int y = 0; y < height; ++y) {
        memcpy(destData + (y * stride), srcData + (y * width * 3), width * 3);
    }

    // 3. Configuration de l'encodeur libheif
    heif_context* ctx = heif_context_alloc();
    heif_encoder* encoder = nullptr;

    if (m_formatChoice == 0) {
        heif_context_get_encoder_for_format(ctx, heif_compression_AV1, &encoder);
    }
    else {
        heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);
    }

    if (encoder) {
        // Application de la qualité (0 - 100)
        heif_encoder_set_lossy_quality(encoder, m_qualitySlider->GetValue());

        // Application de l'effort / vitesse ("0" à "10")
        wxString speedStr = wxString::Format("%d", m_speedSlider->GetValue());
        heif_encoder_set_parameter_string(encoder, "speed", speedStr.utf8_str());

        // Encodage effectif
        heif_context_encode_image(ctx, hImg, encoder, nullptr, nullptr);
        heif_encoder_release(encoder);
    }

    // 4. Récupération du flux compressé en mémoire pour en mesurer la taille
    heif_writer writer;
    std::vector<uint8_t> compressedBuffer;

    writer.writer_api_version = 1;
    writer.write = [](heif_context* ctx, const void* data, size_t size, void* ref) -> heif_error {
        auto* buf = static_cast<std::vector<uint8_t>*>(ref);
        buf->insert(buf->end(), static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
        heif_error error = { heif_error_Ok, heif_suberror_Unspecified, "Success" };
        return error;
        };

    heif_context_write(ctx, &writer, &compressedBuffer);
    m_compressedSize = compressedBuffer.size();

    // 5. Décompression immédiate du flux pour générer la texture de prévisualisation
    heif_context* decodeCtx = heif_context_alloc();
    heif_context_read_from_memory_without_copy(decodeCtx, compressedBuffer.data(), compressedBuffer.size(), nullptr);

    heif_image_handle* handle = nullptr;
    heif_context_get_primary_image_handle(decodeCtx, &handle);

    if (handle) {
        heif_image* decodedImg = nullptr;
        heif_decode_image(handle, &decodedImg, heif_colorspace_RGB, heif_chroma_interleaved_24bit, nullptr);

        if (decodedImg) {
            int decStride = 0;
            uint8_t* decData = heif_image_get_plane(decodedImg, heif_channel_interleaved, &decStride);

            m_compressedPreviewImage.Create(width, height, false);
            unsigned char* previewDest = m_compressedPreviewImage.GetData();

            for (int y = 0; y < height; ++y) {
                memcpy(previewDest + (y * width * 3), decData + (y * decStride), width * 3);
            }
            heif_image_release(decodedImg);
        }
        heif_image_handle_release(handle);
    }

    // Nettoyage
    heif_context_free(decodeCtx);
    heif_image_release(hImg);
    heif_context_free(ctx);
}

void CAvifHeifExportDialog::UpdatePreview()
{
    if (!m_originalPreview || !m_compressedPreview) return;

    auto resizeImage = [](const wxImage& source, wxWindow* target) -> wxBitmap {
        if (!source.IsOk()) return wxBitmap(1, 1);

        wxSize size = target->GetClientSize();
        size.x -= 2 * PREVIEW_BORDER;
        size.y -= 2 * PREVIEW_BORDER;

        if (size.x <= 0 || size.y <= 0) return wxBitmap(1, 1);

        const double scale = std::min(static_cast<double>(size.x) / source.GetWidth(),
            static_cast<double>(size.y) / source.GetHeight());

        const int width = std::max(1, static_cast<int>(source.GetWidth() * scale));
        const int height = std::max(1, static_cast<int>(source.GetHeight() * scale));

        wxImage resized = source.Copy();
        resized.Rescale(width, height, wxIMAGE_QUALITY_HIGH);

        return wxBitmap(resized);
        };

    Freeze();

    m_originalPreview->SetBitmap(resizeImage(m_image->GetwxImage(), m_originalPreviewPanel));
    m_compressedPreview->SetBitmap(resizeImage(m_compressedPreviewImage, m_compressedPreviewPanel));

    m_originalSizeLabel->SetLabel(wxString::Format("%d x %d pixels", m_image->GetWidth(), m_image->GetHeight()));

    wxString formatName = (m_formatChoice == 0) ? "AVIF" : "HEIF";
    m_compressedSizeLabel->SetLabel(wxString::Format("%s : %.2f Ko", formatName, static_cast<double>(m_compressedSize) / 1024.0));

    m_originalPreviewPanel->Layout();
    m_compressedPreviewPanel->Layout();

    Thaw();
}

void CAvifHeifExportDialog::UpdateLabels()
{
    m_qualityLabel->SetLabel(wxString::Format("Quality : %d %%", m_qualitySlider->GetValue()));
    m_speedLabel->SetLabel(wxString::Format("Encoding Effort (Speed) : %d", m_speedSlider->GetValue()));
}

void CAvifHeifExportDialog::OnSettingsChanged(wxCommandEvent& event)
{
    UpdateLabels();
    GeneratePreview();
    UpdatePreview();
    event.Skip();
}

void CAvifHeifExportDialog::OnOk(wxCommandEvent& event) { m_isOk = true; EndModal(wxID_OK); }
void CAvifHeifExportDialog::OnCancel(wxCommandEvent& event) { m_isOk = false; EndModal(wxID_CANCEL); }
void CAvifHeifExportDialog::OnResize(wxSizeEvent& event) { UpdatePreview(); event.Skip(); }

int CAvifHeifExportDialog::GetQuality() const { return m_qualitySlider->GetValue(); }
int CAvifHeifExportDialog::GetSpeed() const { return m_speedSlider->GetValue(); }

