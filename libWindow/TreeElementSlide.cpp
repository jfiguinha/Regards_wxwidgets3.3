#include "header.h"
#include "TreeElementSlide.h"
#include <TreeElementValue.h>
#include "WindowMain.h"
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <RegardsConfigParam.h>
#include <ParamInit.h>

using namespace Regards::Window;
using namespace Regards::UI;
using namespace std;

CTreeElementSlide::CTreeElementSlide(CTreeElementSlideInterface* eventInterface)
    : tabValue(nullptr), m_pSliderModern(nullptr), m_pParentWindow(nullptr), skinMode(0)
{
    this->eventInterface = eventInterface;
    exifKey = L"";
    height = 0;
    position = 0;

    // Récupération des paramètres de l'application
    if (auto* regardsParam = CParamInit::getInstance())
        skinMode = regardsParam->GetSkinWindowMode();

    // Détection de la préférence globale du système
    isDarkTheme = wxSystemSettings::GetAppearance().IsDark();
}

CTreeElementSlide::~CTreeElementSlide()
{
    if (m_pSliderModern != nullptr)
    {
        m_pSliderModern->Destroy();
        m_pSliderModern = nullptr;
    }
}

CTreeElementSlide& CTreeElementSlide::operator=(const CTreeElementSlide& other)
{
    if (this != &other)
    {
        visible = other.visible;
        xPos = other.xPos;
        yPos = other.yPos;
        numRow = other.numRow;
        numColumn = other.numColumn;
        themeSlide = other.themeSlide;
        height = other.height;
        position = other.position;
        tabValue = other.tabValue;
        exifKey = other.exifKey;
        eventInterface = other.eventInterface;
        skinMode = other.skinMode;
        isDarkTheme = other.isDarkTheme;
    }
    return *this;
}

void CTreeElementSlide::SetPosition(const int& position)
{
    this->position = position;
    if (m_pSliderModern != nullptr)
    {
        m_pSliderModern->SetValue(position);
    }
}

bool CTreeElementSlide::TestEgality(CTreeElementValue* value1, CTreeElementValue* value2, int type)
{
    if (!value1 || !value2) return false;

    switch (type)
    {
    case TYPE_ELEMENT_INT:
        return (static_cast<CTreeElementValueInt*>(value1)->GetValue() == static_cast<CTreeElementValueInt*>(value2)->GetValue());
    case TYPE_ELEMENT_FLOAT:
        return (static_cast<CTreeElementValueFloat*>(value1)->GetValue() == static_cast<CTreeElementValueFloat*>(value2)->GetValue());
    case TYPE_ELEMENT_BOOL:
        return (static_cast<CTreeElementValueBool*>(value1)->GetValue() == static_cast<CTreeElementValueBool*>(value2)->GetValue());
    default:
        break;
    }
    return false;
}

void CTreeElementSlide::SetInitValue(CTreeElementValue* value)
{
    if (!tabValue || !value) return;

    for (size_t i = 0; i < tabValue->size(); i++)
    {
        if (TestEgality(tabValue->at(i), value, value->GetType()))
        {
            this->position = static_cast<int>(i);
            break;
        }
    }

    if (m_pSliderModern != nullptr)
    {
        m_pSliderModern->SetValue(this->position);
    }
}

void CTreeElementSlide::SetTheme(CThemeTreeSlide* theme)
{
    if (theme) themeSlide = *theme;
}

void CTreeElementSlide::SetExifKey(const wxString& exifKey)
{
    this->exifKey = exifKey;
}

void CTreeElementSlide::SetTabValue(vector<CTreeElementValue*>* value)
{
    tabValue = value;
}

void CTreeElementSlide::SetVisible(const bool& visible)
{
    this->visible = visible;
    if (!visible && m_pSliderModern != nullptr)
    {
        m_pSliderModern->Show(false);
    }
}

void CTreeElementSlide::SetZoneSize(const int& width, const int& height)
{
    themeSlide.SetWidth(width);
    themeSlide.SetHeight(height);
    this->height = height;
}

void CTreeElementSlide::SetBackgroundColor(const wxColour& color)
{
    themeSlide.color = color;
}

void CTreeElementSlide::SetElementPos(const int& x, const int& y)
{
    xPos = x;
    yPos = y;
}

void CTreeElementSlide::TestMaxMinValue()
{
    if (!tabValue || tabValue->empty()) return;

    if (position >= (int)tabValue->size())
        position = static_cast<int>(tabValue->size()) - 1;
    if (position < 0)
        position = 0;
}

CTreeElementValue* CTreeElementSlide::GetFirstValue()
{
    if (tabValue && !tabValue->empty()) return tabValue->at(0);
    return nullptr;
}

CTreeElementValue* CTreeElementSlide::GetLastValue()
{
    if (tabValue && !tabValue->empty()) return tabValue->at(tabValue->size() - 1);
    return nullptr;
}

CTreeElementValue* CTreeElementSlide::GetPositionValue()
{
    if (tabValue && position >= 0 && position < (int)tabValue->size())
        return tabValue->at(position);
    return nullptr;
}

bool CTreeElementSlide::MouseBlock()
{
    return mouseBlock;
}

void CTreeElementSlide::MouseOver(wxDC* deviceContext, const int& x, const int& y, bool& update)
{
    if (mouseBlock && tabValue && !tabValue->empty())
    {
        int relativeX = x - (xPos + slidePos.x);
        int trackLeft = 9;
        int trackRight = slidePos.width - 9;
        int trackWidth = trackRight - trackLeft;

        if (trackWidth > 0)
        {
            double t = std::clamp(double(relativeX - trackLeft) / double(trackWidth), 0.0, 1.0);
            int newPosition = static_cast<int>(std::lround(t * (tabValue->size() - 1)));

            if (newPosition != position)
            {
                position = newPosition;
                TestMaxMinValue();

                // Notification du filtre (ex: OpenCV) pendant le déplacement
                eventInterface->SlidePosChange(this, position, tabValue->at(position), exifKey);
                update = true; // Demande le rafraîchissement de l'arbre
            }
        }
    }
}

void CTreeElementSlide::UnclickElement(wxWindow* window, const int& x, const int& y)
{
    if (mouseBlock)
    {
        mouseBlock = false;
        if (window && window->GetCapture() == window)
        {
            window->ReleaseMouse();
        }
        if (window)
        {
            window->Refresh();
        }
    }
}
void CTreeElementSlide::ClickElement(wxWindow* window, const int& x, const int& y)
{
    if (window == nullptr || !tabValue || tabValue->empty()) return;
    m_pParentWindow = window;

    // Vérifie si le clic est bien dans la zone du rail du slider
    if (x >= (slidePos.x + xPos) && x < (slidePos.x + slidePos.width + xPos))
    {
        mouseBlock = true;  // On active l'état de drag
        if (!window->HasCapture())
        {
            window->CaptureMouse();
        }

        // Calcul et mise à jour de la position selon le X cliqué
        int relativeX = x - (xPos + slidePos.x);
        int trackLeft = 9;
        int trackRight = slidePos.width - 9;
        int trackWidth = trackRight - trackLeft;

        if (trackWidth > 0)
        {
            double t = std::clamp(double(relativeX - trackLeft) / double(trackWidth), 0.0, 1.0);
            position = static_cast<int>(std::lround(t * (tabValue->size() - 1)));
            TestMaxMinValue();

            // Notification immédiate du changement
            eventInterface->SlidePosChange(this, position, tabValue->at(position), exifKey);
            window->Refresh();
        }
    }
    // Clic sur le bouton Moins (-)
    else if (x >= (moinsPos.x + xPos) && x < (moinsPos.width + moinsPos.x + xPos))
    {
        position--;
        TestMaxMinValue();
        eventInterface->SlidePosChange(this, position, tabValue->at(position), exifKey);
        window->Refresh();
    }
    // Clic sur le bouton Plus (+)
    else if (x >= (plusPos.x + xPos) && x < (plusPos.width + plusPos.x + xPos))
    {
        position++;
        TestMaxMinValue();
        eventInterface->SlidePosChange(this, position, tabValue->at(position), exifKey);
        window->Refresh();
    }
}

void CTreeElementSlide::OnSliderChanged(wxCommandEvent& event)
{
    position = event.GetInt();
    TestMaxMinValue();

    if (eventInterface && tabValue)
    {
        eventInterface->SlidePosChange(this, position, tabValue->at(position), exifKey);
    }

    if (m_pParentWindow)
    {
        m_pParentWindow->Refresh();
    }
}

void CTreeElementSlide::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
    if (deviceContext == nullptr || !tabValue || tabValue->empty()) return;

    int sWidth = themeSlide.GetWidth();
    int sHeight = themeSlide.GetHeight();

    if (!slideCacheBuffer.IsOk() || slideCacheBuffer.GetWidth() != sWidth || slideCacheBuffer.GetHeight() != sHeight)
    {
        slideCacheBuffer = wxBitmap(sWidth, sHeight);
    }

    wxMemoryDC memDC(slideCacheBuffer);

    wxRect rc(0, 0, sWidth, sHeight);
    CWindowMain::FillRect(&memDC, rc, themeSlide.color);

    wxSize renderLast = CWindowMain::GetSizeTexte(deviceContext, GetLastValue()->GetStringValue(), themeSlide.font);
    wxSize renderFirst = CWindowMain::GetSizeTexte(deviceContext, GetPositionValue()->GetStringValue(), themeSlide.font);

    slidePos.x = renderLast.x + themeSlide.GetButtonWidth();
    slidePos.y = 0;
    slidePos.width = sWidth - (renderLast.x * 2) - (themeSlide.GetButtonWidth() * 2);
    slidePos.height = sHeight;

    // Déduction du thème courant
    ModernTheme theme = IsDarkTheme() ? ModernTheme::Dark() : ModernTheme::Light();

    // 2. Mode repos : Rendu vectoriel du rail basé sur le thème dynamique choisi
    if (m_pSliderModern == nullptr || !m_pSliderModern->IsShown())
    {
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(memDC));
        if (gc)
        {
            int trackY = sHeight / 2;
            int trackLeft = slidePos.x + 9;
            int trackRight = slidePos.x + slidePos.width - 9;
            int trackWidth = trackRight - trackLeft;

            // Rail d'arrière plan (Inactif)
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(theme.border));
            gc->DrawRoundedRectangle(trackLeft, trackY - 2, trackWidth, 4, 2);

            // Rail Actif (Progression proportionnelle)
            double pct = static_cast<double>(position) / static_cast<double>(tabValue->size() - 1);
            int thumbX = trackLeft + static_cast<int>(pct * trackWidth);

            gc->SetBrush(wxBrush(theme.accent));
            gc->DrawRoundedRectangle(trackLeft, trackY - 2, std::max(0, thumbX - trackLeft), 4, 2);

            // Curseur (Ellipse)
            gc->SetBrush(wxBrush(theme.surface));
            gc->SetPen(wxPen(theme.accent, 2));
            gc->DrawEllipse(thumbX - 7, trackY - 7, 14, 14);
        }
    }

    // 3. Rendu du texte des valeurs courantes et extrêmes (Utilise theme.text pour la couleur de premier plan si applicable)
    int yMedium = (sHeight - renderFirst.y) / 2;
    CWindowMain::DrawTexte(&memDC, GetPositionValue()->GetStringValue(), 0, yMedium, themeSlide.font);
    yMedium = (sHeight - renderLast.y) / 2;
    CWindowMain::DrawTexte(&memDC, GetLastValue()->GetStringValue(), sWidth - renderLast.x, yMedium, themeSlide.font);
    // 4. Dessin vectoriel épuré des chevrons Moins (-) et Plus (+) synchronisés sur le thème
    std::unique_ptr<wxGraphicsContext> gcIcons(wxGraphicsContext::Create(memDC));

    if (gcIcons)
    {
        gcIcons->SetPen(wxPen(theme.disabled, 2, wxPENSTYLE_SOLID));
        gcIcons->SetBrush(*wxTRANSPARENT_BRUSH);
        int btnW = themeSlide.GetButtonWidth();
        int centerY = sHeight / 2;
        moinsPos.x = renderLast.x + themeSlide.GetMarge();
        moinsPos.y = centerY - (btnW / 2);
        moinsPos.width = btnW;
        moinsPos.height = btnW;
        plusPos.x = sWidth - renderLast.x - btnW - themeSlide.GetMarge();
        plusPos.y = centerY - (btnW / 2);
        plusPos.width = btnW;
        plusPos.height = btnW;
        // Icône Moins
        wxGraphicsPath pathMoins = gcIcons->CreatePath();
        pathMoins.MoveToPoint(moinsPos.x + 3, centerY);
        pathMoins.AddLineToPoint(moinsPos.x + moinsPos.width - 3, centerY);
        gcIcons->StrokePath(pathMoins);
        // Icône Plus
        wxGraphicsPath pathPlus = gcIcons->CreatePath();
        pathPlus.MoveToPoint(plusPos.x + 3, centerY);
        pathPlus.AddLineToPoint(plusPos.x + plusPos.width - 3, centerY);
        pathPlus.MoveToPoint(plusPos.x + (plusPos.width / 2), centerY - (plusPos.height / 2) + 3);
        pathPlus.AddLineToPoint(plusPos.x + (plusPos.width / 2), centerY + (plusPos.height / 2) - 3);
        gcIcons->StrokePath(pathPlus);
    }
    memDC.SelectObject(wxNullBitmap);
    // Sortie finale sur le contexte d'affichage principal
    deviceContext->DrawBitmap(slideCacheBuffer, x, y);
}