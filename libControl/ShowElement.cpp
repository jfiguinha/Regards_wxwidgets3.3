#include <header.h>
#include "ShowElement.h"
#if defined(__WXMSW__)
#include "../include/window_id.h"
#else
#include <window_id.h>
#endif
#include <ParamInit.h>
#include <RegardsConfigParam.h>
#include <ImageLoadingFormat.h>
#include <SqlPhotos.h>
#include <DeepLearning.h>
#include "BitmapWndViewer.h"
#include "VideoControl_soft.h"
#include <BitmapWnd3d.h>
#include <SavePicture.h>
#include <appcontext.h>
using namespace Regards::Video;
using namespace Regards::Window;
using namespace Regards::Sqlite;
using namespace Regards::Picture;
using namespace Regards::Control;

extern AppContext application_context;

// ============================================================================
// CThreadRotate – données partagées entre le thread ML et le thread UI.
// Ownership : le thread principal alloue, OnRotateDetect libère.
// ============================================================================
struct CThreadRotate
{
    bool        isReady = false;
    int         exif = 0;
    wxString    filename;
    cv::Mat     bitmap;
    std::thread* thread = nullptr;   // non-owning une fois joint
    wxWindow* mainWindow = nullptr;
};

// ============================================================================
// Adaptateurs IViewerMode
// ============================================================================
namespace
{
    class BitmapViewerMode : public Regards::Control::IViewerMode
    {
    public:
        explicit BitmapViewerMode(CBitmapWndViewer* v) : v_(v) {}
        void FlipVertical()   override { if (v_) v_->FlipVertical(); }
        void FlipHorizontal() override { if (v_) v_->FlipHorizontal(); }
        void Rotate90()       override { if (v_) v_->Rotate90(); }
        void Rotate270()      override { if (v_) v_->Rotate270(); }
    private:
        CBitmapWndViewer* v_;
    };

    class VideoViewerMode : public Regards::Control::IViewerMode
    {
    public:
        explicit VideoViewerMode(CVideoControlSoft* v) : v_(v) {}
        void FlipVertical()   override { if (v_) v_->FlipVertical(); }
        void FlipHorizontal() override { if (v_) v_->FlipHorizontal(); }
        void Rotate90()       override { if (v_) v_->Rotate90(); }
        void Rotate270()      override { if (v_) v_->Rotate270(); }
    private:
        CVideoControlSoft* v_;
    };
}

// ============================================================================
// Constructeur
// ============================================================================
CShowElement::CShowElement(wxWindow* parent,
    wxWindowID id,
    wxWindowID bitmapViewerId,
    wxWindowID mainViewerId,
    CBitmapInterface* bitmapInterfaceIn,
    CThemeParam* config,
    const bool& exportPicture)
    : CWindowMain("ShowBitmap", parent, id)
{
    bitmapInterface = bitmapInterfaceIn;
    configRegards = CParamInit::getInstance();
    windowMain = static_cast<CWindowMain*>(FindWindowById(mainViewerId));
    tempImage = nullptr;

    // --- Thèmes ---
    CThemeSlider     themeSlider;
    CTheme           themeVideo;
    CThemeBitmapWindow themeBitmap;
    CThemeScrollBar  themeScroll;
    CThemeToolbar    themeToolbar;
    CThemeToolbar    themeVideoToolbar;

    if (config)
    {
        config->GetBitmapToolbarTheme(&themeToolbar);
        config->GetVideoControlTheme(&themeVideo);
		config->GetVideoToolbarTheme(&themeVideoToolbar);
        //config->GetVideoSliderTheme(&themeSlider);
        config->GetBitmapWindowTheme(&themeBitmap);
        config->GetScrollTheme(&themeScroll);
    }

    // --- Valeurs de zoom partagées ---
    const std::vector<int> zoomValues = {
        1, 2, 3, 4, 5, 6, 8, 12, 16, 25, 33, 50, 66, 75, 100,
        133, 150, 166, 200, 300, 400, 500, 600, 700, 800, 1200, 1600
    };

    // --- Widgets image ---
    pictureToolbar = std::make_unique<CBitmapToolbar>(this, ANIMATIONTOOLBARWINDOWID, bitmapViewerId,
        themeToolbar, false, exportPicture);
    pictureToolbar->SetTabValue(zoomValues);

    bitmapWindow = std::make_unique<CBitmapWndViewer>(pictureToolbar.get(), mainViewerId,
        themeBitmap, bitmapInterface);
    bitmapWindow->SetTabValue(zoomValues);

    bitmapWindowRender = std::make_unique<CBitmapWnd3D>(this, bitmapViewerId);
    bitmapWindowRender->SetBitmapRenderInterface(bitmapWindow.get());

    scrollbar = std::make_unique<CScrollbarWnd>(this, bitmapWindowRender.get(), wxID_ANY, "BitmapScroll");

    // --- Widgets vidéo ---
    videoWindow = std::make_unique<CVideoControlSoft>(windowMain, this, this);
    bitmapWindowRender->SetBitmapRenderInterface(videoWindow.get());

    videoSlider = std::make_unique<CSliderVideoToolbar>(this, wxID_ANY, this, themeVideoToolbar);

    bitmapWindowRender->UpdateRenderInterface(bitmapWindow.get());

    videoWindow->PlayFirstMovie(false);

    // État initial : mode image
    videoSlider->Show(false);
    scrollbar->Show(true);
    bitmapWindowRender->Show(true);

    // --- Connexion des événements ---
    Connect(wxEVT_BITMAPDBLCLICK, wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(CShowElement::OnViewerDblClick));
    Connect(wxEVT_BITMAPZOOMIN, wxCommandEventHandler(CShowElement::OnViewerZoomIn));
    Connect(wxEVT_BITMAPZOOMOUT, wxCommandEventHandler(CShowElement::OnViewerZoomOut));
    Connect(wxEVENT_MOVELEFT, wxCommandEventHandler(CShowElement::OnMoveLeft));
    Connect(wxEVENT_MOVERIGHT, wxCommandEventHandler(CShowElement::OnMoveRight));
    Connect(wxEVENT_MOVETOP, wxCommandEventHandler(CShowElement::OnMoveTop));
    Connect(wxEVENT_MOVEBOTTOM, wxCommandEventHandler(CShowElement::OnMoveBottom));
    Connect(wxEVENT_SETCONTROLSIZE, wxCommandEventHandler(CShowElement::OnControlSize));
    Connect(wxEVENT_SETPOSITION, wxCommandEventHandler(CShowElement::OnSetPosition));
    Connect(wxEVENT_ROTATEDETECT, wxCommandEventHandler(CShowElement::OnRotateDetect));
    Connect(wxEVENT_PAUSEMOVIE, wxCommandEventHandler(CShowElement::OnPauseVideo));
    Connect(wxEVENT_SAVE, wxCommandEventHandler(CShowElement::OnSave));
    Connect(wxEVENT_SHRINKPOS, wxCommandEventHandler(CShowElement::OnValueShrinkChange));
    Connect(wxEVENT_ZOOMPOS, wxCommandEventHandler(CShowElement::OnValueChange));
    Connect(wxEVENT_SHRINK, wxCommandEventHandler(CShowElement::OnShrink));
    Connect(wxEVENT_CLOSE, wxCommandEventHandler(CShowElement::OnClose));

    // Timer de survol des bords (actif uniquement en plein écran)
    fullscreenMouseTimer.SetOwner(this, wxNewId());
    Bind(wxEVT_TIMER, &CShowElement::OnFullscreenMouseTimer, this, fullscreenMouseTimer.GetId());

    tempImage = std::make_unique<CImageLoadingFormat>();
}

// ============================================================================
// Destructeur – wxWidgets détruit les enfants ; on n'a qu'à vider
// les non-enfants (ici tout est enfant wx, donc rien à faire manuellement).
// unique_ptr<tempImage> est détruit automatiquement.
// ============================================================================
CShowElement::~CShowElement() = default;

// ============================================================================
// IViewerMode dispatch
// ============================================================================
IViewerMode* CShowElement::ActiveViewer() const
{
    // Retourne un adaptateur temporaire sur la pile – usage unique dans
    // chaque méthode de transformation.  On utilise un thread_local pour
    // éviter l'allocation heap tout en restant simple.
    static thread_local BitmapViewerMode bm(nullptr);
    static thread_local VideoViewerMode  vm(nullptr);
    if (!isVideo) { bm = BitmapViewerMode(bitmapWindow.get()); return &bm; }
    else { vm = VideoViewerMode(videoWindow.get());   return &vm; }
}

void CShowElement::FlipVertical() { ActiveViewer()->FlipVertical(); }
void CShowElement::FlipHorizontal() { ActiveViewer()->FlipHorizontal(); }
void CShowElement::Rotate90() { ActiveViewer()->Rotate90(); }
void CShowElement::Rotate270() { ActiveViewer()->Rotate270(); }


CLayerList* CShowElement::GetLayerList()
{
    if (bitmapWindow)
        return bitmapWindow->GetLayerList();

    return nullptr;
}

// ============================================================================
// Affichage / plein écran
// ============================================================================
void CShowElement::SetFullscreen(const bool& fs)
{
    if (fs == fullscreen)
    {
        bitmapWindow->SetFullscreen(fs);
        return;
    }

    if (fs)
        showToolbarBeforeFullscreen = showToolbar;

    fullscreen = fs;
    bitmapWindow->SetFullscreen(fs);

    if (fs)
    {
        // Plein écran : seul bitmapWindowRender (encapsulé dans scrollbar) est visible
        if (pictureToolbar && pictureToolbar->IsShown())
        {
            bitmapWindow->FixArrowNavigation(false);
            pictureToolbar->Show(false);
        }
        if (videoSlider)
            videoSlider->Show(false);

        showToolbar = false;   // ResizeImage masque alors aussi les barres de défilement
        currentZone = EdgeZone::Center;
        scrollbar->Show(true);
        bitmapWindowRender->Show(true);
        lastMousePos = wxGetMousePosition();
        lastMoveTime = wxGetLocalTimeMillis();
        fullscreenMouseTimer.Start(100);
    }
    else
    {
        // Sortie : arrêt du survol et masquage des barres éventuellement affichées au survol
        fullscreenMouseTimer.Stop();
        SetMouseCursorHidden(false);
        currentZone = EdgeZone::Center;
        if (pictureToolbar && pictureToolbar->IsShown())
        {
            bitmapWindow->FixArrowNavigation(false);
            pictureToolbar->Show(false);
        }
        if (videoSlider)
            videoSlider->Show(false);

        // Retour en mode fenêtré : restauration de l'état d'avant plein écran
        showToolbar = showToolbarBeforeFullscreen;
        if (showToolbar)
        {
            if (mediaMode == MediaMode::Video)
            {
                if (videoSlider)
                    videoSlider->Show(true);
            }
            else if (mediaMode == MediaMode::Bitmap || mediaMode == MediaMode::Animation)
            {
                if (pictureToolbar && !pictureToolbar->IsShown())
                {
                    bitmapWindow->FixArrowNavigation(true);
                    pictureToolbar->Show(true);
                }
            }
        }
    }

    Resize();
}

void CShowElement::UpdateScreenRatio()
{
    scrollbar->UpdateScreenRatio();
    pictureToolbar->UpdateScreenRatio();
    bitmapWindowRender->UpdateScreenRatio();
    videoSlider->UpdateScreenRatio();
    Resize();
}

void CShowElement::ReloadResource()
{
    if (!isVideo)
        bitmapWindow->ReloadResource();
    else
        videoWindow->ReloadResource();
}

// ============================================================================
// Resize – découpé en deux sous-routines claires
// ============================================================================
void CShowElement::Resize()
{
    const int w = GetWindowWidth();
    const int h = GetWindowHeight();
    if (w <= 0 || h <= 0)
        return;

    if (!isVideo)
        ResizeImage(w, h);
    else
    {
        ResizeVideo(w, h);

        videoSlider->SetZoomIndex(videoWindow->GetZoomIndex());
    }

}

void CShowElement::AnimationStart()
{
    if(pictureToolbar)
        pictureToolbar->AnimationStart();
}

void CShowElement::ResizeImage(int w, int h)
{
    if (fullscreen)
    {
        scrollbar->HideHorizontalScroll();
        scrollbar->HideVerticalScroll();
    }
    else
    {
        scrollbar->ShowVerticalScroll();
        scrollbar->ShowHorizontalScroll();
    }

    if (!showToolbar && pictureToolbar->IsShown())
        HideToolbar();

    if (pictureToolbar->IsShown())
    {
        const int tbH = pictureToolbar->GetHeight();
        scrollbar->SetSize(0, 0, w, h - tbH);
        scrollbar->Refresh();
        pictureToolbar->SetSize(0, h - tbH, w, tbH);
        pictureToolbar->Refresh();
    }
    else
    {
        scrollbar->SetSize(0, 0, w, h);
        scrollbar->Refresh();
    }
}

void CShowElement::ResizeVideo(int w, int h)
{
    int posHeight = 0;
    int posHeightSup = 0;

    if (showToolbar)
    {
        if (videoSlider->IsShown())
            posHeight += videoSlider->GetHeight();
    }
    else
    {
        if (videoSlider->IsShown())
            videoSlider->Hide();
    }

    const int contentH = h - posHeight - posHeightSup;
    scrollbar->SetSize(0, 0, w+1, contentH);
    scrollbar->Refresh();

    if (showToolbar && videoSlider->IsShown())
    {
        videoSlider->SetSize(0, contentH + posHeightSup, w, videoSlider->GetHeight(), TRUE);
        videoSlider->Refresh();
    }
}

// ============================================================================
// Plein écran : barres affichées quand la souris est en haut / en bas de l'image
// ============================================================================
void CShowElement::OnFullscreenMouseTimer(wxTimerEvent& /*event*/)
{
    if (!fullscreen || !bitmapWindowRender)
        return;

    const wxPoint screenMouse = wxGetMousePosition();
    const wxPoint mouse = bitmapWindowRender->ScreenToClient(screenMouse);
    const wxSize  size  = bitmapWindowRender->GetClientSize();
    const bool insideRender = mouse.x >= 0 && mouse.x < size.x && mouse.y >= 0 && mouse.y < size.y;

    // Curseur : réaffiché dès que la souris bouge, masqué après 2 s d'immobilité sur l'image
    const wxLongLong now = wxGetLocalTimeMillis();
    if (screenMouse != lastMousePos)
    {
        lastMousePos = screenMouse;
        lastMoveTime = now;
        SetMouseCursorHidden(false);
    }
    else if (insideRender && (now - lastMoveTime).ToLong() >= cursorHideDelayMs)
    {
        SetMouseCursorHidden(true);
    }

    // Souris hors du rendu : sur une barre affichée (on conserve l'état) ou hors fenêtre
    if (!insideRender)
    {
        SetMouseCursorHidden(false);
        return;
    }

    const int band = FromDIP(60);
    EdgeZone zone = EdgeZone::Center;
    if (mouse.y < band)
        zone = EdgeZone::Top;
    else if (mouse.y >= size.y - band)
        zone = EdgeZone::Bottom;

    UpdateEdgeZone(zone);
}

void CShowElement::SetMouseCursorHidden(bool hide)
{
    if (cursorHidden == hide)
        return;

    cursorHidden = hide;
    if (bitmapWindowRender)
        bitmapWindowRender->SetCursor(hide ? wxCursor(wxCURSOR_BLANK) : wxNullCursor);
}

void CShowElement::UpdateEdgeZone(EdgeZone zone)
{
    if (zone == currentZone)
        return;

    const bool bottomChanged = (zone == EdgeZone::Bottom) != (currentZone == EdgeZone::Bottom);
    currentZone = zone;

    if (bottomChanged)
        ShowFullscreenBars(zone == EdgeZone::Bottom);

    // Barres du parent (CPreviewWnd) : infos en haut, toolbar en bas
    if (edgeZoneCallback)
        edgeZoneCallback(zone);
}

void CShowElement::ShowFullscreenBars(bool show)
{
    showToolbar = show;

    if (mediaMode == MediaMode::Video)
    {
        if (videoSlider && videoSlider->IsShown() != show)
            videoSlider->Show(show);
    }
    else if (mediaMode == MediaMode::Bitmap || mediaMode == MediaMode::Animation)
    {
        if (pictureToolbar && pictureToolbar->IsShown() != show)
        {
            bitmapWindow->FixArrowNavigation(show);
            pictureToolbar->Show(show);
        }
    }
    Resize();
}

// ============================================================================
// Toolbar
// ============================================================================
void CShowElement::ShowToolbar()
{
    // Plein écran : les barres ne doivent pas réapparaître
    if (fullscreen || showToolbar)
        return;

    showToolbar = true;
    if (!isVideo)
    {
        if (!pictureToolbar->IsShown())
        {
            bitmapWindow->FixArrowNavigation(true);
            pictureToolbar->Show(true);
            Resize();
        }
    }
    else
    {
        if (!videoSlider->IsShown())
            videoSlider->Show(true);
        Resize();
    }
}

void CShowElement::HideToolbar()
{
	if (!showToolbar)
		return;
    // Vidéo
    if (videoSlider && !videoSlider->IsMouseOver())
    {
        if (videoSlider->IsShown())
            videoSlider->Show(false);
    }

    showToolbar = false;

    // Image
    if (pictureToolbar && pictureToolbar->IsShown() && !pictureToolbar->IsMouseOver())
    {
        bitmapWindow->FixArrowNavigation(false);
        pictureToolbar->Show(false);
    }

    Resize();
}

bool CShowElement::IsToolbarMouseOver()
{
    if (isVideo)
        return pictureToolbar ? pictureToolbar->IsMouseOver() : false;
    return videoSlider ? videoSlider->IsMouseOver() : false;
}

// ============================================================================
// Type de média : une seule toolbar (CBitmapToolbar) pour Bitmap / Animation,
// le slider vidéo pour Video. Aucun : tout est masqué.
// ============================================================================
void CShowElement::SetMedia(const MediaMode& mode)
{
    if (mode == mediaMode)
        return;

    mediaMode = mode;
    isVideo = (mode == MediaMode::Video);

    if (pictureToolbar)
        pictureToolbar->SetMedia(mode);

    // En plein écran, les barres ne sont visibles que si la souris est en bas de l'image
    const bool barsAllowed = !fullscreen || currentZone == EdgeZone::Bottom;
    if (fullscreen)
        showToolbar = barsAllowed;

    const bool toolbarVisible = barsAllowed && (mode == MediaMode::Bitmap || mode == MediaMode::Animation);
    if (pictureToolbar)
        pictureToolbar->Show(toolbarVisible);
    if (videoSlider)
        videoSlider->Show(barsAllowed && mode == MediaMode::Video);
    

    if (mediaMode == MediaMode::Video)
        bitmapWindowRender->UpdateRenderInterface(videoWindow.get());
    else
        bitmapWindowRender->UpdateRenderInterface(bitmapWindow.get());

    Resize();
}

// ============================================================================
// Événements navigation / scroll (forwarding vers scrollbar)
// ============================================================================
namespace
{
    void ForwardScrollEvent(CScrollbarWnd* sb, wxEventType type, wxCommandEvent& src)
    {
        if (!sb) return;
        wxCommandEvent evt(type);
        evt.SetInt(src.GetInt());
        evt.SetClientData(src.GetClientData());
        sb->GetEventHandler()->AddPendingEvent(evt);
    }
}

void CShowElement::OnMoveLeft(wxCommandEvent& e) { ForwardScrollEvent(scrollbar.get(), wxEVENT_MOVELEFT, e); }
void CShowElement::OnMoveRight(wxCommandEvent& e) { ForwardScrollEvent(scrollbar.get(), wxEVENT_MOVERIGHT, e); }
void CShowElement::OnMoveTop(wxCommandEvent& e) { ForwardScrollEvent(scrollbar.get(), wxEVENT_MOVETOP, e); }
void CShowElement::OnMoveBottom(wxCommandEvent& e) { ForwardScrollEvent(scrollbar.get(), wxEVENT_MOVEBOTTOM, e); }

void CShowElement::OnControlSize(wxCommandEvent& event)
{
    if (!scrollbar) return;
    wxCommandEvent evt(wxEVENT_SETCONTROLSIZE);
    evt.SetClientData(event.GetClientData());
    scrollbar->GetEventHandler()->AddPendingEvent(evt);
}

void CShowElement::OnSetPosition(wxCommandEvent& event)
{
    if (!isVideo)
    {
        if (!scrollbar) return;
        wxCommandEvent evt(wxEVENT_SETPOSITION);
        evt.SetClientData(event.GetClientData());
        scrollbar->GetEventHandler()->AddPendingEvent(evt);
    }
    else
    {
        SetTimePosition(event.GetExtraLong() * 1000 * 1000);
    }
}

void CShowElement::OnViewerDblClick(wxCommandEvent& /*event*/)
{
    if (!pictureToolbar || fullscreen) return;
    pictureToolbar->IsShown() ? HideToolbar() : ShowToolbar();
}

void CShowElement::OnViewerZoomIn(wxCommandEvent& /*event*/)
{
    if (pictureToolbar) pictureToolbar->ChangeZoomInPos();
}

void CShowElement::OnViewerZoomOut(wxCommandEvent& /*event*/)
{
    if (pictureToolbar) pictureToolbar->ChangeZoomOutPos();
}

// ============================================================================
// Transition / image
// ============================================================================
void CShowElement::StopTransition()
{
    bitmapWindow->StopTransition();
}

void CShowElement::TransitionEnd()
{
    transitionEnd = true;
    if (tempImage)
    {
        SetBitmapToViewer(tempImage.release(), wxEVENT_SETBITMAP);
        if (pictureToolbar)
            pictureToolbar->SetTrackBarPosition(bitmapWindow->GetPosRatio());
    }
}

void CShowElement::SetBitmapPreviewEffect(const int& effect)
{
    bitmapWindow->SetBitmapPreviewEffect(effect);
}

// Envoie un événement wxEVENT_SHOWSAVEBUTTON / wxEVENT_HIDESAVEBUTTON
// au PREVIEWVIEWERID et retourne le wxWindow trouvé (ou nullptr).
static wxWindow* SendPreviewButtonEvents(wxWindow* root,
    wxEventType saveEvt,
    wxEventType exportEvt)
{
    wxWindow* w = root->FindWindowById(PREVIEWVIEWERID);
    if (!w) return nullptr;
    w->GetEventHandler()->AddPendingEvent(wxCommandEvent(saveEvt));
    w->GetEventHandler()->AddPendingEvent(wxCommandEvent(exportEvt));
    return w;
}

void CShowElement::SetBitmapToViewer(CImageLoadingFormat* bitmap, int eventType)
{
    auto event = new wxCommandEvent(eventType);// wxEVENT_SETBITMAP);
    event->SetClientData(bitmap);
    wxQueueEvent(bitmapWindowRender.get(), event);
}

bool CShowElement::SetBitmap(CImageLoadingFormat* bitmap, const bool& isThumbnail)
{
    SendPreviewButtonEvents(this,
        isThumbnail ? wxEVENT_HIDESAVEBUTTON : wxEVENT_SHOWSAVEBUTTON,
        wxEVENT_HIDEEXPORTBUTTON);

    filename = bitmap->GetFilename();

    // Lecture orientation EXIF depuis la DB
    int angle = 0, flipH = 0, flipV = 0;
    if (!isThumbnail)
    {
        CSqlPhotos sqlPhotos;
        int exif = sqlPhotos.GetPhotoExif(filename);
        if (exif > 0)
            application_context.GetAngleAndFlip(exif, angle, flipH, flipV);
    }

    bitmapWindow->SetIsBitmapThumbnail(isThumbnail);

    const int numEffect = isDiaporama
        ? configRegards->GetDiaporamaTransitionEffect()
        : (configRegards ? configRegards->GetEffect() : 0);

    if (numEffect != 0)
    {
        if (isThumbnail || isDiaporama)
        {
            transitionEnd = false;
            bitmapWindow->ShrinkImage();
            SetBitmapToViewer(bitmap, wxEVENT_SETTRANSITIONBITMAP);
            //bitmapWindow->SetTransitionBitmap(bitmap);
        }
        else if (transitionEnd)
        {
            bitmapWindow->StopTransition();
            SetBitmapToViewer(bitmap, wxEVENT_SETBITMAP);
            //bitmapWindow->SetBitmap(bitmap, false);
            bitmapWindow->ApplyPicturePosition(angle, flipH, flipV);
        }
        else
        {
            transitionEnd = false;
        }
    }
    else
    {
        SetBitmapToViewer(bitmap, wxEVENT_SETBITMAP);
        bitmapWindow->ApplyPicturePosition(angle, flipH, flipV);
    }

    firstElement = false;

    // Mise en cache pour recharge après effet
    if (!isThumbnail && numEffect != 0)
    {
        bool needReload = true;
        if (isDiaporama)
        {
            std::unique_ptr<IAfterEffect> fx = CBitmapWndViewer::AfterEffectPt(numEffect);
            needReload = fx->NeedToReload();
        }
        if (needReload)
        {
            CImageLoadingFormat* picture = new CImageLoadingFormat();
            picture->SetPicture(bitmap->GetMatImage());
            tempImage.reset(picture);
        }
    }

    if (pictureToolbar)
        pictureToolbar->SetTrackBarPosition(bitmapWindow->GetPosRatio());

    Resize();


    // Une animation en cours reste en mode Animation (chaque frame passe par SetBitmap)

    return true;
}

CImageLoadingFormat* CShowElement::GetBitmap(const bool& source)
{
    return bitmapWindow ? bitmapWindow->GetBitmap(source) : nullptr;
}

void CShowElement::SavePicture()
{
    if (isVideo)
    {
        wxCommandEvent evt(wxEVENT_SAVE);
        GetEventHandler()->AddPendingEvent(evt);
    }
    else if (bitmapWindow)
    {
        bitmapWindow->SavePicture();
    }
}

// ============================================================================
// Thread de reconnaissance d'orientation (ML)
// ============================================================================
/*static*/
void CShowElement::RotateRecognition(void* param)
{
    auto* tr = static_cast<CThreadRotate*>(param);
    if (!tr) return;

    if (!tr->bitmap.empty())
    {
        CRegardsConfigParam* cfg = CParamInit::getInstance();
        const bool fast = cfg ? cfg->GetFastDetectionFace() : true;
        tr->exif = DeepLearning::CDeepLearning::GetExifOrientation(tr->bitmap, fast);
        tr->isReady = true;
    }

    if (tr->mainWindow)
    {
        wxCommandEvent evt(wxEVENT_ROTATEDETECT);
        evt.SetClientData(tr);
        tr->mainWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}

void CShowElement::OnRotateDetect(wxCommandEvent& event)
{
    auto* tr = static_cast<CThreadRotate*>(event.GetClientData());
    if (!tr) return;

    if (tr->thread)
    {
        tr->thread->join();
        delete tr->thread;
        tr->thread = nullptr;
    }

    if (tr->filename == filename && tr->isReady)
        bitmapWindow->SetOrientation(tr->exif);

    CSqlPhotos sqlPhotos;
    sqlPhotos.InsertPhotoExif(tr->filename, tr->exif);

    delete tr;
}

/*static*/
int CShowElement::GetAngleAndFlipFromExif(int exif, int& angle, int& flipH, int& flipV)
{
    // Table : exif[0-15] → (angle ∈ {0,90,180,270}, flipH, flipV)
    static const struct { int angle, flipH, flipV; } table[] = {
        { 0, 0, 0 },{ 0, 1, 0 },{ 0, 0, 1 },{ 0, 1, 1 },
        { 90, 0, 0 },{ 90, 1, 0 },{ 90, 0, 1 },{ 90, 1, 1 },
        { 180, 0, 0 },{ 180, 1, 0 },{ 180, 0, 1 },{ 180, 1, 1 },
        { 270, 0, 0 },{ 270, 1, 0 },{ 270, 0, 1 },{ 270, 1, 1 },
    };
    if (exif < 0 || exif > 15) return -1;
    angle = table[exif].angle;
    flipH = table[exif].flipH;
    flipV = table[exif].flipV;
    return 0;
}

// ============================================================================
// Vidéo – contrôle
// ============================================================================
bool CShowElement::SetVideo(const wxString& fn, const int& /*rotation*/, const bool& /*playNow*/)
{

    // Garde mediaMode, isVideo, toolbar, slider et interface de rendu synchronisés
    SetMedia(MediaMode::Video);
    videoTotalTime = 0;
    videoPosOld = 0;
    filename = fn;

    const bool ok = videoWindow->PlayMovie(fn, true);

    wxWindow* w = FindWindowById(PREVIEWVIEWERID);
    if (w)
    {
        wxCommandEvent evt(wxEVENT_SHOWEXPORTBUTTON);
        w->GetEventHandler()->AddPendingEvent(evt);
    }

    return ok;
}

void CShowElement::StopVideo(const wxString& photoName)
{
    play = false;
    ShowSliderToolbar(true);
    videoSlider->SetPastSecondTime(0);
    videoWindow->OnStop(photoName);
    refreshSlider = true;
}

void CShowElement::PlayVideo()
{
    videoWindow->OnPlay();
    videoSlider->SetPlay();
}

void CShowElement::PauseVideo()
{
    play = false;
    videoWindow->OnPause();
    ShowSliderToolbar(true);
    videoSlider->SetPause();
    refreshSlider = true;
}

void CShowElement::RepeatVideo()
{
    videoWindow->RepeatVideo();
}

void CShowElement::OnPauseVideo(wxCommandEvent& /*event*/)
{
    PauseVideo();
}

bool CShowElement::IsPause()
{
    return videoWindow->IsPause();
}

void CShowElement::ChangeAudio(const int& langue)
{
    videoWindow->ChangeAudioStream(langue);
}

CVideoControlSoft* CShowElement::GetVideoControl()
{
    return videoWindow.get();
}

cv::Mat CShowElement::GetVideoBitmap()
{
    if (videoWindow && IsPause())
    {
        bool fromBuffer = false;
        return videoWindow->SavePicture(fromBuffer);
    }
    return {};
}

void CShowElement::SetVideoPreviewEffect(CEffectParameter* ep)
{
    if (videoWindow) videoWindow->SetVideoPreviewEffect(ep);
}

CEffectParameter* CShowElement::GetParameter()
{
    return videoWindow ? videoWindow->GetParameter() : nullptr;
}

void CShowElement::UpdateFiltre(CEffectParameter* ep)
{
    if (videoWindow) videoWindow->UpdateFiltre(ep);
}

// ============================================================================
// Vidéo – slider / position
// ============================================================================
void CShowElement::SetVideoDuration(const int64_t& position)
{
    videoTotalTime = position;
    videoSlider->SetTotalSecondTime(position / 1000);
    refreshSlider = true;
}

void CShowElement::SetPosition(const int64_t& timePosition)
{
    if (!bitmapWindowRender) return;
    wxCommandEvent evt(wxEVENT_SETPOSITION);
    evt.SetExtraLong(timePosition);
    bitmapWindowRender->GetEventHandler()->AddPendingEvent(evt);
}

void CShowElement::SetTimePosition(const int64_t& timePosition)
{
    videoSlider->SetPastSecondTime(timePosition / 1000);
    videoSlider->UpdatePositionEvent();
    refreshSlider = true;
}

void CShowElement::MoveSlider(const int64_t& position)
{
    if (!bitmapWindowRender) return;
    wxCommandEvent evt(wxEVENT_SETPOSITION);
    evt.SetExtraLong(position);
    bitmapWindowRender->GetEventHandler()->AddPendingEvent(evt);
}

void CShowElement::ShowSliderToolbar(bool show)
{
    // Plein écran : le slider vidéo n'apparaît que si la souris est en bas de l'image
    if (fullscreen && show && currentZone != EdgeZone::Bottom)
        return;

    const bool shown = videoSlider->IsShown();
    if (show != shown)
    {
        videoSlider->Show(show);
        Resize();
    }
}

void CShowElement::ShowSlider(bool show)
{
    if (play)
        ShowSliderToolbar(true);
}

void CShowElement::InitControl()
{
    if (videoWindow)
        Resize();
}

// ============================================================================
// Vidéo – callbacks IVideoInterface
// ============================================================================
void CShowElement::OnVideoEnd()
{
    videoSlider->SetPause();
    if (wxWindow* w = FindWindowById(CENTRALVIEWERWINDOWID))
        w->GetEventHandler()->AddPendingEvent(wxCommandEvent(VIDEO_END_ID));
    refreshSlider = true;
}

void CShowElement::OnVideoStop()
{
    videoSlider->SetPause();
    if (wxWindow* w = FindWindowById(CENTRALVIEWERWINDOWID))
        w->GetEventHandler()->AddPendingEvent(wxCommandEvent(wxVIDEO_STOP));
    refreshSlider = true;
}

void CShowElement::OnVideoStart()
{
    play = true;
    videoSlider->SetPlay();
    videoSlider->SetZoomIndex(videoWindow->GetZoomIndex());
    if (wxWindow* w = FindWindowById(CENTRALVIEWERWINDOWID))
        w->GetEventHandler()->AddPendingEvent(wxCommandEvent(VIDEO_START));
    refreshSlider = true;
}

void CShowElement::OnVideoPause() {}

void CShowElement::OnAfterOpenVideo()
{
    videoSlider->SetTotalSecondTime(0);
    videoSlider->SetPastSecondTime(0);
    ShowSlider(false);
    PlayVideo();
    refreshSlider = true;
}

void CShowElement::OnPositionVideo(const int64_t& position)
{
    const int videoPos = static_cast<int>(position / 1000);
    if (position < 0 || position > videoTotalTime || videoPos == videoPosOld)
        return;

    videoSlider->SetPastSecondTime(position);

    if (wxWindow* w = FindWindowById(CENTRALVIEWERWINDOWID))
    {
        wxCommandEvent evt(VIDEO_UPDATE_ID);
        evt.SetExtraLong(videoPos);
        w->GetEventHandler()->AddPendingEvent(evt);
    }
    videoPosOld = videoPos;
    refreshSlider = true;
}

// ============================================================================
// Modes affichage
// ============================================================================
void CShowElement::SetDiaporamaMode()
{
    isDiaporama = true;
    videoWindow->DiaporamaMode(true);
    bitmapWindow->SetDiaporamaMode();
    Resize();
}

void CShowElement::SetNormalMode()
{
    isDiaporama = false;
    videoWindow->DiaporamaMode(false);
    bitmapWindow->SetNormalMode();
    Resize();
}

// ============================================================================
// Streams info
// ============================================================================
void CShowElement::SetStreamInfo(vector<CStreamInfo>& listAudio,
    vector<CStreamInfo>& listVideo,
    vector<CStreamInfo>& listSubtitle)
{
    listStream.clear();
    listStream.insert(listStream.end(), listAudio.begin(), listAudio.end());
    listStream.insert(listStream.end(), listVideo.begin(), listVideo.end());
    listStream.insert(listStream.end(), listSubtitle.begin(), listSubtitle.end());

    if (windowMain)
    {
        wxCommandEvent evt(wxEVT_COMMAND_TEXT_UPDATED, wxEVENT_ADDSTREAMINFO);
        evt.SetClientData(&listStream);
        windowMain->GetEventHandler()->AddPendingEvent(evt);
    }
}

// ============================================================================
// Boutons toolbar vidéo
// ============================================================================
void CShowElement::ClickButton(const int& id)
{
    switch (id)
    {
    case PLAYBUTTONID:
        PlayVideo();
        break;
    case PAUSEBUTTONID:
        PauseVideo();
        break;
    case REPEATID:
        RepeatVideo();
        break;
    case ZOOMBUTTONID:
    case SPEAKERBUTTONID:
        //showToolbarSup = true;
        Resize();
        break;
    case SCREENFORMATID:
        showToolbarSup = false;
        videoWindow->ChangeVideoFormat();
        Resize();
        break;
    default:
        break;
    }
    refreshSlider = true;
}

// ============================================================================
// Événements slider / zoom
// ============================================================================
void CShowElement::OnValueShrinkChange(wxCommandEvent& /*event*/)
{

}


int CShowElement::GetVolume()
{
    if (videoWindow)
        return videoWindow->GetVolume();
    else
        return 0;
}

int CShowElement::GetZoomIndex()
{
    if (videoWindow)
        return videoWindow->GetZoomIndex();
    else
        return bitmapWindow->GetZoom();
}

void CShowElement::SlidePosChange(const int& position, const wxString& key)
{
    if (key == "VOLUME")
        videoWindow->SetVolume(position);
    if (key == "ZOOM")
        videoWindow->SetZoomIndex(position);
    if (key == "SHRINK")
        videoWindow->ShrinkVideo();
}

void CShowElement::OnValueChange(wxCommandEvent& event)
{
    const int value = event.GetInt();
    scrollbar->ShowHorizontalScroll();
    scrollbar->ShowVerticalScroll();
    videoWindow->SetZoomIndex(value);
}

void CShowElement::OnShrink(wxCommandEvent& /*event*/)
{
    videoWindow->ShrinkVideo();
    scrollbar->HideHorizontalScroll();
    scrollbar->HideVerticalScroll();
}

void CShowElement::OnClose(wxCommandEvent& /*event*/)
{
    showToolbarSup = false;
    Resize();
}

// ============================================================================
// Sauvegarde image depuis vidéo
// ============================================================================
void CShowElement::OnSave(wxCommandEvent& /*event*/)
{
    if (!videoWindow) return;
    bool fromBuffer = false;
    cv::Mat bm = videoWindow->SavePicture(fromBuffer);
    auto imageLoading = std::make_unique<CImageLoadingFormat>();
    try
    {
        imageLoading->SetPicture(bm);
        CSavePicture::SavePicture(nullptr, imageLoading.get(), filename);
    }
    catch (...) {}
}

// ============================================================================
// Divers
// ============================================================================
void CShowElement::IsNextPicture(const bool& value)
{
    bitmapWindow->SetNextPictureMove(value);
}

void CShowElement::IdleFunction()
{
    if (refreshSlider && videoSlider->IsShown())
        videoSlider->Refresh();
    refreshSlider = false;
}