#pragma once
#include <wx/popupwin.h>
#include <wx/sizer.h>
#include <RegardsModernControls.h> // Contient CModernSlider et ModernTheme

namespace Regards::UI
{
    class CVolumePopupModern : public wxPopupTransientWindow
    {
    public:
        CVolumePopupModern(wxWindow* parent, int currentVolume, const ModernTheme& theme, int maxValue, std::function<void(int)> onVolumeChanged)
            : wxPopupTransientWindow(parent, wxBORDER_NONE), m_callback(onVolumeChanged)
        {
            // Application du fond du thème moderne
            SetBackgroundColour(theme.background);
            
            // Création du slider moderne en mode vertical (0 à 100)
            // Note : Si votre CModernSlider ne gère pas nativement le flag wxSL_VERTICAL,
            // il s'adaptera à la boîte horizontale/verticale selon le sizer ou ses dimensions.
            m_slider = new CModernSlider(this, wxID_ANY, currentVolume, 0, maxValue);
            m_slider->SetTheme(theme);
            m_slider->SetVertical(true);
            // Liaison de l'événement de défilement du slider moderne
            m_slider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
                if (m_callback)
                    m_callback(m_slider->GetValue());
                });

            // Layout vertical pour centrer le slider dans le pop-up
            auto* sizer = new wxBoxSizer(wxVERTICAL);
            sizer->Add(m_slider, 1, wxEXPAND | wxALL, 8);
            SetSizerAndFit(sizer);

            // Fixation d'une taille minimale élégante pour le pop-up vertical
            SetSize(wxSize(40, 140));
        }

		void SetVolume(int volume)
		{
			if (m_slider)
				m_slider->SetValue(volume);
		}

		int GetVolume() const
		{
			return m_slider ? m_slider->GetValue() : 0;
		}

    private:
        CModernSlider* m_slider = nullptr;
        std::function<void(int)> m_callback;
    };
}
