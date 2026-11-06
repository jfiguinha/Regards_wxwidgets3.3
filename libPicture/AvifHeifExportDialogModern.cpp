#include <header.h>
#include "AvifHeifExportDialogModern.h"
#include <ImageLoadingFormat.h>

namespace Regards::UI
{
    namespace
    {
        wxString ImageDimensions(int width, int height)
        {
            if (width <= 0 || height <= 0)
                return "No image";

            return wxString::Format("%d x %d pixels", width, height);
        }

        wxString ImageSizeLabel(size_t sizeInBytes, int formatChoice)
        {
            if (sizeInBytes == 0)
                return "Preview unavailable";

            wxString formatName = (formatChoice == 0) ? "AVIF" : "HEIF";
            return wxString::Format("%s Preview : %.2f Ko", formatName, static_cast<double>(sizeInBytes) / 1024.0);
        }

        wxBitmap FitBitmap(const wxImage& image, const wxSize& available)
        {
            if (!image.IsOk() || available.x <= 0 || available.y <= 0)
                return wxBitmap();

            const double scale = std::min(
                static_cast<double>(available.x) / image.GetWidth(),
                static_cast<double>(available.y) / image.GetHeight());

            const int width = std::max(1, static_cast<int>(image.GetWidth() * scale));
            const int height = std::max(1, static_cast<int>(image.GetHeight() * scale));

            wxImage scaled = image.Scale(width, height, wxIMAGE_QUALITY_HIGH);
            return wxBitmap(scaled);
        }
    }

    CAvifHeifExportDialogModern::CAvifHeifExportDialogModern(
        wxWindow* parent, CImageLoadingFormat* bitmap, int formatChoice,
        int initialQuality, int initialSpeed, PreviewUpdateCallback previewCallback)
        : wxDialog(parent, wxID_ANY, (formatChoice == 0) ? "AVIF Export" : "HEIF Export",
            wxDefaultPosition, wxSize(900, 760),
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
        m_formatChoice(formatChoice),
        m_imageFormat(bitmap),
        m_previewCallback(std::move(previewCallback))
    {
        if (!m_imageFormat)
            return;

        SetMinSize(wxSize(760, 650));
        SetBackgroundColour(m_theme.background);
        BuildUi(initialQuality, initialSpeed);

        m_originalPreview->SetImage(m_imageFormat->GetwxImage());
        m_originalSizeText->SetLabel(ImageDimensions(m_imageFormat->GetWidth(), m_imageFormat->GetHeight()));

       // NotifyPreviewChanged();

        Layout();
        CentreOnParent();
    }

    CAvifHeifExportDialogModern::CImagePreview::CImagePreview(wxWindow* parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetBackgroundColour(wxColour(245, 246, 248));
        SetMinSize(wxSize(280, 280));
        Bind(wxEVT_PAINT, &CImagePreview::OnPaint, this);
        Bind(wxEVT_ERASE_BACKGROUND, &CImagePreview::OnEraseBackground, this);
    }

    void CAvifHeifExportDialogModern::CImagePreview::SetImage(const wxImage& image)
    {
        m_image = image;
        Refresh();
    }

    void CAvifHeifExportDialogModern::CImagePreview::OnEraseBackground(wxEraseEvent&) {}

    void CAvifHeifExportDialogModern::CImagePreview::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        const wxColour background(245, 246, 248);
        dc.SetBackground(wxBrush(background));
        dc.Clear();

        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (!gc) return;

        const wxSize size = GetClientSize();
        gc->SetPen(wxPen(wxColour(220, 224, 230), 1));
        gc->SetBrush(wxBrush(background));
        gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, 7);

        if (m_image.IsOk())
        {
            const int margin = 12;
            wxBitmap bitmap = FitBitmap(m_image, wxSize(size.x - margin * 2, size.y - margin * 2));
            if (bitmap.IsOk())
            {
                const int x = (size.x - bitmap.GetWidth()) / 2;
                const int y = (size.y - bitmap.GetHeight()) / 2;
                gc->DrawBitmap(bitmap, x, y, bitmap.GetWidth(), bitmap.GetHeight());
            }
        }
        else
        {
            const wxString emptyText = "Preview unavailable";
            gc->SetFont(GetFont(), wxColour(130, 136, 145));
            wxDouble tw = 0, th = 0;
            gc->GetTextExtent(emptyText, &tw, &th);
            gc->DrawText(emptyText, (size.x - tw) / 2.0, (size.y - th) / 2.0);
        }
        delete gc;
    }

    void CAvifHeifExportDialogModern::BuildUi(int initialQuality, int initialSpeed)
    {
        auto* root = new wxBoxSizer(wxVERTICAL);

        auto* imageInfo = new wxStaticText(this, wxID_ANY, "Original image");
        imageInfo->SetForegroundColour(m_theme.secondaryText);
        root->Add(imageInfo, 0, wxLEFT | wxRIGHT | wxTOP, 20);

        // Zone des Previews
        auto* previews = new wxBoxSizer(wxHORIZONTAL);

        auto* originalColumn = new wxBoxSizer(wxVERTICAL);
        auto* originalTitle = new wxStaticText(this, wxID_ANY, "Original Picture");
        originalTitle->SetFont(originalTitle->GetFont().Bold());
        m_originalPreview = new CImagePreview(this);
        m_originalSizeText = new wxStaticText(this, wxID_ANY, "");
        m_originalSizeText->SetForegroundColour(m_theme.secondaryText);

        originalColumn->Add(originalTitle, 0, wxBOTTOM, 8);
        originalColumn->Add(m_originalPreview, 1, wxEXPAND);
        originalColumn->Add(m_originalSizeText, 0, wxALIGN_CENTER | wxTOP, 8);

        auto* compressedColumn = new wxBoxSizer(wxVERTICAL);
        wxString compressedTitleText = (m_formatChoice == 0) ? "AVIF Preview" : "HEIF Preview";
        auto* compressedTitle = new wxStaticText(this, wxID_ANY, compressedTitleText);
        compressedTitle->SetFont(compressedTitle->GetFont().Bold());
        m_compressedPreview = new CImagePreview(this);
        m_compressedSizeText = new wxStaticText(this, wxID_ANY, "");
        m_compressedSizeText->SetForegroundColour(m_theme.secondaryText);

        compressedColumn->Add(compressedTitle, 0, wxBOTTOM, 8);
        compressedColumn->Add(m_compressedPreview, 1, wxEXPAND);
        compressedColumn->Add(m_compressedSizeText, 0, wxALIGN_CENTER | wxTOP, 8);

        previews->Add(originalColumn, 1, wxEXPAND | wxRIGHT, 10);
        previews->Add(compressedColumn, 1, wxEXPAND | wxLEFT, 10);
        root->Add(previews, 1, wxEXPAND | wxALL, 20);

        // Groupe Qualité (CModernGroupBox)
        auto* qualityGroup = new CModernGroupBox(this, "Compression Quality");
        qualityGroup->SetTheme(m_theme);
        auto* qualitySizer = new wxBoxSizer(wxVERTICAL);

        m_qualityText = new wxStaticText(qualityGroup, wxID_ANY, "");
        m_qualityText->SetForegroundColour(m_theme.secondaryText);

        m_qualitySlider = new CModernSlider(qualityGroup, wxID_ANY, std::clamp(initialQuality, 0, 100), 0, 100);
        m_qualitySlider->SetTheme(m_theme);
        m_qualitySlider->Bind(wxEVT_SLIDER, &CAvifHeifExportDialogModern::OnSettingsChanged, this);

        qualitySizer->Add(m_qualityText, 0, wxEXPAND | wxBOTTOM, 8);
        qualitySizer->Add(m_qualitySlider, 0, wxEXPAND);
        qualityGroup->SetSizer(qualitySizer);
        root->Add(qualityGroup, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

        // Groupe Effort / Vitesse (CModernGroupBox)
        auto* speedGroup = new CModernGroupBox(this, "Encoding Effort (Speed)");
        speedGroup->SetTheme(m_theme);
        auto* speedSizer = new wxBoxSizer(wxVERTICAL);

        m_speedText = new wxStaticText(speedGroup, wxID_ANY, "");
        m_speedText->SetForegroundColour(m_theme.secondaryText);

        m_speedSlider = new CModernSlider(speedGroup, wxID_ANY, std::clamp(initialSpeed, 0, 10), 0, 10);
        m_speedSlider->SetTheme(m_theme);
        m_speedSlider->Bind(wxEVT_SLIDER, &CAvifHeifExportDialogModern::OnSettingsChanged, this);

        speedSizer->Add(m_speedText, 0, wxEXPAND | wxBOTTOM, 8);
        speedSizer->Add(m_speedSlider, 0, wxEXPAND);
        speedGroup->SetSizer(speedSizer);
        root->Add(speedGroup, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

        // Boutons d'actions
        auto* buttons = new wxBoxSizer(wxHORIZONTAL);
        buttons->AddStretchSpacer();

        m_cancelButton = new CModernButton(this, wxID_CANCEL, "Cancel");
        m_cancelButton->SetTheme(m_theme);
        m_exportButton = new CModernButton(this, wxID_OK, "Export");
        m_exportButton->SetTheme(m_theme);
        m_exportButton->SetAccent(true);

        buttons->Add(m_cancelButton, 0, wxRIGHT, 10);
        buttons->Add(m_exportButton, 0);
        root->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

        SetSizer(root);

        m_cancelButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
        m_exportButton->Bind(wxEVT_BUTTON, &CAvifHeifExportDialogModern::OnExport, this);

        UpdateLabelsText();
    }

    int CAvifHeifExportDialogModern::GetQuality() const
    {
        return m_qualitySlider ? m_qualitySlider->GetValue() : 75;
    }

    int CAvifHeifExportDialogModern::GetSpeed() const
    {
        return m_speedSlider ? m_speedSlider->GetValue() : 6;
    }

    void CAvifHeifExportDialogModern::SetCompressedPreview(const wxImage& image, size_t compressedSize)
    {
        if (m_compressedPreview)
            m_compressedPreview->SetImage(image);

        if (m_compressedSizeText)
            m_compressedSizeText->SetLabel(ImageSizeLabel(compressedSize, m_formatChoice));
    }

    void CAvifHeifExportDialogModern::OnSettingsChanged(wxCommandEvent&)
    {
        UpdateLabelsText();
        NotifyPreviewChanged();
    }

    void CAvifHeifExportDialogModern::OnExport(wxCommandEvent&)
    {
        EndModal(wxID_OK);
    }

    void CAvifHeifExportDialogModern::UpdateLabelsText()
    {
        if (m_qualityText)
            m_qualityText->SetLabel(wxString::Format("Quality: %d %%", GetQuality()));
        if (m_speedText)
            m_speedText->SetLabel(wxString::Format("Encoding Effort (Speed): %d", GetSpeed()));
    }

    void CAvifHeifExportDialogModern::NotifyPreviewChanged()
    {
        if (m_previewCallback)
            m_previewCallback(GetQuality(), GetSpeed(), GetFormatChoice());
        this->Refresh();
    }
}