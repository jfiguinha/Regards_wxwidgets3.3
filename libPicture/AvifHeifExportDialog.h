#pragma once
#pragma once

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/statbmp.h>
#include <wx/slider.h>
#include <wx/choice.h>

class CImageLoadingFormat;

class CAvifHeifExportDialog : public wxDialog
{
public:
    CAvifHeifExportDialog(CImageLoadingFormat* bitmap, int formatChoice);

    bool IsOk() const { return m_isOk; }
    int GetQuality() const;
    int GetSpeed() const;
  //int GetFormatSelection() const; // 0 = AVIF, 1 = HEIF

private:
    bool CreateControlsFromXrc();
    void GeneratePreview();
    void UpdatePreview();
    void UpdateLabels();

    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnSettingsChanged(wxCommandEvent& event);
    void OnResize(wxSizeEvent& event);

private:
    CImageLoadingFormat* m_image = nullptr;
    wxImage m_compressedPreviewImage;

    wxPanel* m_originalPreviewPanel = nullptr;
    wxPanel* m_compressedPreviewPanel = nullptr;

    wxStaticBitmap* m_originalPreview = nullptr;
    wxStaticBitmap* m_compressedPreview = nullptr;

    wxSlider* m_qualitySlider = nullptr;
    wxSlider* m_speedSlider = nullptr;

    wxStaticText* m_imageInfo = nullptr;
    wxStaticText* m_originalSizeLabel = nullptr;
    wxStaticText* m_compressedSizeLabel = nullptr;
    wxStaticText* m_qualityLabel = nullptr;
    wxStaticText* m_speedLabel = nullptr;
	int m_formatChoice = 0; // 0 = AVIF, 1 = HEIF
    size_t m_compressedSize = 0;
    bool m_isOk = false;

    static const int DEFAUL_PICTURE_QUALITY = 75;
    static const int DEFAULT_SPEED = 6;
};
