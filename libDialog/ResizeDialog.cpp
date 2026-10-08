#include <header.h>
#include "ResizeDialog.h"
#include <wx/xrc/xmlres.h>

using namespace Regards::Dialog;

BEGIN_EVENT_TABLE(CResizeDialog, wxDialog)
END_EVENT_TABLE()

CResizeDialog::CResizeDialog(
    wxWindow* parent,
    const ResizeParameter& parameter)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "ResizeDialog");

    // 1. Récupération de l'ensemble des contrôles XRC
    spinPixelWidth = XRCCTRL(*this, "m_spinPixelWidth", wxSpinCtrl);
    spinPixelHeight = XRCCTRL(*this, "m_spinPixelHeight", wxSpinCtrl);
    spinPercentWidth = XRCCTRL(*this, "m_spinPercentWidth", wxSpinCtrl);
    spinPercentHeight = XRCCTRL(*this, "m_spinPercentHeight", wxSpinCtrl);
    checkKeepRatio = XRCCTRL(*this, "m_checkKeepRatio", wxCheckBox);
    comboInterpolation = XRCCTRL(*this, "m_comboInterpolation", wxComboBox);
    notebook = XRCCTRL(*this, "m_notebook", wxNotebook);

    // 2. Sécurité anti-crash
    if (!spinPixelWidth || !spinPixelHeight || !spinPercentWidth ||
        !spinPercentHeight || !checkKeepRatio || !comboInterpolation || !notebook)
    {
        wxLogError(wxT("Erreur fatale : Composants de ResizeDialog introuvables dans le fichier XRC."));
        return;
    }

    // 3. Initialisation des données
    m_originalWidth = parameter.width;
    m_originalHeight = parameter.height;

    spinPixelWidth->SetValue(parameter.width);
    spinPixelHeight->SetValue(parameter.height);
    spinPercentWidth->SetValue(100);
    spinPercentHeight->SetValue(100);

    if (parameter.interpolation < static_cast<int>(comboInterpolation->GetCount()))
        comboInterpolation->SetSelection(parameter.interpolation);
    else
        comboInterpolation->SetSelection(1); // LINEAR par défaut si hors limite

    // Ajustement de la taille de la fenêtre
    this->GetSizer()->Fit(this);
    this->GetSizer()->SetSizeHints(this);

    // 4. Liaisons d'événements directes (Boutons natifs)
    Bind(wxEVT_BUTTON, &CResizeDialog::OnbtnOkClick, this, wxID_OK);
    Bind(wxEVT_BUTTON, &CResizeDialog::OnBtnCancelClick, this, wxID_CANCEL);

    // 5. Liaisons pour la mise à jour dynamique des tailles et proportions
    spinPixelWidth->Bind(wxEVT_SPINCTRL, &CResizeDialog::OnPixelWidthChanged, this);
    spinPixelHeight->Bind(wxEVT_SPINCTRL, &CResizeDialog::OnPixelHeightChanged, this);
    spinPercentWidth->Bind(wxEVT_SPINCTRL, &CResizeDialog::OnPercentWidthChanged, this);
    spinPercentHeight->Bind(wxEVT_SPINCTRL, &CResizeDialog::OnPercentHeightChanged, this);
    notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &CResizeDialog::OnNotebookPageChanged, this);
}

void CResizeDialog::OnbtnOkClick(wxCommandEvent& event)
{
    isOk = true;
    EndModal(wxID_OK); // Quitte le mode modal proprement avec l'état OK
}

void CResizeDialog::OnBtnCancelClick(wxCommandEvent& event)
{
    isOk = false;
    EndModal(wxID_CANCEL); // Quitte le mode modal proprement avec l'état Cancel
}

bool CResizeDialog::IsOk()
{
    return isOk;
}

ResizeParameter CResizeDialog::GetParameter() const
{
    ResizeParameter parameter;
    if (spinPixelWidth && spinPixelHeight && comboInterpolation)
    {
        parameter.width = spinPixelWidth->GetValue();
        parameter.height = spinPixelHeight->GetValue();
        parameter.interpolation = comboInterpolation->GetSelection();
    }
    return parameter;
}

// --- Synchronisation : Changement de la Largeur en Pixels ---
void CResizeDialog::OnPixelWidthChanged(wxSpinEvent& event)
{
    int newWidth = event.GetPosition();

    // Calcul et mise à jour du pourcentage de largeur lié
    if (m_originalWidth > 0)
    {
        int newPercentWidth = static_cast<int>((static_cast<float>(newWidth) / m_originalWidth) * 100.0f);
        spinPercentWidth->SetValue(newPercentWidth);
    }

    // Gestion de la proportion hauteur
    if (checkKeepRatio && checkKeepRatio->GetValue() && m_originalWidth > 0)
    {
        float ratio = static_cast<float>(m_originalHeight) / static_cast<float>(m_originalWidth);
        int newHeight = static_cast<int>(newWidth * ratio);
        int newPercentHeight = static_cast<int>((static_cast<float>(newHeight) / m_originalHeight) * 100.0f);

        spinPixelHeight->SetValue(newHeight);
        spinPercentHeight->SetValue(newPercentHeight);
    }
}

// --- Synchronisation : Changement de la Hauteur en Pixels ---
void CResizeDialog::OnPixelHeightChanged(wxSpinEvent& event)
{
    int newHeight = event.GetPosition();

    if (m_originalHeight > 0)
    {
        int newPercentHeight = static_cast<int>((static_cast<float>(newHeight) / m_originalHeight) * 100.0f);
        spinPercentHeight->SetValue(newPercentHeight);
    }

    if (checkKeepRatio && checkKeepRatio->GetValue() && m_originalHeight > 0)
    {
        float ratio = static_cast<float>(m_originalWidth) / static_cast<float>(m_originalHeight);
        int newWidth = static_cast<int>(newHeight * ratio);
        int newPercentWidth = static_cast<int>((static_cast<float>(newWidth) / m_originalWidth) * 100.0f);

        spinPixelWidth->SetValue(newWidth);
        spinPercentWidth->SetValue(newPercentWidth);
    }
}

// --- Synchronisation : Changement de la Largeur en Pourcentage ---
void CResizeDialog::OnPercentWidthChanged(wxSpinEvent& event)
{
    int percentX = event.GetPosition();
    int newWidth = static_cast<int>(m_originalWidth * (percentX / 100.0f));
    spinPixelWidth->SetValue(newWidth);

    if (checkKeepRatio && checkKeepRatio->GetValue())
    {
        spinPercentHeight->SetValue(percentX);
        int newHeight = static_cast<int>(m_originalHeight * (percentX / 100.0f));
        spinPixelHeight->SetValue(newHeight);
    }
}

// --- Synchronisation : Changement de la Hauteur en Pourcentage ---
void CResizeDialog::OnPercentHeightChanged(wxSpinEvent& event)
{
    int percentY = event.GetPosition();
    int newHeight = static_cast<int>(m_originalHeight * (percentY / 100.0f));
    spinPixelHeight->SetValue(newHeight);

    if (checkKeepRatio && checkKeepRatio->GetValue())
    {
        spinPercentWidth->SetValue(percentY);
        int newWidth = static_cast<int>(m_originalWidth * (percentY / 100.0f));
        spinPixelWidth->SetValue(newWidth);
    }
}

// --- Événement provoqué lors du basculement d'onglet du Notebook ---
void CResizeDialog::OnNotebookPageChanged(wxBookCtrlEvent& event)
{
    // Permet d'actualiser proprement l'affichage au besoin lors du changement d'onglet
    event.Skip();
}
