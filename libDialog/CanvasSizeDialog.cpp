#include <header.h>
#include <CanvasSizeDialog.h>
#include <wx/xrc/xmlres.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
using namespace Regards::Dialog;

BEGIN_EVENT_TABLE(CCanvasSizeDialog, wxDialog)
END_EVENT_TABLE()

CCanvasSizeDialog::CCanvasSizeDialog(
    wxWindow* parent,
    const CanvasSizeParameter& parameter)
{
    // 1. Charger la boîte de dialogue depuis le fichier XRC
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("ID_CANVAS_SIZE_DIALOG"));

    // 2. Lier les pointeurs C++ aux composants graphiques du XRC
    m_width = XRCCTRL(*this, "ID_CANVAS_WIDTH", wxSpinCtrl);
    m_height = XRCCTRL(*this, "ID_CANVAS_HEIGHT", wxSpinCtrl);
    m_keepRatio = XRCCTRL(*this, "ID_KEEP_RATIO", wxCheckBox);
    m_position = XRCCTRL(*this, "ID_CANVAS_POSITION", wxRadioBox);

    // 3. Sécurité anti-crash
    if (!m_width || !m_height || !m_keepRatio || !m_position)
    {
        wxLogError(wxT("Erreur fatale : Impossible de lier les composants du Canvas Size Dialog depuis le fichier XRC."));
        return;
    }

    // 4. Initialiser les valeurs par défaut reçues en paramètre
    m_originalWidth = parameter.width;
    m_originalHeight = parameter.height;

    m_width->SetValue(parameter.width);
    m_height->SetValue(parameter.height);
    m_keepRatio->SetValue(parameter.keepRatio);

    // Sélectionner la bonne position dans le RadioBox
    m_position->SetSelection(static_cast<int>(parameter.position));

    // Ajuster automatiquement les sizers pour que la fenêtre s'adapte au contenu
    this->GetSizer()->Fit(this);
    this->GetSizer()->SetSizeHints(this);

    // 5. Branchement correct des événements des boutons standards wxWidgets
    Bind(wxEVT_BUTTON, &CCanvasSizeDialog::OnbtnOkClick, this, wxID_OK);
    Bind(wxEVT_BUTTON, &CCanvasSizeDialog::OnBtnCancelClick, this, wxID_CANCEL);

    // Branchement des événements de modification de valeur
    m_width->Bind(wxEVT_SPINCTRL, &CCanvasSizeDialog::OnWidthChanged, this);
    m_height->Bind(wxEVT_SPINCTRL, &CCanvasSizeDialog::OnHeightChanged, this);
}

CanvasSizeParameter CCanvasSizeDialog::GetParameter() const
{
    CanvasSizeParameter canvasSize;

    // Récupération des valeurs saisies dans l'IHM
    if (m_width && m_height && m_keepRatio && m_position)
    {
        canvasSize.width = m_width->GetValue();
        canvasSize.height = m_height->GetValue();
        canvasSize.keepRatio = m_keepRatio->GetValue();
        canvasSize.position = static_cast<CanvasSizeParameter::Position>(m_position->GetSelection());
    }

    return canvasSize;
}

void CCanvasSizeDialog::OnbtnOkClick(wxCommandEvent& event)
{
    isOk = true;
    EndModal(wxID_OK); // Ferme proprement le dialogue modal en renvoyant wxID_OK
}

bool CCanvasSizeDialog::IsOk()
{
    return isOk;
}

void CCanvasSizeDialog::OnBtnCancelClick(wxCommandEvent& event)
{
    isOk = false;
    EndModal(wxID_CANCEL); // Ferme proprement le dialogue modal en renvoyant wxID_CANCEL
}

void CCanvasSizeDialog::OnWidthChanged(wxSpinEvent& event)
{
    // Si l'utilisateur change la largeur et veut conserver le ratio
    if (m_keepRatio && m_keepRatio->GetValue() && m_originalWidth > 0)
    {
        float ratio = static_cast<float>(m_originalHeight) / static_cast<float>(m_originalWidth);
        int newHeight = static_cast<int>(event.GetPosition() * ratio);

        // Bloquer temporairement les événements pour éviter une boucle infinie
        m_height->Unbind(wxEVT_SPINCTRL, &CCanvasSizeDialog::OnHeightChanged, this);
        m_height->SetValue(newHeight);
        m_height->Bind(wxEVT_SPINCTRL, &CCanvasSizeDialog::OnHeightChanged, this);
    }
}

void CCanvasSizeDialog::OnHeightChanged(wxSpinEvent& event)
{
    // Si l'utilisateur change la hauteur et veut conserver le ratio
    if (m_keepRatio && m_keepRatio->GetValue() && m_originalHeight > 0)
    {
        float ratio = static_cast<float>(m_originalWidth) / static_cast<float>(m_originalHeight);
        int newWidth = static_cast<int>(event.GetPosition() * ratio);

        // Bloquer temporairement les événements pour éviter une boucle infinie
        m_width->Unbind(wxEVT_SPINCTRL, &CCanvasSizeDialog::OnWidthChanged, this);
        m_width->SetValue(newWidth);
        m_width->Bind(wxEVT_SPINCTRL, &CCanvasSizeDialog::OnWidthChanged, this);
    }
}
