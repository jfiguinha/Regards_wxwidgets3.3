#pragma once
#include "TreeControl.h"
#include "TreeElement.h"
#include "TreeElementSlideInterface.h"
#include "RegardsModernControls.h"

namespace Regards::Window
{
    class CTreeElementSlide : public CTreeElement
    {
    public:
        CTreeElementSlide(CTreeElementSlideInterface* eventInterface);
        virtual ~CTreeElementSlide();

        CTreeElementSlide& operator=(const CTreeElementSlide& other);

        // Surcharges de CTreeElement
        void DrawElement(wxDC* deviceContext, const int& x, const int& y) override;
        void ClickElement(wxWindow* window, const int& x, const int& y) override;
        void UnclickElement(wxWindow* window, const int& x, const int& y) override;
        void MouseOver(wxDC* deviceContext, const int& x, const int& y, bool& update) override;
        void SetVisible(const bool& visible) override;
        void SetZoneSize(const int& width, const int& height) override;
        void SetBackgroundColor(const wxColour& color) override;
        void SetElementPos(const int& x, const int& y) override;

        // Configuration du Slider
        void SetTabValue(std::vector<CTreeElementValue*>* value);
        void SetExifKey(const wxString& exifKey);
        void SetInitValue(CTreeElementValue* value);
        void SetPosition(const int& position);
        void SetTheme(CThemeTreeSlide* theme);

        bool MouseBlock();
        int GetElementWidth() { return themeSlide.GetElementWidth(); }
        int GetElementHeight() { return height; }
        int GetWidth() override { return themeSlide.GetWidth(); }
        int GetHeight() override { return height; }

        // Gestion dynamique du thème identique à CTreeElementComboBox
        bool IsDarkTheme() const {
            if (skinMode == 0)
            {
                return isDarkTheme;
            }
            else if (skinMode == 1)
            {
                return false;
            }
            return true;
        }

    private:
        bool TestEgality(CTreeElementValue* value1, CTreeElementValue* value2, int type);
        void TestMaxMinValue();
        CTreeElementValue* GetFirstValue();
        CTreeElementValue* GetLastValue();
        CTreeElementValue* GetPositionValue();

        // Callback de notification pour l'événement du CModernSlider
        void OnSliderChanged(wxCommandEvent& event);

        std::vector<CTreeElementValue*>* tabValue;
        wxString exifKey;
        int height;
        int position;
        CTreeElementSlideInterface* eventInterface;
        CThemeTreeSlide themeSlide;

        // Variables pour la gestion du thème
        int skinMode;
        bool isDarkTheme;
        bool mouseBlock = false;
        // Nouveau widget de contrôle moderne éphémère
        Regards::UI::CModernSlider* m_pSliderModern;
        wxWindow* m_pParentWindow;

        wxRect slidePos;
        wxRect moinsPos;
        wxRect plusPos;
        wxBitmap slideCacheBuffer;
    };
}
