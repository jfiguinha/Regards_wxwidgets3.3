#include <header.h>
#include "WindowModeController.h"
#include "ListPicture.h"
#include "ListFace.h"
#include "PreviewWnd.h"
#include "PanelInfosWnd.h"
#include <WindowManager.h>
#include <PanelWithClickToolbar.h>
#include "window_mode_id.h"
#include <ViewerParamInit.h>
#include <ViewerParam.h>
#include <TreeWindow.h>
#include <ScrollbarWnd.h>
#include <ModernPanel.h>
using namespace Regards::Viewer;
using namespace Regards::Window;

CWindowModeController::CWindowModeController(wxWindow*              parent,
                                             CWindowManager*        windowManager,
                                             CModernPanel* panelInfosClick,
                                             CPreviewWnd*           previewWindow,
                                             CPanelInfosWnd*        panelInfosWindow,
                                             CListPicture*          listPicture,
                                             CListFace*             listFace,
                                             const int&             faceDetection)
    : parent(parent)
    , windowManager(windowManager)
    , panelInfosClick(panelInfosClick)
    , previewWindow(previewWindow)
    , panelInfosWindow(panelInfosWindow)
    , listPicture(listPicture)
    , listFace(listFace)
    , faceDetection(faceDetection)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Mode switching
// ─────────────────────────────────────────────────────────────────────────────

void CWindowModeController::SetMode(wxCommandEvent& event)
{
    bool showInfos         = true;
    bool showThumbnail     = true;
    bool showFolder        = true;
    bool showVideoThumbnail = true;

    windowMode = event.GetInt();
    if (windowMode == oldWindowMode)
        return;

    CMainParam* config = CMainParamInit::getInstance();

    if (!windowInit && oldWindowMode == WINDOW_VIEWER)
    {
        showVideoThumbnail = windowManager->GetPaneState(Pos::wxTOP);
        if (config != nullptr)
            config->SetShowVideoThumbnail(showVideoThumbnail);
    }

    if (windowInit && config != nullptr)
    {
        config->GetShowInfos(showInfos);
        config->GetShowThumbnail(showThumbnail);
        config->GetShowFolder(showFolder);
    }

    if (config != nullptr)
        config->GetShowVideoThumbnail(showVideoThumbnail);

    previewWindow->SetNormalMode();
    panelInfosWindow->Show(false);
    panelInfosClick->Show(false);
    if (faceDetection && listFace != nullptr)
        listFace->Show(false);
    listPicture->Show(false);

    if (windowInit)
    {
        if (!showFolder)
            windowManager->HidePaneWindow(Pos::wxLEFT);
        else
            windowManager->ShowPaneWindow(Pos::wxLEFT);

        if (!showInfos)
            windowManager->HidePaneWindow(Pos::wxRIGHT);
        else
            windowManager->ShowPaneWindow(Pos::wxRIGHT);

        if (!showThumbnail)
            windowManager->HidePaneWindow(Pos::wxBOTTOM);
        else
            windowManager->ShowPaneWindow(Pos::wxBOTTOM);
    }

    if (!showVideoThumbnail)
        windowManager->HidePaneWindow(Pos::wxTOP);
    else
        windowManager->ShowPaneWindow(Pos::wxTOP);

    switch (windowMode)
    {
    case WINDOW_VIEWER:
    {
        wxWindow* window = parent->FindWindowById(PREVIEWVIEWERID);
        if (window != nullptr)
        {
            wxCommandEvent evt(wxEVENT_SHOWSCREENBUTTON);
            window->GetEventHandler()->AddPendingEvent(evt);
        }

        if (!windowManager->GetWindowIsShow(Pos::wxLEFT))   windowManager->ShowWindow(Pos::wxLEFT);
        if (!windowManager->GetWindowIsShow(Pos::wxRIGHT))  windowManager->ShowWindow(Pos::wxRIGHT);
        if (!windowManager->GetWindowIsShow(Pos::wxBOTTOM)) windowManager->ShowWindow(Pos::wxBOTTOM);
        if (!windowManager->GetWindowIsShow(Pos::wxTOP))    windowManager->ShowWindow(Pos::wxTOP);

        if (isVideo || isAnimation)
        {
            if (showVideoThumbnail)
                windowManager->ShowPaneWindow(Pos::wxTOP);
            else
                windowManager->HidePaneWindow(Pos::wxTOP);
        }
        else
        {
            windowManager->HideWindow(Pos::wxTOP);
        }

        windowManager->ShowPaneWindow(Pos::wxRIGHT);
        panelInfosWindow->Show(true);
        panelInfosClick->SetWindow(panelInfosWindow);
        panelInfosClick->Show(true);
        panelInfosClick->SetTitle("Informations");

        if (windowInit && !showInfos)
            windowManager->HidePaneWindow(Pos::wxRIGHT);
    }
    break;

    case WINDOW_FACE:
    {
        wxWindow* window = parent->FindWindowById(PREVIEWVIEWERID);
        if (window != nullptr)
        {
            wxCommandEvent evt(wxEVENT_HIDESCREENBUTTON);
            window->GetEventHandler()->AddPendingEvent(evt);
        }

        if (faceDetection && listFace != nullptr)
        {
            panelInfosClick->Show(true);
            if (!windowManager->GetWindowIsShow(Pos::wxLEFT))   windowManager->ShowWindow(Pos::wxLEFT);
            if (!windowManager->GetWindowIsShow(Pos::wxRIGHT))  windowManager->ShowWindow(Pos::wxRIGHT);
            if (windowManager->GetWindowIsShow(Pos::wxBOTTOM))  windowManager->HideWindow(Pos::wxBOTTOM);
            if (windowManager->GetWindowIsShow(Pos::wxTOP))     windowManager->HideWindow(Pos::wxTOP);

            windowManager->ShowPaneWindow(Pos::wxRIGHT);
            listFace->Show(true);
            panelInfosClick->SetWindow(listFace);
            panelInfosClick->Show(true);
            panelInfosClick->SetTitle("Face List");

            if (windowInit && !showInfos)
                windowManager->HidePaneWindow(Pos::wxRIGHT);
        }
    }
    break;

    case WINDOW_EXPLORER:
    {
        wxWindow* window = parent->FindWindowById(PREVIEWVIEWERID);
        if (window != nullptr)
        {
            wxCommandEvent evt(wxEVENT_HIDESCREENBUTTON);
            window->GetEventHandler()->AddPendingEvent(evt);
        }

        panelInfosClick->Show(true);
        if (!windowManager->GetWindowIsShow(Pos::wxLEFT))   windowManager->ShowWindow(Pos::wxLEFT);
        if (!windowManager->GetWindowIsShow(Pos::wxRIGHT))  windowManager->ShowWindow(Pos::wxRIGHT);
        if (windowManager->GetWindowIsShow(Pos::wxBOTTOM))  windowManager->HideWindow(Pos::wxBOTTOM);
        if (windowManager->GetWindowIsShow(Pos::wxTOP))     windowManager->HideWindow(Pos::wxTOP);

        windowManager->ShowPaneWindow(Pos::wxRIGHT);
        listPicture->Show(true);
        panelInfosClick->SetWindow(listPicture);
        panelInfosClick->Show(true);
        panelInfosClick->SetTitle("Picture List");

        if (windowInit && !showInfos)
            windowManager->HidePaneWindow(Pos::wxRIGHT);
    }
    break;

    case WINDOW_PICTURE:
    {
        wxWindow* window = parent->FindWindowById(PREVIEWVIEWERID);
        if (window != nullptr)
        {
            wxCommandEvent evt(wxEVENT_SHOWSCREENBUTTON);
            window->GetEventHandler()->AddPendingEvent(evt);
        }

        if (windowManager->GetWindowIsShow(Pos::wxLEFT))   windowManager->HideWindow(Pos::wxLEFT);
        if (windowManager->GetWindowIsShow(Pos::wxBOTTOM)) windowManager->HideWindow(Pos::wxBOTTOM);
        if (windowManager->GetWindowIsShow(Pos::wxTOP))    windowManager->HideWindow(Pos::wxTOP);
        if (windowManager->GetWindowIsShow(Pos::wxRIGHT))  windowManager->HideWindow(Pos::wxRIGHT);
    }
    break;

    default: break;
    }

    windowInit    = false;
    oldWindowMode = windowMode;
    windowManager->Resize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Fullscreen
// ─────────────────────────────────────────────────────────────────────────────

bool CWindowModeController::FullscreenMode()
{
    if (!(windowMode == WINDOW_VIEWER || windowMode == WINDOW_PICTURE))
        return false;

    const bool showVideoThumbnail = windowManager->GetPaneState(Pos::wxTOP);
    CMainParam* config = CMainParamInit::getInstance();
    if (config != nullptr)
        config->SetShowVideoThumbnail(showVideoThumbnail);

    previewWindow->SetFullscreen(true);
    if (!isFullscreen)
    {
        // Mémorise l'état courant de chaque panneau pour le retour en mode fenêtré
        savedPanes.left   = windowManager->GetWindowIsShow(Pos::wxLEFT);
        savedPanes.right  = windowManager->GetWindowIsShow(Pos::wxRIGHT);
        savedPanes.top    = windowManager->GetWindowIsShow(Pos::wxTOP);
        savedPanes.bottom = windowManager->GetWindowIsShow(Pos::wxBOTTOM);

        isFullscreen = true;

        // Seule la partie centrale (wxCENTRAL) reste visible
        windowManager->HideWindow(Pos::wxLEFT);
        windowManager->HideWindow(Pos::wxRIGHT);
        windowManager->HideWindow(Pos::wxTOP);
        windowManager->HideWindow(Pos::wxBOTTOM);
        windowManager->Resize();
    }
    return true;
}

bool CWindowModeController::ScreenMode()
{
    if (!(windowMode == WINDOW_VIEWER || windowMode == WINDOW_PICTURE))
        return false;

    const bool showVideoThumbnail = windowManager->GetPaneState(Pos::wxTOP);
    CMainParam* config = CMainParamInit::getInstance();
    if (config != nullptr)
        config->SetShowVideoThumbnail(showVideoThumbnail);

    previewWindow->SetFullscreen(false);
    if (isFullscreen)
    {
        isFullscreen = false;

        // Restauration directe de l'état d'avant plein écran
        // (plus de repost de wxEVENT_SETMODEVIEWER : il recalculait la
        // disposition depuis la config au lieu de conserver l'état réel).
        auto restore = [this](Pos pos, bool wasShown)
        {
            if (wasShown && !windowManager->GetWindowIsShow(pos))
                windowManager->ShowWindow(pos);
        };
        restore(Pos::wxLEFT,   savedPanes.left);
        restore(Pos::wxRIGHT,  savedPanes.right);
        restore(Pos::wxBOTTOM, savedPanes.bottom);
        restore(Pos::wxTOP,    savedPanes.top);
        windowManager->Resize();
    }
    else
    {
        if (windowMode == WINDOW_PICTURE)
        {
            windowManager->HideWindow(Pos::wxRIGHT);
            windowManager->Resize();
        }
    }
    return true;
}

bool CWindowModeController::IsCompatibleFullscreen() const
{
    return (windowMode == WINDOW_VIEWER || windowMode == WINDOW_PICTURE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Toolbar
// ─────────────────────────────────────────────────────────────────────────────

void CWindowModeController::HideToolbar()
{
    // Plein écran : tout est déjà masqué, l'état d'avant plein écran est conservé
    if (isFullscreen)
        return;

    showToolbar = false;

    wxWindow* window = parent->FindWindowById(PREVIEWVIEWERID);
    if (window != nullptr)
    {
        wxCommandEvent evt(wxEVENT_HIDETOOLBAR);
        window->GetEventHandler()->AddPendingEvent(evt);
    }
}

void CWindowModeController::HideTopWindow()
{
    // Plein écran : on ne touche pas à l'affichage, on met à jour l'état à restaurer
    if (isFullscreen)
    {
        savedPanes.top = false;
        return;
    }

    const int showVideoThumbnail = windowManager->GetWindowIsShow(Pos::wxTOP);
    if (showVideoThumbnail)
    {
        windowManager->HideWindow(Pos::wxTOP);
        windowManager->Resize();
    }
}

void CWindowModeController::ShowTopWindow()
{
    // Plein écran : le bandeau vidéo ne doit pas réapparaître, il sera restauré au retour
    if (isFullscreen)
    {
        savedPanes.top = true;
        return;
    }

    const int showVideoThumbnail = windowManager->GetWindowIsShow(Pos::wxTOP);
    if (!showVideoThumbnail)
    {
        windowManager->ShowWindow(Pos::wxTOP);
        windowManager->Resize();
    }
}

void CWindowModeController::ShowToolbar()
{
    // Plein écran : aucune barre / panneau ne doit être réaffiché
    if (isFullscreen)
        return;

    showToolbar = true;

    if (windowMode == WINDOW_VIEWER)
        windowManager->ShowWindow(Pos::wxBOTTOM);

    windowManager->Resize();

    if (previewWindow != nullptr)
    {
        wxCommandEvent evt(wxEVENT_SHOWTOOLBAR);
        previewWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Resize / screen ratio
// ─────────────────────────────────────────────────────────────────────────────

void CWindowModeController::Resize()
{
    windowManager->SetSize(0, 0, parent->GetSize().GetWidth(), parent->GetSize().GetHeight());
}

void CWindowModeController::UpdateScreenRatio()
{
    CThemePane theme_pane;
    CThemeThumbnail themeVideo;
    const int size_new = themeVideo.themeIcone.GetHeight() + theme_pane.GetHeight() * 2;

    if (windowManager != nullptr)
    {
        windowManager->SetWindowSize(Pos::wxBOTTOM, true, size_new);
        windowManager->SetWindowSize(Pos::wxTOP,    true, size_new);
        windowManager->Init();
        windowManager->UpdateScreenRatio();
        windowManager->Resize();
    }
}

void CWindowModeController::UpdateThumbnailIconeSize(wxCommandEvent& /*event*/)
{
    CThemePane theme_pane;
    CThemeThumbnail themeVideo;
    const int size_new = themeVideo.themeIcone.GetHeight() + theme_pane.GetHeight() * 2;

    windowManager->SetWindowSize(Pos::wxBOTTOM, true, size_new);
    windowManager->SetWindowSize(Pos::wxTOP,    true, size_new);
    windowManager->Resize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence
// ─────────────────────────────────────────────────────────────────────────────

void CWindowModeController::SaveParameter(const wxString &lastFile)
{
    CMainParam* config = CMainParamInit::getInstance();
    if (config == nullptr)
        return;

    const wxRect left  = windowManager->GetWindowSize(Pos::wxLEFT);
    const wxRect right = windowManager->GetWindowSize(Pos::wxRIGHT);
    config->SetViewerMode(windowMode);

    const int showFolder        = windowManager->GetPaneState(Pos::wxLEFT);
    const int showVideoThumbnail = windowManager->GetPaneState(Pos::wxTOP);
    const int showThumbnail     = windowManager->GetPaneState(Pos::wxBOTTOM);
    const int showInfos         = windowManager->GetPaneState(Pos::wxRIGHT);

    // En plein écran les panneaux sont masqués : on ne mémorise pas leur taille
    if (showFolder && !isFullscreen) config->SetPositionLeftPanel(left);
    if (showInfos  && !isFullscreen) config->SetPositionRightPanel(right);

    config->SetLastShowPicture(lastFile);
    config->SetShowInfos(showInfos);
    config->SetShowThumbnail(showThumbnail);
    config->SetShowVideoThumbnail(showVideoThumbnail);
    config->SetShowFolder(showFolder);
}
