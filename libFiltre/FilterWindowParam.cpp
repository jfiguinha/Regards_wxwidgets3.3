#include <header.h>
//
//  FilterWindowParam.cpp
//  Regards.libViewer
//
//  Created by figuinha jacques on 12/04/2016.
//  Copyright © 2016 figuinha jacques. All rights reserved.
//

#include "FilterWindowParam.h"
#include <FilterData.h>
#include <ImageLoadingFormat.h>
#include <FiltreEffet.h>
#include <Draw.h>
#include <BitmapDisplay.h>


CFilterWindowParam::CFilterWindowParam()
{

}

CImageLoadingFormat* CFilterWindowParam::ApplyMouseMoveEffect(CEffectParameter* effectParameter,
                                                              IBitmapDisplay* bitmapViewer, CDraw* dessing)
{
	return nullptr;
}

void CFilterWindowParam::Drawing(wxMemoryDC* dc, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	m_cDessin->Dessiner(dc, 0, 0, 1, wxColour(30, 30, 30), wxColour(30, 30, 30), wxColour(255, 255, 255), 2);
}

void CFilterWindowParam::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{

	int hpos = bitmapViewer->GetHPos();
	int vpos = bitmapViewer->GetVPos();
	float ratio = bitmapViewer->GetRatio();
	m_cDessin->DessinerSurMat(matrix, hpos, vpos, ratio);
}

void CFilterWindowParam::DrawingToPicture(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer,
	CFiltreEffet* filtreEffet, CDraw* m_cDessin)
{
	if (m_cDessin != nullptr && effectParameter != nullptr && !effectParameter->IsApplyBeforeInterpolation())
	{
		

		if (effectParameter->IfNeedTransparence())
		{
			Regards::Picture::CPictureArray mat = filtreEffet->GetMatrix();
			// /!\ ATTENTION : Si GetBitmap(false) contient déjà les dessins précédents,
			// préférez passer 'true' ou écraser avec la source propre pour éviter la superposition infinie.
			cv::Mat matBase = mat.getMat();

			// Appelle CPenFilter::Drawing qui appelle à son tour m_cDessin->DessinerSurMat
			Drawing(matBase, bitmapViewer, m_cDessin);

			CImageLoadingFormat imageLoad;
			imageLoad.SetPicture(matBase);
			filtreEffet->SetBitmap(&imageLoad);
			// Plus besoin de delete manuel ici pour imageLoad car alloué sur la pile
		}
		else
		{
			wxImage image = filtreEffet->GetwxImage();
			wxBitmap bitmap(image);

			{
				wxMemoryDC dc;
				dc.SelectObject(bitmap);
				Drawing(&dc, bitmapViewer, m_cDessin);
				dc.SelectObject(wxNullBitmap);
			} // Le scope { } garantit que la restriction de la DC est libérée proprement avant la conversion

			// ALLOCATION SUR LA PILE (STACK) : Évite les fuites de mémoire (Memory Leaks)
			wxImage local_image = bitmap.ConvertToImage();

			CImageLoadingFormat imageLoad;
			imageLoad.SetPicture(local_image);
			filtreEffet->SetBitmap(&imageLoad);
		}
	}
}




bool CFilterWindowParam::SupportMouseClick()
{
	return false;
}

void CFilterWindowParam::SetCursor()
{
	wxSetCursor(wxCursor(wxCURSOR_ARROW));
}

bool CFilterWindowParam::SupportMouseSelection()
{
	return false;
}

bool CFilterWindowParam::IsOpenCLCompatible()
{
	return true;
}

void CFilterWindowParam::ApplyPreviewEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer,
                                            CFiltreEffet* filtreEffet, CDraw* dessing, int& widthOutput,
                                            int& heightOutput)
{
	if (CFiltreData::IsOpenCLCompatible(GetNameFilter()) && cv::ocl::haveOpenCL())
		filtreEffet->RenderEffect(GetNameFilter(), effectParameter);
	else
	{
		cv::Mat bitmap = filtreEffet->GetBitmap(false);
		if (!bitmap.empty())
		{
			CImageLoadingFormat image;
			image.SetPicture(bitmap);
			auto filtre = std::make_unique<CFiltreEffet>(bitmapViewer->GetBackColor(), nullptr, &image);
			filtre->RenderEffect(GetNameFilter(), effectParameter);
			auto imageLoad = std::make_unique<CImageLoadingFormat>();
			cv::Mat mat = filtre->GetBitmap(true);
			imageLoad->SetPicture(mat);
			filtreEffet->SetBitmap(imageLoad.get());

		}
	}
}

void CFilterWindowParam::ApplyPreviewEffectSource(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer,
                                                  CFiltreEffet* filtreEffet, CDraw* m_cDessin)
{
	if (effectParameter->IsApplyBeforeInterpolation())
	{
		cv::Mat matBase = filtreEffet->GetBitmap(true);

		// Appelle CPenFilter::Drawing qui appelle à son tour m_cDessin->DessinerSurMat
		//Drawing(matBase, bitmapViewer, m_cDessin);

		int hpos = bitmapViewer->GetHPos();
		int vpos = bitmapViewer->GetVPos();
		float ratio = 1;// bitmapViewer->GetRatio();
		m_cDessin->DessinerSurMat(matBase, hpos, vpos, ratio);

		auto imageLoad = std::make_unique<CImageLoadingFormat>();
		imageLoad->SetPicture(matBase);
		filtreEffet->SetBitmap(imageLoad.get());
	}
}

void CFilterWindowParam::ApplyExifToPoint(wxPoint& pt, int numExif, const int& width, const int& height)
{
	int y = width - pt.y;
	int x = pt.x;


	switch (numExif)
	{
	case 1: // top left side
		break;
	case 2: // top right side
		pt.x = width - pt.x;
	//filtre->FlipHorizontal();
		break;
	case 3: // bottom right side
		pt.x = width - pt.x;
		pt.y = height - pt.y;
	//filtre->FlipHorizontal();
	//filtre->FlipVertical();
		break;
	case 4: // bottom left side
		pt.y = height - pt.y;
	//filtre->FlipVertical();
		break;
	case 5: //left side top
		//filtre->Rotate90();
		pt.y = x;
		pt.x = y;
	//pt.y = height - pt.y;
		pt.x = width - pt.x;
	//filtre->FlipVertical();
		break;
	case 6: // right side top
		//filtre->Rotate90();
		pt.y = x;
		pt.x = y;
		pt.x = width - pt.x;
		pt.y = height - pt.y;
	//filtre->FlipVertical();
	//filtre->FlipHorizontal();
		break;
	case 7: // right side bottom
		//filtre->Rotate90();
		pt.y = x;
		pt.x = y;
	//pt.x = width - pt.x;
		pt.y = height - pt.y;
	//filtre->FlipHorizontal();
		break;
	case 8: // left side bottom
		//filtre->Rotate90();
		pt.y = x;
		pt.x = y;
		break;
	default: ;
	}
}

void CFilterWindowParam::RotateExif(const int& orientation, CFiltreEffet* filtre)
{
	switch (orientation)
	{
	case 1: // top left side
		break;
	case 2: // top right side
		filtre->FlipHorizontal();
		break;
	case 3: // bottom right side
		filtre->FlipHorizontal();
		filtre->FlipVertical();
		break;
	case 4: // bottom left side
		filtre->FlipVertical();
		break;
	case 5: //left side top
		filtre->Rotate90();
		filtre->FlipVertical();
		break;
	case 6: // right side top
		filtre->Rotate90();
		filtre->FlipVertical();
		filtre->FlipHorizontal();
		break;
	case 7: // right side bottom
		filtre->Rotate90();
		filtre->FlipHorizontal();
		break;
	case 8: // left side bottom
		filtre->Rotate90();

		break;
	default: ;
	}
}

CImageLoadingFormat* CFilterWindowParam::RenderEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer,
                                                      int numFiltre)
{
	CImageLoadingFormat* imageLoad = nullptr;

	CFiltreEffet* filtre = bitmapViewer->GetFiltreEffet();
	if (filtre != nullptr)
	{
		filtre->SetPreview(false);
		RotateExif(bitmapViewer->GetOrientation(), filtre);
	}

	if (CFiltreData::IsOpenCLCompatible(numFiltre) && cv::ocl::haveOpenCL())
	{
		if (filtre != nullptr)
		{
			int orientation = bitmapViewer->GetOrientation();
			filtre->RenderEffect(numFiltre, effectParameter);
			imageLoad = new CImageLoadingFormat();
			cv::Mat bitmapOut = filtre->GetBitmap(true);
			if (orientation > 4)
			{
				flip(bitmapOut, bitmapOut, 0);
				flip(bitmapOut, bitmapOut, 1);
			}
			imageLoad->SetPicture(bitmapOut);
			imageLoad->SetOrientation(0);
		}
	}
	else
	{
		CImageLoadingFormat* bitmap = bitmapViewer->GetBitmap(true);
		if (bitmap != nullptr)
		{
			bitmap->RotateExif(bitmapViewer->GetOrientation());
			auto filtre_effet = new CFiltreEffet(bitmapViewer->GetBackColor(), nullptr, bitmap);
			filtre_effet->RenderEffect(numFiltre, effectParameter);
			imageLoad = new CImageLoadingFormat();
			cv::Mat mat = filtre_effet->GetBitmap(true);
			imageLoad->SetPicture(mat);
			imageLoad->SetOrientation(0);

			delete filtre_effet;
		}
	}


	imageLoad->SetFilename(bitmapViewer->GetFilename());
	return imageLoad;
}

CImageLoadingFormat* CFilterWindowParam::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	CImageLoadingFormat* imageLoad = RenderEffect(effectParameter, bitmapViewer, GetNameFilter());
	return imageLoad;
}
