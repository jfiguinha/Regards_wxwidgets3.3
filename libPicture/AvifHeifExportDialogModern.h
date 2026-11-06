#pragma once

#include <RegardsModernControls.h>

class CImageLoadingFormat;

namespace Regards::UI
{
    // Dialogue moderne d'exportation AVIF / HEIF interfaçé avec CImageLoadingFormat
    class CAvifHeifExportDialogModern final : public wxDialog
    {
    public:
        // Callback pour notifier le recalcul de la preview (qualité, effort/vitesse)
        using PreviewUpdateCallback = std::function<void(int quality, int speed, int format)>;

        CAvifHeifExportDialogModern(wxWindow* parent,
            CImageLoadingFormat* bitmap,
            int formatChoice, // 0 = AVIF, 1 = HEIF
            int initialQuality = 75,
            int initialSpeed = 6,
            PreviewUpdateCallback previewCallback = {});

        int GetQuality() const;
        int GetSpeed() const;
        int GetFormatChoice() const { return m_formatChoice; }
        void NotifyPreviewChanged();
        // Permet à la logique métier d'injecter la texture décompressée et sa taille réelle
        void SetCompressedPreview(const wxImage& image, size_t compressedSize);

    private:
        class CImagePreview final : public wxPanel
        {
        public:
            explicit CImagePreview(wxWindow* parent);
            void SetImage(const wxImage& image);

        private:
            void OnPaint(wxPaintEvent& event);
            void OnEraseBackground(wxEraseEvent& event);

            wxImage m_image;
        };

        void BuildUi(int initialQuality, int initialSpeed);
        void OnSettingsChanged(wxCommandEvent& event);
        void OnExport(wxCommandEvent& event);
        void UpdateLabelsText();
        

        ModernTheme m_theme;
        CImagePreview* m_originalPreview = nullptr;
        CImagePreview* m_compressedPreview = nullptr;

        CModernSlider* m_qualitySlider = nullptr;
        CModernSlider* m_speedSlider = nullptr;

        CModernButton* m_exportButton = nullptr;
        CModernButton* m_cancelButton = nullptr;

        wxStaticText* m_qualityText = nullptr;
        wxStaticText* m_speedText = nullptr;
        wxStaticText* m_originalSizeText = nullptr;
        wxStaticText* m_compressedSizeText = nullptr;

        int m_formatChoice = 0;
        CImageLoadingFormat* m_imageFormat = nullptr;
        PreviewUpdateCallback m_previewCallback;
    };
}
