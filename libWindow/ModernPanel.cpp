#include <header.h>
#include "ModernPanel.h"
#include <wx/graphics.h>
#include "MainTheme.h"
#include "MainThemeInit.h"

namespace Regards::Window
{
    namespace
    {
        constexpr int PANEL_ID = 1;

        //const wxColour PANEL_BACKGROUND(40, 42, 46); //lineColorTop
        //const wxColour HEADER_BACKGROUND(48, 50, 55); //lineColorBottom
        //const wxColour HEADER_FOREGROUND(230, 232, 235); //actifTop
        //const wxColour ACCENT_COLOUR(80, 160, 240); //actifBottom
        //const wxColour HOVER_COLOUR(65, 68, 74); //colorActifReplacement
    }

    // =========================================================
    // CModernPanelHeader
    // =========================================================

    CModernPanelHeader::CModernPanelHeader(
        wxWindow* parent,
        const CThemeToolbar& themeClickInfosToolbar,
        const wxString& title,
        bool vertical)
        : wxPanel(parent),
        title_(title),
        vertical_(vertical),
        background_(themeClickInfosToolbar.button.lineColorBottom), //HEADER_BACKGROUND
        foreground_(themeClickInfosToolbar.button.actifTop), //HEADER_FOREGROUND
        accent_(themeClickInfosToolbar.button.actifBottom), //ACCENT_COLOUR
        hover_(themeClickInfosToolbar.button.colorActifReplacement) //HOVER_COLOUR

        /*
        *         //const wxColour PANEL_BACKGROUND(40, 42, 46); //lineColorTop
        //const wxColour HEADER_BACKGROUND(48, 50, 55); //lineColorBottom
        //const wxColour HEADER_FOREGROUND(230, 232, 235); //actifTop
        //const wxColour ACCENT_COLOUR(80, 160, 240); //actifBottom
        //const wxColour HOVER_COLOUR(65, 68, 74); //colorActifReplacement
        */
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        SetMinSize(vertical_
            ? wxSize(FromDIP(38), -1)
            : wxSize(-1, FromDIP(36)));

        Bind(wxEVT_PAINT,
            &CModernPanelHeader::OnPaint, this);

        Bind(wxEVT_MOTION,
            &CModernPanelHeader::OnMouseMove, this);

        Bind(wxEVT_LEAVE_WINDOW,
            &CModernPanelHeader::OnMouseLeave, this);

        Bind(wxEVT_LEFT_DOWN,
            &CModernPanelHeader::OnMouseDown, this);
    }

    void CModernPanelHeader::SetTitle(const wxString& title)
    {
        title_ = title;
        Refresh(false);
    }

    void CModernPanelHeader::SetCollapsed(bool collapsed)
    {
        collapsed_ = collapsed;
        Refresh(false);
    }

    wxRect CModernPanelHeader::GetCollapseRect() const
    {
        const int buttonSize = FromDIP(28);
        const int margin = FromDIP(4);

        if (vertical_)
        {
            return wxRect(
                (GetClientSize().x - buttonSize) / 2,
                GetClientSize().y - buttonSize - margin,
                buttonSize,
                buttonSize);
        }

        return wxRect(
            GetClientSize().x - buttonSize - margin,
            (GetClientSize().y - buttonSize) / 2,
            buttonSize,
            buttonSize);
    }


    void CModernPanelHeader::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);

        const wxSize size = GetClientSize();

        dc.SetBackground(wxBrush(background_));
        dc.Clear();

        dc.SetPen(*wxTRANSPARENT_PEN);

        // --- Rendu du Texte ---
        // --- Rendu du Texte ---
        wxFont font(FromDIP(10), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_MEDIUM);
        dc.SetFont(font);
        dc.SetTextForeground(foreground_);

        if (!vertical_)
        {
            // Mode horizontal : texte classique centré verticalement
            dc.DrawText(title_, FromDIP(12), (size.y - dc.GetCharHeight()) / 2);
        }
        else
        {
            // Mode vertical (ouvert OU fermé) : le texte doit toujours être visible
            if (collapsed_)
            {
                // Quand le panneau est fermé, le bouton occupe le bas (GetCollapseRect)
                // On centre le texte verticalement dans l'espace restant au-dessus du bouton
                const int disponibleY = GetCollapseRect().y;

                // Calcul de la taille du texte simulée pour un centrage propre
                // Note : DrawRotatedText à 90° dessine du bas vers le haut depuis son point d'ancrage
                int texteWidth = 0, texteHeight = 0;
                dc.GetTextExtent(title_, &texteWidth, &texteHeight);

                int targetY = (disponibleY + texteWidth) / 2;
                if (targetY > disponibleY - FromDIP(5))
                    targetY = disponibleY - FromDIP(5); // Sécurité marge

                dc.DrawRotatedText(title_, FromDIP(12), targetY, 90);
            }
            else
            {
                // Quand le panneau est ouvert, on utilise le centrage global classique au milieu de la hauteur
                dc.DrawRotatedText(title_, FromDIP(12), size.y / 2, 90);
            }
        }


        // --- Rendu du Fond des Boutons (Hover) ---
        const wxRect collapseRect = GetCollapseRect();
        if (hoverCollapse_)
        {
            dc.SetBrush(wxBrush(hover_));
            dc.DrawRoundedRectangle(collapseRect, FromDIP(5));
        }

        // =========================================================
        // INITIALISATION DE L'ANTIALIASING VIA wxGraphicsContext
        // =========================================================
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (gc)
        {
            // Activation explicite de l'anti-crénelage pour les tracés
            gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

            // Configuration du crayon pour les formes (blanc / foreground_)
            wxGraphicsPen pen = gc->CreatePen(wxPen(foreground_, FromDIP(2), wxPENSTYLE_SOLID));
            gc->SetPen(pen);

            // 1. Dessin de la flèche (Triangle) de réduction / développement
            const double cx = collapseRect.x + collapseRect.width / 2.0;
            const double cy = collapseRect.y + collapseRect.height / 2.0;

            wxGraphicsPath path = gc->CreatePath();

            if (!vertical_)
            {
                if (collapsed_)
                {
                    // Flèche vers la droite (>)
                    path.MoveToPoint(cx - FromDIP(3), cy - FromDIP(5));
                    path.AddLineToPoint(cx + FromDIP(3), cy);
                    path.AddLineToPoint(cx - FromDIP(3), cy + FromDIP(5));
                }
                else
                {
                    // Flèche vers le bas (v)
                    path.MoveToPoint(cx - FromDIP(5), cy - FromDIP(2));
                    path.AddLineToPoint(cx, cy + FromDIP(3));
                    path.AddLineToPoint(cx + FromDIP(5), cy - FromDIP(2));
                }
            }
            else
            {
                if (collapsed_)
                {
                    // Flèche vers le bas (v) pour le mode vertical plié
                    path.MoveToPoint(cx - FromDIP(5), cy - FromDIP(3));
                    path.AddLineToPoint(cx, cy + FromDIP(3));
                    path.AddLineToPoint(cx + FromDIP(5), cy - FromDIP(3));
                }
                else
                {
                    // Flèche vers le haut (^) pour le mode vertical déplié
                    path.MoveToPoint(cx - FromDIP(5), cy + FromDIP(3));
                    path.AddLineToPoint(cx, cy - FromDIP(3));
                    path.AddLineToPoint(cx + FromDIP(5), cy + FromDIP(3));
                }
            }
            gc->StrokePath(path);

        }

        // --- Ligne de bordure (Accent) ---
        dc.SetPen(wxPen(accent_, FromDIP(1)));
        if (vertical_)
            dc.DrawLine(size.x - 1, 0, size.x - 1, size.y);
        else
            dc.DrawLine(0, size.y - 1, size.x, size.y - 1);
    }


    void CModernPanelHeader::OnMouseMove(wxMouseEvent& event)
    {
        const wxPoint pos = event.GetPosition();

        const bool newCollapse =
            GetCollapseRect().Contains(pos);

        if (newCollapse != hoverCollapse_)
        {
            hoverCollapse_ = newCollapse;
            Refresh(false);
        }

        SetCursor(wxCursor(
            hoverCollapse_
            ? wxCURSOR_HAND
            : wxCURSOR_ARROW));

        event.Skip();
    }

    void CModernPanelHeader::OnMouseLeave(wxMouseEvent&)
    {
        hoverCollapse_ = false;

        SetCursor(wxCursor(wxCURSOR_ARROW));
        Refresh(false);
    }

    void CModernPanelHeader::OnMouseDown(wxMouseEvent& event)
    {
        const wxPoint pos = event.GetPosition();

        // Panneau ferme : un clic n'importe ou sur la barre le rouvre
        if (collapsed_ || GetCollapseRect().Contains(pos))
        {
            collapsed_ = !collapsed_;
            Refresh(false);

            if (OnCollapse)
                OnCollapse();

            return;
        }
    }

    // =========================================================
    // CModernPanel
    // =========================================================

    CModernPanel::CModernPanel(
        wxWindow* parent,
        const CThemePane& theme,
        const CThemeToolbar& themeClickInfosToolbar,
        const wxString& windowName,
        wxWindowID id,
        const wxString& title,
        bool visible,
        bool vertical)
        : CWindowMain(windowName, parent, id),
        isPanelVisible_(visible),
        isVertical_(vertical),
        panelId_(id)
    {
        SetBackgroundColour(theme.colorBack);

        header_ = new CModernPanelHeader(
            this,
            themeClickInfosToolbar,
            title,
            vertical);

        // visible = true : panneau ouvert ; false : demarre ferme (barre d'en-tete seule)
        isCollapsed_ = !visible;
        header_->SetCollapsed(isCollapsed_);

        content_ = new wxPanel(this);
        content_->SetBackgroundColour(theme.colorBack);

        panelSizer_ = new wxBoxSizer(
            vertical ? wxHORIZONTAL : wxVERTICAL);

        panelSizer_->Add(
            header_,
            0,
            wxEXPAND);

        panelSizer_->Add(
            content_,
            1,
            wxEXPAND);

        SetSizer(panelSizer_);

        header_->OnCollapse = [this]()
            {
                OnHeaderCollapse();
            };

        Connect(wxID_ANY, wxEVENT_SHOWPANE,
            wxCommandEventHandler(CModernPanel::OnShowPane));

        Connect(wxID_ANY, wxEVENT_CLOSEPANE,
            wxCommandEventHandler(CModernPanel::OnClosePane));

        Connect(wxID_ANY, wxEVENT_REFRESHDATA,
            wxCommandEventHandler(CModernPanel::OnRefreshData));

        UpdatePanelLayout();
    }


    // Memes comportements que CPanelWithClickToolbar::ShowPane / ClosePaneEvent
    void CModernPanel::OnShowPane(wxCommandEvent& event)
    {
        ClickShowButton(PANE_WITHCLICKTOOLBAR);
    }

    void CModernPanel::OnClosePane(wxCommandEvent& event)
    {
        ClosePane(PANE_WITHCLICKTOOLBAR);
    }

    void CModernPanel::OnRefreshData(wxCommandEvent& event)
    {
        if (mainWindow_)
        {
            wxCommandEvent outboundEvent(wxEVENT_REFRESHDATA);
            outboundEvent.SetExtraLong(1);

            wxQueueEvent(
                mainWindow_->GetEventHandler(),
                outboundEvent.Clone());
        }
    }



    void CModernPanel::SetTitle(const wxString& title)
    {
        if (header_)
            header_->SetTitle(title);
    }

    void CModernPanel::SetWindow(CWindowMain* window)
    {
        if (window == mainWindow_)
            return;

        wxSize oldSize;

        if (mainWindow_)
        {
            oldSize = mainWindow_->GetSize();

            if (content_->GetSizer())
                content_->GetSizer()->Detach(mainWindow_);

            mainWindow_->Reparent(nullptr);
            mainWindow_->Hide();
        }

        mainWindow_ = window;

        if (mainWindow_)
        {
            mainWindow_->Reparent(content_);
            mainWindow_->SetSize(content_->GetClientSize());
            mainWindow_->Show(true);
        }

        if (!content_->GetSizer())
        {
            content_->SetSizer(
                new wxBoxSizer(wxVERTICAL));
        }

        if (mainWindow_)
        {
            content_->GetSizer()->Add(
                mainWindow_,
                1,
                wxEXPAND);
        }

        if (oldSize.IsFullySpecified() && mainWindow_)
            mainWindow_->SetSize(oldSize);

        content_->Layout();
        Resize();
    }

    wxWindow* CModernPanel::GetWindow()
    {
        return isPanelVisible_
            ? static_cast<wxWindow*>(this)
            : static_cast<wxWindow*>(header_);
    }

    wxWindow* CModernPanel::GetPaneWindow()
    {
        return content_;
    }

    CWindowMain* CModernPanel::GetContentWindow() const
    {
        return mainWindow_;
    }

    // Affiche tout le panneau (ouvert ou ferme, selon son etat actuel)
    void CModernPanel::ShowPanel()
    {
        Show(true);

        UpdatePanelLayout();
        Resize();
    }

    // Masque tout le panneau (ce n'est pas la "fermeture" : l'etat ouvert/ferme est conserve)
    void CModernPanel::HidePanel(bool refresh)
    {
        Show(false);

        if (refresh)
            Refresh(false);
    }

    void CModernPanel::SetCollapsed(bool collapsed)
    {
        if (isCollapsed_ == collapsed)
            return;

        SetOpenState(!collapsed, 1);

        if (OnCollapse)
            OnCollapse();
    }

    bool CModernPanel::IsCollapsed() const
    {
        return isCollapsed_;
    }

    bool CModernPanel::IsPanelVisible() const
    {
        return isPanelVisible_;
    }

    // Ouvre le panneau (equivalent de CPanelWithClickToolbar::ClickShowButton)
    void CModernPanel::ClickShowButton(
        const int& id,
        const int& refresh)
    {
        if (id != panelId_ && id != PANEL_ID)
            return;

        SetOpenState(true, refresh);
    }

    // Ferme le panneau : il reste visible sous forme de barre d'en-tete, d'ou on peut le rouvrir
    // (equivalent de CPanelWithClickToolbar::ClosePane, qui affichait la barre de clic)
    void CModernPanel::ClosePane(
        const int& id,
        const int& refresh)
    {
        if (id != panelId_ && id != PANEL_ID)
            return;

        SetOpenState(false, refresh);
    }

    void CModernPanel::SetOpenState(bool open, int refresh)
    {
        // Rien a faire si l'etat ne change pas (comme dans CPanelWithClickToolbar)
        if (isPanelVisible_ == open && isCollapsed_ == !open)
        {
            Resize();
            return;
        }

        isPanelVisible_ = open;
        isCollapsed_ = !open;

        if (header_)
            header_->SetCollapsed(isCollapsed_);

        UpdatePanelLayout();
        Resize();

        // Le parent doit recalculer sa mise en page avec les nouvelles GetWidth()/GetHeight()
        NotifyParent(open, refresh);
    }

    void CModernPanel::NotifyParent(bool open, int refresh)
    {
        if (!GetParent())
            return;

        auto* event = new wxCommandEvent(wxEVENT_RESIZE);
        event->SetId(GetId());
        event->SetInt(open ? 1 : 0);
        event->SetExtraLong(refresh);
        wxQueueEvent(GetParent(), event);
    }

    void CModernPanel::UpdateScreenRatio()
    {
        if (mainWindow_)
            mainWindow_->UpdateScreenRatio();

        Layout();
        Resize();
    }

    // Dimensions "voulues" par le panneau pour la mise en page du parent :
    //  - ferme : seulement l'epaisseur de la barre d'en-tete
    //  - ouvert : contenu + barre d'en-tete
    int CModernPanel::GetWidth()
    {
        const int headerWidth = header_ ? header_->GetMinSize().x : 0;

        if (isVertical_)
        {
            if (!isPanelVisible_ || mainWindow_ == nullptr)
                return headerWidth;
            return mainWindow_->GetWidth() + headerWidth;
        }

        if (!isPanelVisible_ || mainWindow_ == nullptr)
            return GetSize().x;
        return mainWindow_->GetWidth();
    }

    int CModernPanel::GetHeight()
    {
        const int headerHeight = header_ ? header_->GetMinSize().y : 0;

        if (isVertical_)
        {
            if (!isPanelVisible_ || mainWindow_ == nullptr)
                return GetSize().y;
            return mainWindow_->GetHeight();
        }

        if (!isPanelVisible_ || mainWindow_ == nullptr)
            return headerHeight;
        return mainWindow_->GetHeight() + headerHeight;
    }

    void CModernPanel::SetPanelBackground(
        const wxColour& colour)
    {
        SetBackgroundColour(colour);
        content_->SetBackgroundColour(colour);

        Refresh(false);
        content_->Refresh(false);
    }

    CModernPanel* CModernPanel::CreatePanel(
        wxWindow* parent,
        const wxString& panelLabel,
        const wxString& windowName,
        const bool& isVisible,
        const int& idPanel,
        const bool& isVertical,
        const bool& refreshButton)
    {

        CMainTheme* viewerTheme = CMainThemeInit::getInstance();
        CThemePane theme;
        viewerTheme->GetPaneTheme(&theme);
        CThemeToolbar themeClickInfosToolbar;
        viewerTheme->GetClickToolbarTheme(&themeClickInfosToolbar);

        auto* panel = new CModernPanel(
            parent,
            theme,
            themeClickInfosToolbar,
            windowName,
            idPanel,
            panelLabel,
            isVisible,
            isVertical);

        panel->refreshButton_ = refreshButton;

        return panel;
    }

    void CModernPanel::Resize()
    {
        if (!panelSizer_)
            return;

        Layout();

        if (mainWindow_ && content_->IsShown())
        {
            mainWindow_->SetSize(
                content_->GetClientRect());

            mainWindow_->Resize();
        }

        needToRefresh = true;
    }

    void CModernPanel::UpdatePanelLayout()
    {
        if (!panelSizer_)
            return;

        // Ferme : seule la barre d'en-tete reste affichee
        if (isCollapsed_)
            content_->Hide();
        else
            content_->Show();

        header_->SetMinSize(isVertical_
            ? wxSize(FromDIP(38), -1)
            : wxSize(-1, FromDIP(36)));

        panelSizer_->Layout();
        Layout();
    }

    void CModernPanel::OnHeaderCollapse()
    {
        // L'en-tete a deja bascule son propre etat : on aligne le panneau dessus
        SetCollapsed(!isCollapsed_);
    }
}