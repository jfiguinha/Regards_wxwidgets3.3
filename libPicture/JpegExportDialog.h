
#pragma once

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/statbmp.h>
#include <wx/slider.h>
#include <wx/textctrl.h>

class CImageLoadingFormat;

class CJpegExportDialog : public wxDialog
{
public:
    CJpegExportDialog(CImageLoadingFormat* bitmap);

    int GetQuality() const;
    bool IsOk()
    {
        return isOk;
    }

	int CompressionOption() const
	{
		return m_codecOptions->GetSelection();
	}

	int CompressionLevel() const
	{
		return m_qualitySlider->GetValue();
	}   



private:
    bool CreateControlsFromXrc();
    void GenerateJpegPreview();
    void UpdatePreview();
    void UpdateQualityLabel();

    void AdjustSzieToJpegScalingFactors(int& width, int& height);
    float CalcFitRatio(int pictureWidth, int pictureHeight,
        int targetWidth, int targetHeight);
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnQualityChanged(wxCommandEvent& event);
    void OnResize(wxSizeEvent& event);

private:
    CImageLoadingFormat * m_image;
    wxImage m_jpegPreview;

    wxPanel* m_originalPreviewPanel = nullptr;
    wxPanel* m_jpegPreviewPanel = nullptr;

    wxStaticBitmap* m_originalPreview = nullptr;
    wxStaticBitmap* m_compressedPreview = nullptr;
    wxRadioBox* m_codecOptions = nullptr;
    wxStaticText* m_imageInfo = nullptr;
    wxStaticText* m_originalSizeLabel = nullptr;
    wxStaticText* m_jpegSizeLabel = nullptr;
    wxStaticText* m_qualityLabel = nullptr;

    wxSlider* m_qualitySlider = nullptr;
    wxTextCtrl* m_filePath = nullptr;

    wxString m_outputPath;

    size_t m_jpegSize = 0;
	bool isOk = false;
    static const int JPEG_DEFAULT_QUALITY = 90;
    static const int MIN_QUALITY = 1;
    static const int MAX_QUALITY = 100;
};
