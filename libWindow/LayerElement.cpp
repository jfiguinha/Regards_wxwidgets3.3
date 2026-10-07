#include <header.h>
#include "LayerElement.h"
#include <ImageLoadingFormat.h>

CImageLoadingFormat* CLayerList::GetPictureToShow()
{
	if (m_layers.size() > 0)
		return m_layers[0]->GetPicture();

	return nullptr;
}


int CLayerList::GetWidth()
{
	if (m_layers.size() > 0)
		return m_layers[0]->GetPicture()->GetWidth();

	return 0;
}

int CLayerList::GetHeight()
{
	if (m_layers.size() > 0)
		return m_layers[0]->GetPicture()->GetHeight();

	return 0;
}