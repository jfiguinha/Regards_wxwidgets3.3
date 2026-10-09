#pragma once
#include <LayerElement.h>
namespace Regards
{
    namespace Dialog
    {

        class CLayerPropertiesDialog : public wxDialog
        {
        public:

            CLayerPropertiesDialog(
                wxWindow* parent,
                const wxString& layerName = wxEmptyString,
                LayerBlendMode blendMode = LayerBlendMode::Normal,
                int opacity = 100);

            ~CLayerPropertiesDialog() override = default;

            wxString GetLayerName() const;
            LayerBlendMode GetBlendMode() const;
            int GetOpacity() const;

        private:

            void OnOpacityChanged(wxCommandEvent& event);

            void UpdateOpacityText();

            void InitializeBlendModes();

        private:

            wxTextCtrl* m_layerName = nullptr;
            wxComboBox* m_blendMode = nullptr;
            wxSlider* m_opacitySlider = nullptr;
            wxStaticText* m_opacityValue = nullptr;

            wxDECLARE_EVENT_TABLE();
        };

    }
}