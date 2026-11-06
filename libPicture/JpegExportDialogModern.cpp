#include <header.h>
#include "JpegExportDialogModern.h"
#include <ImageLoadingFormat.h> // Inclusion requise pour accéder aux méthodes de m_imageFormat
#include <wx/dcbuffer.h>
#include <wx/graphics.h> // En-tête pour wxGraphicsContext

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

        wxString ImageSizeLabel(size_t jpegSizeInBytes)
        {
            if (jpegSizeInBytes == 0)
                return "JPEG preview unavailable";

            return wxString::Format("Preview : %.2f Ko", static_cast<double>(jpegSizeInBytes) / 1024.0);
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

    // Un seul constructeur conservé avec l'appel de notification initial
    CJpegExportDialogModern::CJpegExportDialogModern(
        wxWindow* parent, CImageLoadingFormat* bitmap,
        int initialQuality, int initialCodec, PreviewUpdateCallback previewCallback)
        : wxDialog(parent, wxID_ANY, "JPEG Export",
            wxDefaultPosition, wxSize(900, 760),
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
        m_imageFormat(bitmap),
        m_previewCallback(std::move(previewCallback))
    {
        if (!m_imageFormat)
            return;

        SetMinSize(wxSize(760, 650));
        SetBackgroundColour(m_theme.background);
        BuildUi(initialQuality, initialCodec);

        m_originalPreview->SetImage(m_imageFormat->GetwxImage());
        m_originalSizeText->SetLabel(ImageDimensions(m_imageFormat->GetWidth(), m_imageFormat->GetHeight()));

        //NotifyPreviewChanged();

        Layout();
        CentreOnParent();
    }

    CJpegExportDialogModern::CImagePreview::CImagePreview(wxWindow* parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetBackgroundColour(wxColour(245, 246, 248));
        SetMinSize(wxSize(280, 280));
        Bind(wxEVT_PAINT, &CImagePreview::OnPaint, this);
        Bind(wxEVT_ERASE_BACKGROUND, &CImagePreview::OnEraseBackground, this);
    }

    void CJpegExportDialogModern::CImagePreview::SetImage(const wxImage& image)
    {
        m_image = image;
        Refresh();
    }

    void CJpegExportDialogModern::CImagePreview::SetCaption(const wxString& caption) {}

    void CJpegExportDialogModern::CImagePreview::OnEraseBackground(wxEraseEvent&) {}

    void CJpegExportDialogModern::CImagePreview::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        const wxColour background(245, 246, 248);
        dc.SetBackground(wxBrush(background));
        dc.Clear();

        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (!gc)
            return;

        const wxSize size = GetClientSize();

        gc->SetPen(wxPen(wxColour(220, 224, 230), 1));
        gc->SetBrush(wxBrush(background));
        gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, 7);

        if (m_image.IsOk())
        {
            const int margin = 12;
            wxBitmap bitmap = FitBitmap(m_image, wxSize(size.x - margin * 2,
                size.y - margin * 2));
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

    void CJpegExportDialogModern::BuildUi(int initialQuality, int initialCodec)
    {
        auto* root = new wxBoxSizer(wxVERTICAL);

        auto* imageInfo = new wxStaticText(this, wxID_ANY, "Original image");
        imageInfo->SetForegroundColour(m_theme.secondaryText);
        root->Add(imageInfo, 0, wxLEFT | wxRIGHT | wxTOP, 20);

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

        auto* jpegColumn = new wxBoxSizer(wxVERTICAL);
        auto* jpegTitle = new wxStaticText(this, wxID_ANY, "JPEG Preview");
        jpegTitle->SetFont(jpegTitle->GetFont().Bold());
        m_jpegPreview = new CImagePreview(this);
        m_jpegSizeText = new wxStaticText(this, wxID_ANY, "");
        m_jpegSizeText->SetForegroundColour(m_theme.secondaryText);

        jpegColumn->Add(jpegTitle, 0, wxBOTTOM, 8);
        jpegColumn->Add(m_jpegPreview, 1, wxEXPAND);
        jpegColumn->Add(m_jpegSizeText, 0, wxALIGN_CENTER | wxTOP, 8);

        previews->Add(originalColumn, 1, wxEXPAND | wxRIGHT, 10);
        previews->Add(jpegColumn, 1, wxEXPAND | wxLEFT, 10);
        root->Add(previews, 1, wxEXPAND | wxALL, 20);

        auto* codecGroup = new CModernGroupBox(this, "Encoding options");
        codecGroup->SetTheme(m_theme);
        auto* codecSizer = new wxBoxSizer(wxVERTICAL);
        auto* codecGrid = new wxGridSizer(3, 3, 7, 16);

        const wxString codecLabels[] = {
            "Baseline", "Arithmetic", "Grayscale",
            "Optimize", "Progressive", "Lossless",
            "Smoothing", "Subsample 4:2:2", "Subsample 4:4:4"
        };

        for (int i = 0; i < 9; ++i)
        {
            m_codecRadios[i] = new CModernRadioButton(
                codecGroup, wxID_ANY, codecLabels[i], 1, i == initialCodec);
            m_codecRadios[i]->SetTheme(m_theme);
            codecGrid->Add(m_codecRadios[i], 0, wxEXPAND);
            m_codecRadios[i]->Bind(wxEVT_RADIOBUTTON,
                &CJpegExportDialogModern::OnCodecChanged, this);
        }

        codecSizer->Add(codecGrid, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 16);
        codecGroup->SetSizer(codecSizer);
        root->Add(codecGroup, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

        auto* qualityGroup = new CModernGroupBox(this, "JPEG Quality");
        qualityGroup->SetTheme(m_theme);
        auto* qualitySizer = new wxBoxSizer(wxVERTICAL);

        m_qualityText = new wxStaticText(qualityGroup, wxID_ANY, "");
        m_qualityText->SetForegroundColour(m_theme.secondaryText);

        m_qualitySlider = new CModernSlider(
            qualityGroup, wxID_ANY, std::clamp(initialQuality, 1, 100), 1, 100);
        m_qualitySlider->SetTheme(m_theme);
        m_qualitySlider->Bind(wxEVT_SLIDER,
            &CJpegExportDialogModern::OnQualityChanged, this);

        qualitySizer->Add(m_qualityText, 0, wxEXPAND | wxBOTTOM, 8);
        qualitySizer->Add(m_qualitySlider, 0, wxEXPAND);
        qualityGroup->SetSizer(qualitySizer);
        root->Add(qualityGroup, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

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
        m_exportButton->Bind(wxEVT_BUTTON, &CJpegExportDialogModern::OnExport, this);

        UpdateQualityText();
    }

    int CJpegExportDialogModern::GetQuality() const
    {
        return m_qualitySlider ? m_qualitySlider->GetValue() : 90;
    }

    int CJpegExportDialogModern::GetCodecOption() const
    {
        for (int i = 0; i < static_cast<int>(m_codecRadios.size()); ++i)
        {
            if (m_codecRadios[i] && m_codecRadios[i]->GetValue())
                return i;
        }
        return Baseline;
    }

    void CJpegExportDialogModern::SetJpegPreview(const wxImage& image, size_t jpegSize)
    {
        if (m_jpegPreview)
            m_jpegPreview->SetImage(image);

        if (m_jpegSizeText)
            m_jpegSizeText->SetLabel(ImageSizeLabel(jpegSize));
    }

    void CJpegExportDialogModern::OnQualityChanged(wxCommandEvent&)
    {
        UpdateQualityText();
        NotifyPreviewChanged();
    }

    void CJpegExportDialogModern::OnCodecChanged(wxCommandEvent&)
    {
        NotifyPreviewChanged();
    }

    void CJpegExportDialogModern::OnExport(wxCommandEvent&)
    {
        EndModal(wxID_OK);
    }
    void CJpegExportDialogModern::UpdateQualityText()
    {
        if (m_qualityText)
            m_qualityText->SetLabel(wxString::Format("Quality: %d %%", GetQuality()));
    }
    void CJpegExportDialogModern::NotifyPreviewChanged()
    {
        if (m_previewCallback)
            m_previewCallback(GetQuality(), GetCodecOption());
		this->Refresh(); // Rafraîchit le dialogue pour refléter les changements
    }
}
