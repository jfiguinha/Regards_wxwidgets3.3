#pragma once
#include "TreeElement.h"
#include <wx/combobox.h>

namespace Regards::Window
{
	class CTreeElementComboBox : public CTreeElement
	{
	public:
		CTreeElementComboBox();
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

	private:
		// Événement déclenché lors de la sélection dans la ComboBox flottante
		void OnComboSelection(wxCommandEvent& event);

		std::vector<wxString> m_items;
		int m_selectionIndex;
		int m_width;
		int m_height;

		// Pointeur vers le contrôle natif éphémère
		wxComboBox* m_pComboBoxNative;
		wxWindow* m_pParentWindow;

		static constexpr int DEFAULT_WIDTH = 120;
		static constexpr int DEFAULT_HEIGHT = 20;
	};
}
