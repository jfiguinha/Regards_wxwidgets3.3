#include <header.h>
#include "MoveFaceDialog.h"
#include <SqlFindFacePhoto.h>
using namespace Regards::Sqlite;

#include <wx/xrc/xmlres.h>


BEGIN_EVENT_TABLE(MoveFaceDialog, wxDialog)

END_EVENT_TABLE()

MoveFaceDialog::MoveFaceDialog(wxWindow* parent)
{
	isOk = false;

	// 1. Initialisation de la classe de base requise pour wxWidgets 3.x+
	// (Si vous héritez de wxDialog, il faut s'assurer que l'instance XRC sait à quoi se lier)
	if (!wxXmlResource::Get()->LoadDialog(this, parent, "MoveFaceDialog"))
	{
		wxMessageBox("Erreur Critique : Le layout XML 'MoveFaceDialog' n'a pas pu être chargé.\nVérifiez que le fichier XRC est présent dans le dossier de l'exécutable.", "Erreur de Ressources");
		return;
	}

	// 2. Récupération sécurisée des contrôles
	cbFaceLabel = static_cast<wxComboBox*>(FindWindow(XRCID("ID_CBLISTFACE")));
	deviceLabel = static_cast<wxStaticText*>(FindWindow(XRCID("ID_STATICTEXT1")));
	btnOk = static_cast<wxButton*>(FindWindow(XRCID("ID_OK")));
	BtnCancel = static_cast<wxButton*>(FindWindow(XRCID("ID_CANCEL")));

	
	// 3. Test anti-crash de sécurité
	if (!cbFaceLabel || !btnOk || !BtnCancel)
	{
		wxMessageBox("Erreur : Un ou plusieurs éléments (ComboBox, Boutons) sont introuvables dans le fichier XRC.", "Erreur XRCID");
		return;
	}

	// 4. Connexion des événements (Uniquement si les pointeurs sont valides !)
	Connect(XRCID("ID_OK"), wxEVT_COMMAND_BUTTON_CLICKED, (wxObjectEventFunction)&MoveFaceDialog::OnbtnOkClick);
	Connect(XRCID("ID_CANCEL"), wxEVT_COMMAND_BUTTON_CLICKED, (wxObjectEventFunction)&MoveFaceDialog::OnBtnCancelClick);
	Connect(wxID_ANY, wxEVT_INIT_DIALOG, (wxObjectEventFunction)&MoveFaceDialog::OnInit);
}

MoveFaceDialog::~MoveFaceDialog()
{
	//(*Destroy(MoveFaceDialog)
	//*)
}

void MoveFaceDialog::OnInit(wxInitDialogEvent& event)
{
	//int i = 0;
	wxString platform = "";
	wxString device = "";

	CSqlFindFacePhoto sqlFindFacePhoto;
	std::vector<CFaceName> listFaceName = sqlFindFacePhoto.GetListFaceName();

	for (CFaceName faceName : listFaceName)
	{
		cbFaceLabel->Append(faceName.faceName);
	}

	if (listFaceName.size() > 0)
	{
		CFaceName faceName = listFaceName[0];
		cbFaceLabel->SetStringSelection(faceName.faceName);
	}
}


wxString MoveFaceDialog::GetFaceNameSelected()
{
	return cbFaceLabel->GetStringSelection();
}

bool MoveFaceDialog::IsOk()
{
	return isOk;
}


void MoveFaceDialog::OnbtnOkClick(wxCommandEvent& event)
{
	isOk = true;
	this->Close();
}

void MoveFaceDialog::OnBtnCancelClick(wxCommandEvent& event)
{
	isOk = false;
	this->Close();
}
