#pragma once
#include "TreeElement.h"
#include "RegardsModernControls.h"

class CThemeTree;

namespace Regards::Window
{
    class CTreeControl;

    class CTreeElementComboBox : public CTreeElement
    {
    public:
        CTreeElementComboBox(CTreeControl* parent, wxString exifKey);
        virtual ~CTreeElementComboBox();

        // Surcharges obligatoires de CTreeElement
        void DrawElement(wxDC* deviceContext, const int& x, const int& y) override;
        void ClickElement(wxWindow* window, const int& x, const int& y) override;

        void SetZoneSize(const int& width, const int& height) override;
        int GetWidth() override;
        int GetHeight() override;

        // Gestion des éléments de la ComboBox
        void SetItems(const std::vector<wxString>& items, const int& defaultSelection = 0);
        int GetSelectionIndex() const;
        wxString GetSelectionString() const;
        void SetSelection(const int& index);

        bool IsDarkTheme() const {
            if (skinMode == 0)
            {
                return isDarkTheme
                    ? true
                    : false;
            }
            else if (skinMode == 1)
            {
                return false;
            }
            return true;
        }

    private:
        // Événement déclenché lors de la sélection dans le CModernCombo
        void OnComboSelection(wxCommandEvent& event);

        // Replace le CModernCombo sur la cellule courante (suivi du défilement de l'arbre)
        void SynchronizeComboPosition(const int& x, const int& y);

        std::vector<wxString> m_items;
        int m_selectionIndex;
        int m_width;
        int m_height;
        wxString exifKey = "";
        int skinMode = 0;
        // Pointeur vers le contrôle moderne éphémère
        Regards::UI::CModernCombo* m_pComboBoxModern;
        wxWindow* m_pParentWindow;
        CTreeControl* m_pParentControl;
        bool isDarkTheme;
        static constexpr int DEFAULT_WIDTH = 120;
        static constexpr int DEFAULT_HEIGHT = 24; // Augmenté légèrement pour un look plus moderne
    };
}