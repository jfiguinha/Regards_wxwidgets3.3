#include <header.h>
#include "TextFilter.h"
#include "TextEffectParameter.h"
#include "TextDraw.h"
#include <LibResource.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include <Metadata.h>
#include <treetypeid.h>
#include <opencv2/imgproc.hpp>
#include <effect_id.h>
#include <FiltreEffet.h>
#include <wx/fontenum.h>
using namespace Regards::Filter;

CTextFilter::CTextFilter() {

    lblFontFace = "Effect.Police";
    lblFontSize = "Effect.Size";
    lblColor = "Effect.Color";
    lblBold = "Effect.Bold";
    lblItalic = "Effect.Italic";
    lblOpacity = "Effect.Transparence";


    // <-- AJOUT : Récupération automatique des polices du système
    m_systemFonts = wxFontEnumerator::GetFacenames();
    m_systemFonts.Sort(); // Optionnel : Tri par ordre alphabétique (A-Z)
}

int CTextFilter::GetTypeFilter() { return IDM_TEXTFILTER; } // Utilisez un ID unique de votre effect_id.h
int CTextFilter::GetNameFilter() { return IDM_TEXTFILTER; }
wxString CTextFilter::GetFilterLabel() { return "Texte"; }

CEffectParameter* CTextFilter::GetEffectPointer() { return new CTextFilterParameter(); }
CDraw* CTextFilter::GetDrawingPt() { return new Regards::FiltreEffet::CTextDraw(); }

void CTextFilter::AddMetadataElement(std::vector<CMetadata>& element, wxString value, int key) {
    CMetadata data;
    data.value = value;
    data.depth = key;
    element.push_back(data);
}

void CTextFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface) {
    this->source = source;
    this->filename = filename;
    auto* param = static_cast<CTextFilterParameter*>(effectParameter);


    // 2. MODIFICATION : Remplissage de la ComboBox avec les polices réelles du système
    std::vector<CMetadata> fontOptions;
    for (size_t i = 0; i < m_systemFonts.GetCount(); ++i) {
        // On associe le nom de la police à son index dans le tableau m_systemFonts
        AddMetadataElement(fontOptions, m_systemFonts[i], static_cast<int>(i));
    }

    // Si m_systemFonts est vide (sécurité), on met une police par défaut
    if (fontOptions.empty()) {
        AddMetadataElement(fontOptions, "Arial", 0);
    }

    filtreInterface->AddTreeInfos(lblFontFace, new CTreeElementValueInt(param->fontIndex), &fontOptions, 3, TYPE_COMBOBOX);

    // 3. Taille de la police (Curseur ou liste de 1 à 100)
    std::vector<int> elementSize;
    for (int i = 5; i <= 150; i += 5) elementSize.push_back(i);
    filtreInterface->AddTreeInfos(lblFontSize, new CTreeElementValueInt(param->fontSize), &elementSize);

    // 4. Choix de la couleur
    filtreInterface->AddTreeInfos(lblColor, new CTreeElementValueColor(param->ConvertScalarToWxColour()), &elementSize, TYPE_COLOR, TYPE_COLOR);


    filtreInterface->AddTreeInfos(lblBold, new CTreeElementValueInt(param->isBold),
        &param->isBold, 2, 2);


    filtreInterface->AddTreeInfos(lblItalic, new CTreeElementValueInt(param->isItalic),
        &param->isItalic, 2, 2);
    // 6. Niveau de Transparence (0 à 255)
    std::vector<int> elementOpacity;
    for (int i = 0; i <= 255; i++) elementOpacity.push_back(i);
    filtreInterface->AddTreeInfos(lblOpacity, new CTreeElementValueInt(param->opacity), &elementOpacity);
}

void CTextFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key) {
    auto* param = static_cast<CTextFilterParameter*>(effectParameter);

    if (key == lblFontFace && valueData->GetType() == TYPE_ELEMENT_INT) {
        param->fontIndex = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
    }
    else if (key == lblFontSize && valueData->GetType() == TYPE_ELEMENT_INT) {
        param->fontSize = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
    }
    else if (key == lblColor && valueData->GetType() == 4) {
        wxColour c = static_cast<CTreeElementValueColor*>(valueData)->GetValue();
        param->color = cv::Scalar(c.Blue(), c.Green(), c.Red(), param->opacity);
    }
    else if (key == lblBold && valueData->GetType() == TYPE_ELEMENT_BOOL) {
        param->isBold = static_cast<CTreeElementValueBool*>(valueData)->GetValue();
    }
    else if (key == lblItalic && valueData->GetType() == TYPE_ELEMENT_BOOL) {
        param->isItalic = static_cast<CTreeElementValueBool*>(valueData)->GetValue();
    }
    else if (key == lblOpacity && valueData->GetType() == TYPE_ELEMENT_INT) {
        param->opacity = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
    }
}

void CTextFilter::AppliquerRenduTexte(cv::Mat& matrix, CTextFilterParameter* param) {
    for (const auto& t : param->listTexts) {
        if (t.text.IsEmpty()) continue;

        cv::Mat overlay = matrix.clone();
        int fontFace = t.fontIndex;
        if (t.isItalic) fontFace |= cv::FONT_ITALIC;

        double fontScale = t.fontSize * 0.05; // Conversion brute OpenCV
        int epaisseur = t.isBold ? 3 : 1;

        cv::putText(overlay, t.text.ToStdString(), cv::Point(t.position.x, t.position.y), fontFace, fontScale, t.color, epaisseur, cv::LINE_AA);

        if (t.opacity >= 255) {
            overlay.copyTo(matrix);
        }
        else if (t.opacity > 0) {
            double alpha = t.opacity / 255.0;
            cv::addWeighted(overlay, alpha, matrix, 1.0 - alpha, 0, matrix);
        }
    }
}

void CTextFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview) {
    auto* param = static_cast<CTextFilterParameter*>(effectParameter);
    if (param->apply) {
        CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
        cv::Mat picture = filtreEffet->GetBitmap(true);
        imageLoad->SetPicture(picture);
        imageLoad->RotateExif(orientation);

        AppliquerRenduTexte(imageLoad->GetMatImage(), param);
        filtreEffet->SetBitmap(imageLoad);
    }
}

CImageLoadingFormat* CTextFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer) {
    auto* param = static_cast<CTextFilterParameter*>(effectParameter);
    CImageLoadingFormat* imageLoad = nullptr;

    if (!source.empty()) {
        imageLoad = new CImageLoadingFormat();
        imageLoad->SetPicture(source);
        imageLoad->RotateExif(orientation);

        AppliquerRenduTexte(imageLoad->GetMatImage(), param);
        param->apply = true;
    }
    return imageLoad;
}

void CTextFilter::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin) {
    if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;

    int hpos = bitmapViewer->GetHPos();
    int vpos = bitmapViewer->GetVPos();
    float ratio = bitmapViewer->GetRatio();

    m_cDessin->DessinerSurMat(matrix, hpos, vpos, ratio);

    auto* textDraw = static_cast<Regards::FiltreEffet::CTextDraw*>(m_cDessin);
    auto* param = static_cast<CTextFilterParameter*>(bitmapViewer->GetEffectPointer());

    if (param != nullptr && textDraw != nullptr) {
        param->listTexts.clear();
        const auto& textesEcran = textDraw->GetTousLesTextes();

        for (const auto& te : textesEcran) {
            STextTrace tOpenCV;
            tOpenCV.position = te.point;
            tOpenCV.text = te.text;
            tOpenCV.fontSize = te.fontSize;
            tOpenCV.fontIndex = te.fontIndex;
            tOpenCV.isBold = te.isBold;
            tOpenCV.isItalic = te.isItalic;
            tOpenCV.opacity = te.opacity;
            tOpenCV.color = cv::Scalar(te.color.Blue(), te.color.Green(), te.color.Red(), te.opacity);

            param->listTexts.push_back(tOpenCV);
        }
    }
}


void CTextFilter::Drawing(wxMemoryDC* dc, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
    if (!m_cDessin || !dc || !bitmapViewer) return;

    auto param = static_cast<CTextFilterParameter*>(bitmapViewer->GetEffectPointer());
    auto textDraw = static_cast<Regards::FiltreEffet::CTextDraw*>(m_cDessin);

    if (param && textDraw) {
        textDraw->SetCurrentTextParams(param->fontSize, param->fontName, param->fontIndex, param->isBold, param->isItalic,
            param->opacity, textDraw->WithOpacity(param->color));
        wxColour color(0, 0, 0);
        m_cDessin->Dessiner(dc, bitmapViewer->GetHPos(), bitmapViewer->GetVPos(), bitmapViewer->GetRatio(), color, color, color, 1);

        param->listTexts.clear();
        for (const auto& t : textDraw->GetTousLesTextes()) {
            STextTrace textOpenCV;
            textOpenCV.position = t.point;
            textOpenCV.text = t.text;
            textOpenCV.fontSize = t.fontSize;
            textOpenCV.fontIndex = t.fontIndex;
            textOpenCV.isBold = t.isBold;
            textOpenCV.isItalic = t.isItalic;
            textOpenCV.opacity = t.opacity;
            textOpenCV.color = cv::Scalar(t.color.Blue(), t.color.Green(), t.color.Red(), t.opacity);

            param->listTexts.push_back(textOpenCV);
        }
    }
}


