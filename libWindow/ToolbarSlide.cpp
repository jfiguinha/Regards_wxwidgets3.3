#include "header.h"
#include <ClosedHandCursor.h>
#include <LibResource.h>
#include <wx/graphics.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "SliderInterface.h"
#include "ToolbarSlide.h"
#include "WindowMain.h"


using namespace Regards::Window;

namespace {
    constexpr int SLIDER_TRACK_HEIGHT = 4;
    constexpr int SLIDER_THUMB_RADIUS = 7;
    constexpr int SLIDER_TRACK_MARGIN = 9;
    constexpr int SLIDER_TEXT_MARGIN = 4;


    wxColour GetButtonAccent(const CThemeSlider& theme) {
        return theme.colorInactifReplacement;
    }

    wxColour GetButtonBackground(const CThemeSlider& theme) {
        return theme.colorActifReplacement;

        const wxColour back = theme.colorBack;

        const int luminance =
            (back.Red() * 299 + back.Green() * 587 + back.Blue() * 114) / 1000;

        return luminance < 128 ? wxColour(235, 238, 242) : wxColour(255, 255, 255);
    }

    wxColour GetSliderAccent(const CThemeSlider& theme) {
        return theme.rectanglePast;
    }

    wxColour GetSliderBackground(const CThemeSlider& theme) {
        const wxColour back = theme.colorBack;

        const int luminance =
            (back.Red() * 299 + back.Green() * 587 + back.Blue() * 114) / 1000;

        return luminance < 128 ? wxColour(235, 238, 242) : wxColour(255, 255, 255);
    }

    wxColour GetSliderRail(const CThemeSlider& theme) {
        return theme.rectangleNext;
    }

    void DrawSliderIcons(wxGraphicsContext* gc, const wxRect& minusRect,
        const wxRect& plusRect, const int centerY,
        const wxColour& color) {
        if (gc == nullptr) return;

        wxPen pen(color, 2);
        pen.SetCap(wxCAP_ROUND);

        gc->SetPen(pen);
        gc->SetBrush(*wxTRANSPARENT_BRUSH);

        // Bouton moins
        wxGraphicsPath pathMinus = gc->CreatePath();

        pathMinus.MoveToPoint(minusRect.x + 3, centerY);

        pathMinus.AddLineToPoint(minusRect.x + minusRect.width - 3, centerY);

        gc->StrokePath(pathMinus);

        // Bouton plus
        wxGraphicsPath pathPlus = gc->CreatePath();

        pathPlus.MoveToPoint(plusRect.x + 3, centerY);

        pathPlus.AddLineToPoint(plusRect.x + plusRect.width - 3, centerY);

        pathPlus.MoveToPoint(plusRect.x + plusRect.width / 2, plusRect.y + 3);

        pathPlus.AddLineToPoint(plusRect.x + plusRect.width / 2,
            plusRect.y + plusRect.height - 3);

        gc->StrokePath(pathPlus);
    }
}  // namespace

// ============================================================
// Constructeur
// ============================================================

CToolbarSlide::CToolbarSlide(const CThemeSlider& themeSlider,
    CSliderInterface* eventInterface) {
    y = 0;

    colorBackground = false;
    position = 0;

    this->eventInterface = eventInterface;
    this->themeSlider = themeSlider;

    positionButton = wxRect(0, 0, 0, 0);

    hCursorHand = CResourceCursor::GetClosedHand();

    mouseBlock = false;
    captureBall = false;

    minusRect = wxRect(0, 0, 0, 0);
    plusRect = wxRect(0, 0, 0, 0);

    minusPressed = false;
    plusPressed = false;
}

// ============================================================
// Dimensions
// ============================================================

void CToolbarSlide::Resize(const int& tailleX, const int& tailleY) {
    themeSlider.SetWidth(tailleX);
    themeSlider.SetHeight(tailleY);
}

int CToolbarSlide::GetWidth() { return themeSlider.GetWidth(); }

int CToolbarSlide::GetHeight() { return themeSlider.GetHeight(); }

// ============================================================
// Gestion de la position
// ============================================================

void CToolbarSlide::ChangePosition(const int& delta) {
    if (tabValue.empty()) return;

    position =
        std::clamp(position + delta, 0, static_cast<int>(tabValue.size()) - 1);

    if (eventInterface != nullptr) eventInterface->SlidePosChange(position, L"");
}

void CToolbarSlide::ZoomPos(const int& position) {
    if (eventInterface != nullptr) eventInterface->ZoomPos(position);
}

void CToolbarSlide::SetPosition(const int& iPos) {
    if (tabValue.empty()) {
        position = 0;
        return;
    }

    position = std::clamp(iPos, 0, static_cast<int>(tabValue.size()) - 1);
}

int CToolbarSlide::GetNbValue() { return static_cast<int>(tabValue.size()); }

int CToolbarSlide::GetPosition() { return position; }

void CToolbarSlide::SetTabValue(vector<int> value) {
    tabValue = std::move(value);

    if (tabValue.empty())
        position = 0;
    else
        position = std::clamp(position, 0, static_cast<int>(tabValue.size()) - 1);
}

// ============================================================
// Calcul de la position du curseur
// ============================================================

void CToolbarSlide::CalculZoomPosition(const int& x) {
    if (tabValue.size() < 2 || positionSlider.width <= 0) {
        position = 0;
        return;
    }

    const double posX = static_cast<double>(x - positionSlider.x);

    const double ratio =
        std::clamp(posX / static_cast<double>(positionSlider.width), 0.0, 1.0);

    position = static_cast<int>(
        std::round(ratio * static_cast<double>(tabValue.size() - 1)));
}

// ============================================================
// Clic sur la partie gauche du rail
// ============================================================

void CToolbarSlide::ClickLeftPage(const int& x) {
    CalculZoomPosition(x);

    if (eventInterface != nullptr) eventInterface->SlidePosChange(position, L"");
}

// ============================================================
// Clic sur la partie droite du rail
// ============================================================

void CToolbarSlide::ClickRightPage(const int& x) {
    CalculZoomPosition(x);

    if (eventInterface != nullptr) eventInterface->SlidePosChange(position, L"");
}

// ============================================================
// Recherche du curseur
// ============================================================

bool CToolbarSlide::FindCirclePos(wxWindow* window, const int& y, const int& x) {
    const int posXButtonBegin = this->x + positionButton.x;
    const int posXButtonEnd = posXButtonBegin + positionButton.width;

    const int posYButtonBegin = this->y + positionButton.y;
    const int posYButtonEnd = posYButtonBegin + positionButton.height;

    return x >= posXButtonBegin && x <= posXButtonEnd &&
        y >= posYButtonBegin && y <= posYButtonEnd;
}


// ============================================================
// Clic souris
// ============================================================
void CToolbarSlide::ClickElement(wxWindow* window, const int& x, const int& y) {
    if (window == nullptr) return;

    const wxPoint pt(x - this->x, y - this->y);

    // Bouton moins
    if (minusRect.Contains(pt)) {
        minusPressed = true;
        ChangePosition(-1);
        return;
    }

    // Bouton plus
    if (plusRect.Contains(pt)) {
        plusPressed = true;
        ChangePosition(1);
        return;
    }

    const int posXButtonBegin = this->x + positionButton.x;
    const int posXButtonEnd = posXButtonBegin + positionButton.width;

    const int railLeft = this->x + posRectangle.x;
    const int railRight = railLeft + posRectangle.width;

    if (x < railLeft || x > railRight) return;

    if (FindCirclePos(window, y, x)) {
        mouseBlock = true;
        captureBall = true;

        if (!window->HasCapture()) window->CaptureMouse();

        wxSetCursor(hCursorHand);
        return;
    }

    const int localX = x - this->x;
    if (x > posXButtonEnd) {
        ClickRightPage(localX);
    }
    else if (x < posXButtonBegin) {
        ClickLeftPage(localX);
    }
}


// ============================================================
// Relâchement souris
// ============================================================

void CToolbarSlide::UnclickElement(wxWindow* window, const int& x,
    const int& y) {
    minusPressed = false;
    plusPressed = false;

    if (captureBall) {
        mouseBlock = false;
        captureBall = false;

        if (window != nullptr && window->HasCapture()) window->ReleaseMouse();
    }
}

// ============================================================
// Déplacement du curseur
// ============================================================

bool CToolbarSlide::MouseOver(wxDC* deviceContext, const int& x, const int& y) {
    if (!mouseBlock || tabValue.empty()) return false;

    wxSetCursor(hCursorHand);

    // Coordonnées locales du composant (cohérent avec positionSlider.x)
    const int localX = x - this->x;

    CalculZoomPosition(localX);

    if (eventInterface != nullptr) eventInterface->SlidePosChange(position, L"");

    return true;
}

// ============================================================
// Valeur courante
// ============================================================

int CToolbarSlide::GetPositionValue() {
    if (position >= 0 && position < static_cast<int>(tabValue.size())) {
        return tabValue[position];
    }

    return -1;
}

// ============================================================
// Dernière valeur
// ============================================================

int CToolbarSlide::GetLastValue() {
    if (!tabValue.empty()) return tabValue.back();

    return -1;
}

// ============================================================
// Première valeur
// ============================================================

int CToolbarSlide::GetFirstValue() {
    if (!tabValue.empty()) return tabValue.front();

    return -1;
}

// ============================================================
// Fond
// ============================================================

void CToolbarSlide::SetBackgroundBitmap(const wxBitmap& background) {
    colorBackground = false;
    this->background = background;
}

// ============================================================
// Ratio de position
// ============================================================

double CToolbarSlide::GetPositionRatio() const {
    if (tabValue.size() < 2 || position <= 0) return 0.0;

    return std::clamp(
        static_cast<double>(position) / static_cast<double>(tabValue.size() - 1),
        0.0, 1.0);
}

// ============================================================
// Position du curseur
// ============================================================

void CToolbarSlide::CalculPositionButton() {
    if (positionSlider.width <= 0) {
        positionButton = wxRect(0, 0, 0, 0);
        return;
    }

    const double ratio = GetPositionRatio();

    const int thumbX =
        positionSlider.x + static_cast<int>(ratio * positionSlider.width);

    CalculPositionButton(thumbX);
}

void CToolbarSlide::CalculPositionButton(const int& x) {
    const int buttonWidth = themeSlider.GetButtonWidth();

    const int buttonHeight = themeSlider.GetButtonHeight();

    const int xPos = x - buttonWidth / 2;

    const int yPos =
        positionSlider.y + (positionSlider.height - buttonHeight) / 2;

    positionButton.x = xPos;
    positionButton.y = yPos;

    positionButton.width = buttonWidth;
    positionButton.height = buttonHeight;
}

// ============================================================
// Rendu du rail moderne
// ============================================================

void CToolbarSlide::DrawShapeElement(wxGraphicsContext* gc, const wxRect& rc) {
    if (gc == nullptr || rc.width <= 0) return;

    const double railHeight = SLIDER_TRACK_HEIGHT;

    const double railRadius = railHeight / 2.0;

    const double ratio = GetPositionRatio();

    const double progressWidth = static_cast<double>(rc.width) * ratio;

    // Rail inactif
    gc->SetPen(*wxTRANSPARENT_PEN);

    gc->SetBrush(wxBrush(GetSliderRail(themeSlider)));

    gc->DrawRoundedRectangle(rc.x, rc.y, rc.width, railHeight, railRadius);

    // Rail actif
    if (progressWidth > 0.0) {
        gc->SetBrush(wxBrush(GetSliderAccent(themeSlider)));

        gc->DrawRoundedRectangle(rc.x, rc.y, progressWidth, railHeight,
            std::min(railRadius, progressWidth / 2.0));
    }
}

// ============================================================
// Rendu du curseur
// ============================================================

void CToolbarSlide::DrawThumb(wxGraphicsContext* gc, const wxRect& rcButton) {
    if (gc == nullptr) return;

    const double centerX = rcButton.x + rcButton.width / 2.0;

    const double centerY = rcButton.y + rcButton.height / 2.0;

    const double radius =
        mouseBlock ? SLIDER_THUMB_RADIUS + 1.0 : SLIDER_THUMB_RADIUS;

    gc->SetBrush(wxBrush(GetSliderBackground(themeSlider)));

    gc->SetPen(wxPen(GetSliderAccent(themeSlider), 2));

    gc->DrawEllipse(centerX - radius, centerY - radius, radius * 2.0,
        radius * 2.0);
}

// ============================================================
// Couleur du curseur
// ============================================================

wxColour CToolbarSlide::GetThumbColour() const {
    return GetSliderBackground(themeSlider);
}

// ============================================================
// Bouton moderne
// ============================================================

void CToolbarSlide::DrawModernButton(wxGraphicsContext* gc, const wxRect& rc,
    bool plus, bool pressed) {
    if (gc == nullptr || rc.IsEmpty()) return;

    const double centerX = rc.x + rc.width / 2.0;

    const double centerY = rc.y + rc.height / 2.0;

    wxColour glyphColor = GetButtonAccent(themeSlider);

    if (pressed) glyphColor = GetButtonBackground(themeSlider);

    wxPen pen(glyphColor, 2);
    pen.SetCap(wxCAP_ROUND);

    gc->SetPen(pen);
    gc->SetBrush(*wxTRANSPARENT_BRUSH);

    // Trait horizontal
    gc->StrokeLine(rc.x + 3, centerY, rc.x + rc.width - 3, centerY);

    // Trait vertical
    if (plus) {
        gc->StrokeLine(centerX, rc.y + 3, centerX, rc.y + rc.height - 3);
    }
}

// ============================================================
// Rendu principal du slider
// ============================================================

void CToolbarSlide::RenderSlide(wxDC* dc, const int& width, const int& height,
    const int& x, const int& y) {
    if (dc == nullptr) return;

    if (tabValue.empty()) return;

    positionSlider.x = SLIDER_TRACK_MARGIN;

    positionSlider.width = std::max(0, width - SLIDER_TRACK_MARGIN * 2);

    positionSlider.y = (height - SLIDER_TRACK_HEIGHT) / 2;

    positionSlider.height = SLIDER_TRACK_HEIGHT;

    CalculPositionButton();

    std::unique_ptr<wxGraphicsContext> gc(
        wxGraphicsContext::CreateFromUnknownDC(*dc));

    if (!gc) return;

    wxRect rcRail = positionSlider;
    rcRail.Offset(x, y);

    DrawShapeElement(gc.get(), rcRail);

    wxRect rcThumb = positionButton;
    rcThumb.Offset(x, y);

    DrawThumb(gc.get(), rcThumb);
}

// ============================================================
// Rendu complet
// ============================================================

void CToolbarSlide::DrawButton(wxDC* dc, const int& x, const int& y) {
    if (dc == nullptr) return;

    const int sWidth = themeSlider.GetWidth();

    const int sHeight = themeSlider.GetHeight();

    if (sWidth <= 0 || sHeight <= 0) return;

    // Fond
    if (colorBackground) {
        wxRect rc(x, y, sWidth, sHeight);

        CWindowMain::FillRect(dc, rc, themeSlider.colorBack);
    }
    else if (background.IsOk()) {
        dc->DrawBitmap(background, x, y);
    }

    // Valeurs
    const wxString firstValue = wxString::Format("%d", GetPositionValue());

    const wxString lastValue = wxString::Format("%d", GetLastValue());

    wxSize renderFirst =
        CWindowMain::GetSizeTexte(dc, firstValue, themeSlider.font);

    wxSize renderLast =
        CWindowMain::GetSizeTexte(dc, lastValue, themeSlider.font);

    const int buttonWidth = themeSlider.GetButtonWidth();

    const int margin = SLIDER_TEXT_MARGIN;

    const int centerY = sHeight / 2;

    // --------------------------------------------------------
    // Libellé courant
    // --------------------------------------------------------

    const int xFirstText = margin;

    const int yFirstText = (sHeight - renderFirst.y) / 2;

    CWindowMain::DrawTexte(dc, firstValue, x + xFirstText, y + yFirstText,
        themeSlider.font);

    // --------------------------------------------------------
    // Libellé maximum
    // --------------------------------------------------------

    const int xLastText = sWidth - renderLast.x - margin;

    const int yLastText = (sHeight - renderLast.y) / 2;

    CWindowMain::DrawTexte(dc, lastValue, x + xLastText, y + yLastText,
        themeSlider.font);

    // --------------------------------------------------------
    // Bouton moins
    // --------------------------------------------------------

    minusRect = wxRect(xFirstText + renderFirst.x + margin,
        centerY - buttonWidth / 2, buttonWidth, buttonWidth);

    // --------------------------------------------------------
    // Bouton plus
    // --------------------------------------------------------

    plusRect = wxRect(xLastText - margin - buttonWidth, centerY - buttonWidth / 2,
        buttonWidth, buttonWidth);

    // --------------------------------------------------------
    // Rail : entre les deux boutons
    // --------------------------------------------------------

    const int railLeft = minusRect.GetRight() + margin;

    const int railRight = plusRect.x - margin;

    const int railWidth = std::max(0, railRight - railLeft);

    posRectangle = wxRect(railLeft, 0, railWidth, sHeight);

    // --------------------------------------------------------
    // Position du rail
    // --------------------------------------------------------

    positionSlider.x = posRectangle.x + SLIDER_TRACK_MARGIN;

    positionSlider.y = centerY - SLIDER_TRACK_HEIGHT / 2;

    positionSlider.width =
        std::max(0, posRectangle.width - SLIDER_TRACK_MARGIN * 2);

    positionSlider.height = SLIDER_TRACK_HEIGHT;

    CalculPositionButton();

    // --------------------------------------------------------
    // Rendu vectoriel
    // --------------------------------------------------------

    std::unique_ptr<wxGraphicsContext> gc(
        wxGraphicsContext::CreateFromUnknownDC(*dc));

    if (!gc) return;

    // Rail et curseur
    wxRect rcRail = positionSlider;
    rcRail.Offset(x, y);

    DrawShapeElement(gc.get(), rcRail);

    wxRect rcThumb = positionButton;
    rcThumb.Offset(x + posRectangle.x, y);

    // Le curseur est dessiné dans les coordonnées de la barre.
    // On corrige son décalage pour conserver son positionnement.
    rcThumb.x = x + positionButton.x;
    rcThumb.y = y + positionButton.y;

    DrawThumb(gc.get(), rcThumb);

    // Boutons
    wxRect rcMinus = minusRect;
    rcMinus.Offset(x, y);

    wxRect rcPlus = plusRect;
    rcPlus.Offset(x, y);

    DrawModernButton(gc.get(), rcMinus, false, minusPressed);

    DrawModernButton(gc.get(), rcPlus, true, plusPressed);
}
