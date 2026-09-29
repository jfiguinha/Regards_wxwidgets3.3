#include <header.h>
#include "RectangleDraw.h"
#include <algorithm>
#include "RectangleFilterParameter.h"

using namespace Regards::FiltreEffet;

CRectangleDraw::CRectangleDraw() : m_isDrawing(false) { m_tousLesTraces.clear(); }
void CRectangleDraw::Reset() { m_tousLesTraces.clear(); m_isDrawing = false; }
void CRectangleDraw::MouseUp() { m_isDrawing = false; }

void CRectangleDraw::GetPoint(wxPoint& pt) {
    if (!m_tousLesTraces.empty()) pt = m_tousLesTraces.back().endPoint;
    else pt = wxPoint(0, 0);
}

void CRectangleDraw::MouseDown(CEffectParameter* effect) {
    m_isDrawing = true;
    CRectangleFilterParameter* param = (CRectangleFilterParameter*)effect;

    SShapeStyleDraw nouvelleForme;
    nouvelleForme.color = param->ConvertScalarToWxColour();
    nouvelleForme.penSize = param->penSize;
    nouvelleForme.typeBrush = param->typeBrush;
    nouvelleForme.shapeType = param->shapeType;
    nouvelleForme.opacity = param->opacity;
    nouvelleForme.isFilled = param->isFilled;

    m_tousLesTraces.push_back(nouvelleForme);
}

void CRectangleDraw::InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {
    const wxPoint point(static_cast<int>(m_lx), static_cast<int>(m_ly));
    if (!VerifierValiditerPoint(point)) return;

    MouseDown(effect);

    float realX = XRealPosition(static_cast<float>(m_lx), m_lHScroll, ratio);
    float realY = YRealPosition(static_cast<float>(m_ly), m_lVScroll, ratio);

    m_tousLesTraces.back().startPoint = wxPoint(static_cast<int>(realX), static_cast<int>(realY));
    m_tousLesTraces.back().endPoint = wxPoint(static_cast<int>(realX), static_cast<int>(realY));
}

void CRectangleDraw::MouseMove(const long& xNewSize, const long& yNewSize, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {
    if (!m_isDrawing || m_tousLesTraces.empty()) return;

    const wxPoint point(static_cast<int>(xNewSize), static_cast<int>(yNewSize));
    if (!VerifierValiditerPoint(point)) return;

    float realX = XRealPosition(static_cast<float>(xNewSize), m_lHScroll, ratio);
    float realY = YRealPosition(static_cast<float>(yNewSize), m_lVScroll, ratio);

    // Met à jour la forme actuelle (étirement géométrique)
    m_tousLesTraces.back().endPoint = wxPoint(static_cast<int>(realX), static_cast<int>(realY));
}

void CRectangleDraw::Dessiner(wxDC* deviceContext, const long& hScroll, const long& vScroll, const float& ratio, const wxColour& rgb, const wxColour& rgbFirst, const wxColour& rgbSecond, const int32_t& style) {
    if (deviceContext == nullptr || m_tousLesTraces.empty()) return;

    for (const auto& shape : m_tousLesTraces) {
        int epaisseur = std::max(1, static_cast<int>(shape.penSize * 2 * ratio));
        wxPenStyle penStylewx = (shape.typeBrush == 2) ? wxPENSTYLE_SHORT_DASH : wxPENSTYLE_SOLID;

        wxPen pen(wxColour(shape.color.Red(), shape.color.Green(), shape.color.Blue(), shape.opacity), epaisseur, penStylewx);
        deviceContext->SetPen(pen);

        if (shape.isFilled && shape.shapeType != SHAPE_LINE) {
            deviceContext->SetBrush(wxBrush(wxColour(shape.color.Red(), shape.color.Green(), shape.color.Blue(), shape.opacity)));
        }
        else {
            deviceContext->SetBrush(*wxTRANSPARENT_BRUSH);
        }

        int x1 = static_cast<int>(XDrawingPosition(static_cast<float>(shape.startPoint.x), hScroll, ratio));
        int y1 = static_cast<int>(YDrawingPosition(static_cast<float>(shape.startPoint.y), vScroll, ratio));
        int x2 = static_cast<int>(XDrawingPosition(static_cast<float>(shape.endPoint.x), hScroll, ratio));
        int y2 = static_cast<int>(YDrawingPosition(static_cast<float>(shape.endPoint.y), vScroll, ratio));

        if (shape.shapeType == SHAPE_RECTANGLE) {
            deviceContext->DrawRectangle(wxRect(wxPoint(x1, y1), wxPoint(x2, y2)));
        }
        else if (shape.shapeType == SHAPE_CIRCLE) {
            double radius = std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
            deviceContext->DrawCircle(wxPoint(x1, y1), static_cast<wxCoord>(radius));
        }
        else if (shape.shapeType == SHAPE_LINE) {
            deviceContext->DrawLine(x1, y1, x2, y2);
        }
    }
    deviceContext->SetBrush(wxNullBrush);
    deviceContext->SetPen(wxNullPen);
}


void CRectangleDraw::DrawShapeOnMat(cv::Mat& matrix, SShapeStyleDraw& shape, const long& hScroll, const long& vScroll, const float& ratio)
{
    int thickness = shape.isFilled ? -1 : shape.penSize * 2 * ratio;
    if (shape.shapeType == SHAPE_LINE) thickness = shape.penSize * 2 * ratio; // Une ligne ne peut pas être "remplie"

    int lineStyle = (shape.typeBrush == 1) ? cv::LINE_8 : cv::LINE_AA;
    if (shape.typeBrush == 2) lineStyle = cv::LINE_8; // Pointillés forcés en standard pour le calcul du saut

    cv::Point p1;
    cv::Point p2;

    p1.x = static_cast<int>(XDrawingPosition(static_cast<float>(shape.startPoint.x), hScroll, ratio));
    p1.y = static_cast<int>(XDrawingPosition(static_cast<float>(shape.startPoint.y), vScroll, ratio));
    
    p2.x = static_cast<int>(YDrawingPosition(static_cast<float>(shape.endPoint.x), hScroll, ratio));
    p2.y = static_cast<int>(YDrawingPosition(static_cast<float>(shape.endPoint.y), vScroll, ratio));


    if (shape.shapeType == SHAPE_RECTANGLE) {
        cv::rectangle(matrix, p1, p2, GetColorWithTransparancy(shape.color, shape.opacity), thickness, lineStyle);
    }
    else if (shape.shapeType == SHAPE_CIRCLE) {
        double radius = cv::norm(p1 - p2);
        cv::circle(matrix, p1, static_cast<int>(radius), GetColorWithTransparancy(shape.color, shape.opacity), thickness, lineStyle);
    }
    else if (shape.shapeType == SHAPE_LINE) {
        if (shape.typeBrush == 2) {
            // Rendu basique d'une ligne pointillée en OpenCV
            cv::LineIterator it(matrix, p1, p2, 8);
            for (int i = 0; i < it.count; i++, ++it) {
                if (i % 10 < 5) { // Dessine 5 pixels, saute 5 pixels
                    cv::Scalar color = GetColorWithTransparancy(shape.color, shape.opacity);
                    matrix.at<cv::Vec4b>(it.pos()) = cv::Vec4b(color[0], color[1], color[2], color[3]);
                }
            }
        }
        else {
            cv::line(matrix, p1, p2, GetColorWithTransparancy(shape.color, shape.opacity), thickness, lineStyle);
        }
    }
}

void CRectangleDraw::DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio) {
    if (matrix.empty()) return;
    if (matrix.channels() == 3) cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);


    try {

        for (auto& shape : m_tousLesTraces)
        {
            if (shape.opacity >= 255) {
                DrawShapeOnMat(matrix, shape, hScroll, vScroll, ratio);
            }
            else if (shape.opacity > 0) {
                cv::Mat overlay = matrix.clone();
                DrawShapeOnMat(overlay, shape, hScroll, vScroll, ratio);
                double alpha = shape.opacity / 255.0;
                cv::addWeighted(overlay, alpha, matrix, 1.0 - alpha, 0, matrix);
            }
        }
    }
    catch (cv::Exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }


}
