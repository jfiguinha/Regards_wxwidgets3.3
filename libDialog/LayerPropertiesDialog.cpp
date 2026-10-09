#include "header.h"
#include "LayerPropertiesDialog.h"
#include <wx/xrc/xmlres.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>

namespace Regards
{
    namespace Dialog
    {

        wxBEGIN_EVENT_TABLE(CLayerPropertiesDialog, wxDialog)
            EVT_SLIDER(XRCID("ID_OPACITY_SLIDER"),
                CLayerPropertiesDialog::OnOpacityChanged)
            wxEND_EVENT_TABLE()


            CLayerPropertiesDialog::CLayerPropertiesDialog(
                wxWindow* parent,
                const wxString& layerName,
                LayerBlendMode blendMode,
                int opacity)
            : wxDialog()
        {

            wxXmlResource::Get()->LoadDialog(this, parent, "ID_LAYER_PROPERTIES_DIALOG");

            // Désormais, les pointeurs s'associeront correctement sans planter
            m_layerName = XRCCTRL(*this, "ID_LAYER_NAME", wxTextCtrl);
            m_blendMode = XRCCTRL(*this, "ID_BLEND_MODE", wxComboBox);
            m_opacitySlider = XRCCTRL(*this, "ID_OPACITY_SLIDER", wxSlider);
            m_opacityValue = XRCCTRL(*this, "ID_OPACITY_VALUE", wxStaticText);

            InitializeBlendModes();

            // Nom du calque
            m_layerName->SetValue(layerName);

            // Mode de fusion
            m_blendMode->SetSelection(
                static_cast<int>(blendMode));

            // Opacité
            opacity = std::clamp(opacity, 0, 100);

            m_opacitySlider->SetValue(opacity);

            UpdateOpacityText();

            // Le nom est directement sélectionné
            m_layerName->SetFocus();
            m_layerName->SelectAll();

            // Ajustement automatique
            Layout();
            Fit();
            CentreOnParent();
        }


        void CLayerPropertiesDialog::InitializeBlendModes()
        {
            m_blendMode->Clear();

            m_blendMode->Append("Normal");
            m_blendMode->Append("Multiplier");
            m_blendMode->Append("Écran");
            m_blendMode->Append("Incrustation");
            m_blendMode->Append("Assombrir");
            m_blendMode->Append("Éclaircir");
            m_blendMode->Append("Addition");
            m_blendMode->Append("Soustraction");

            m_blendMode->SetSelection(0);
        }


        void CLayerPropertiesDialog::OnOpacityChanged(wxCommandEvent& event)
        {
            UpdateOpacityText();

            event.Skip();
        }


        void CLayerPropertiesDialog::UpdateOpacityText()
        {
            if (m_opacityValue == nullptr ||
                m_opacitySlider == nullptr)
            {
                return;
            }

            const int opacity = m_opacitySlider->GetValue();

            m_opacityValue->SetLabel(
                wxString::Format("%d %%", opacity));

            Layout();
        }


        wxString CLayerPropertiesDialog::GetLayerName() const
        {
            if (m_layerName == nullptr)
                return wxEmptyString;

            return m_layerName->GetValue();
        }


        LayerBlendMode CLayerPropertiesDialog::GetBlendMode() const
        {
            if (m_blendMode == nullptr)
                return LayerBlendMode::Normal;

            const int selection = m_blendMode->GetSelection();

            if (selection == wxNOT_FOUND)
                return LayerBlendMode::Normal;

            return static_cast<LayerBlendMode>(selection);
        }


        int CLayerPropertiesDialog::GetOpacity() const
        {
            if (m_opacitySlider == nullptr)
                return 100;

            return m_opacitySlider->GetValue();
        }

    }
}