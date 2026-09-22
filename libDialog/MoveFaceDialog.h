#pragma once


#ifndef WX_PRECOMP

#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/combobox.h>

#endif


class MoveFaceDialog : public wxDialog
{
public:
	MoveFaceDialog(wxWindow* parent);
	~MoveFaceDialog() override;


	wxButton* BtnCancel;
	wxButton* btnOk;
	wxStaticText* deviceLabel;
	wxComboBox* cbFaceLabel;

	bool IsOk();
	wxString GetFaceNameSelected();

protected:

private:

	void OnInit(wxInitDialogEvent& event);
	void OnbtnOkClick(wxCommandEvent& event);
	void OnBtnCancelClick(wxCommandEvent& event);

	//*)


	bool isOk;
	wxString selectItem;
	DECLARE_EVENT_TABLE()
};
