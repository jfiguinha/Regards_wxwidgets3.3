#pragma once

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

        private:

            void OnWidthChanged(wxSpinEvent& event);
            void OnHeightChanged(wxSpinEvent& event);

        private:

            wxSpinCtrl* spinPixelWidth = nullptr;
            wxSpinCtrl* spinPixelHeight = nullptr;

            wxComboBox* comboInterpolation = nullptr;

            //wxRadioBox* m_position = nullptr;

            int m_originalWidth = 0;
            int m_originalHeight = 0;

            wxDECLARE_EVENT_TABLE();
        };
    }
}

