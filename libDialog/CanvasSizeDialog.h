#pragma once

struct CanvasSizeParameter
{
    int width = 1920;
    int height = 1080;

    bool keepRatio = false;

    enum class Position
    {
        TopLeft,
        Center,
        TopRight,
        BottomLeft,
        BottomRight
    };

    Position position = Position::Center;
};


namespace Regards
{
    namespace Dialog
    {

        class CCanvasSizeDialog : public wxDialog
        {
        public:

            CCanvasSizeDialog(
                wxWindow* parent,
                const CanvasSizeParameter& parameter);

            CanvasSizeParameter GetParameter() const;
            bool IsOk();

        private:
            void OnbtnOkClick(wxCommandEvent& event);
            void OnBtnCancelClick(wxCommandEvent& event);
            void OnWidthChanged(wxSpinEvent& event);
            void OnHeightChanged(wxSpinEvent& event);

        private:

            wxSpinCtrl* m_width = nullptr;
            wxSpinCtrl* m_height = nullptr;

            wxCheckBox* m_keepRatio = nullptr;

            wxRadioBox* m_position = nullptr;
            bool isOk = false;
            int m_originalWidth = 0;
            int m_originalHeight = 0;

            wxDECLARE_EVENT_TABLE();
        };
    }
}