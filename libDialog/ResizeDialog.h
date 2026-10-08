#pragma once
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/notebook.h>

struct ResizeParameter
{
    int width = 1920;
    int height = 1080;
    int interpolation = 0;
};

namespace Regards
{
    namespace Dialog
    {
        class CResizeDialog : public wxDialog
        {
        public:
            CResizeDialog(
                wxWindow* parent,
                const ResizeParameter& parameter);

            ResizeParameter GetParameter() const;
            bool IsOk();

        private:
            void OnbtnOkClick(wxCommandEvent& event);
            void OnBtnCancelClick(wxCommandEvent& event);

            // Événements de changement de valeurs
            void OnPixelWidthChanged(wxSpinEvent& event);
            void OnPixelHeightChanged(wxSpinEvent& event);
            void OnPercentWidthChanged(wxSpinEvent& event);
            void OnPercentHeightChanged(wxSpinEvent& event);
            void OnNotebookPageChanged(wxBookCtrlEvent& event);

        private:
            // Éléments Pixels
            wxSpinCtrl* spinPixelWidth = nullptr;
            wxSpinCtrl* spinPixelHeight = nullptr;

            // Éléments Pourcentage
            wxSpinCtrl* spinPercentWidth = nullptr;
            wxSpinCtrl* spinPercentHeight = nullptr;

            // Options et structure
            wxCheckBox* checkKeepRatio = nullptr;
            wxComboBox* comboInterpolation = nullptr;
            wxNotebook* notebook = nullptr;

            int m_originalWidth = 0;
            int m_originalHeight = 0;
            bool isOk = false;

            wxDECLARE_EVENT_TABLE();
        };
    }
}
