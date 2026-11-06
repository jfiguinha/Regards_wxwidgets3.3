#pragma once

#include <RegardsModernControls.h>

// On déclare la classe externe sans inclure tout son fichier d'en-tête ici
class CImageLoadingFormat;

namespace Regards::UI
{
    // Dialogue moderne d'exportation JPEG interfaçé avec CImageLoadingFormat
    class CJpegExportDialogModern final : public wxDialog
    {
    public:
        enum CodecOption
        {
            Baseline = 0,
            Arithmetic,
            Grayscale,
            Optimize,
            Progressive,
            Lossless,
            Smoothing,
            Subsample422,
            Subsample444
        };

        // Callback utilisé pour notifier la logique métier qu'un recalcul de la preview JPEG est nécessaire
        using PreviewUpdateCallback = std::function<void(int quality, int codec)>;

        // Remplacement des paramètres wxImage par le pointeur CImageLoadingFormat*
        CJpegExportDialogModern(wxWindow* parent,
            CImageLoadingFormat* bitmap,
            int initialQuality = 90,
            int initialCodec = Baseline,
            PreviewUpdateCallback previewCallback = {});

        int GetQuality() const;
        int GetCodecOption() const;
        void NotifyPreviewChanged();
        // Permet de mettre à jour la prévisualisation JPEG calculée et sa taille associée
        void SetJpegPreview(const wxImage& image, size_t jpegSize);

    private:
        class CImagePreview final : public wxPanel
        {
        public:
            explicit CImagePreview(wxWindow* parent);
            void SetImage(const wxImage& image);
            void SetCaption(const wxString& caption);

        private:
            void OnPaint(wxPaintEvent& event);
            void OnEraseBackground(wxEraseEvent& event);

            wxImage m_image;
            wxString m_caption;
        };

        void BuildUi(int initialQuality, int initialCodec);
        void OnQualityChanged(wxCommandEvent& event);
        void OnCodecChanged(wxCommandEvent& event);
        void OnExport(wxCommandEvent& event);
        void UpdateQualityText();
       

        ModernTheme m_theme;
        CImagePreview* m_originalPreview = nullptr;
        CImagePreview* m_jpegPreview = nullptr;
        CModernSlider* m_qualitySlider = nullptr;
        CModernButton* m_exportButton = nullptr;
        CModernButton* m_cancelButton = nullptr;
        wxStaticText* m_qualityText = nullptr;
        wxStaticText* m_originalSizeText = nullptr;
        wxStaticText* m_jpegSizeText = nullptr;
        std::array<CModernRadioButton*, 9> m_codecRadios{};
        CImageLoadingFormat* m_imageFormat = nullptr;
        PreviewUpdateCallback m_previewCallback;
    };
}
