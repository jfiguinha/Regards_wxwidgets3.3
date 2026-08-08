#include "header.h"
#include "MetadataExiv2.h"
#ifdef __NEW_EXIV2__
#include "PictureMetadataExiv_new.h"
#else
#include "PictureMetadataExiv.h"
#endif
#include <libPicture.h>
#include <MediaInfo.h>
#include <picture_id.h>

using namespace Regards::Picture;
using namespace Regards::exiv2;


CMetadataExiv2::CMetadataExiv2(const wxString& filename)
{
	metaExiv = nullptr;
	CLibPicture libPicture;
	this->filename = filename;
	int type = libPicture.TestImageFormat(filename);
	metaExiv = new CPictureMetadataExiv(filename);
}


wxString CMetadataExiv2::GetCreationDate()
{
	if (metaExiv != nullptr)
		return metaExiv->GetCreationDate();
	return "";
}

int CMetadataExiv2::GetOrientation()
{
	if (metaExiv != nullptr)
		return metaExiv->GetOrientation();
	return 0;
}

CMetadataExiv2::~CMetadataExiv2()
{

	if (metaExiv != nullptr)
		delete metaExiv;
}

bool CMetadataExiv2::HasExif()
{
	if (metaExiv != nullptr)
		return metaExiv->HasExif();
	return false;
}

std::vector<uint8_t> CMetadataExiv2::GetMetadataBuffer()
{
	CLibPicture libPicture;
	int type = libPicture.TestImageFormat(filename);
	std::vector<uint8_t> buffer;
	if (metaExiv != nullptr)
		return metaExiv->GetMetadataBuffer();
	return buffer;
}

bool CMetadataExiv2::CopyMetadata(const wxString& output)
{
	if (metaExiv != nullptr)
		return metaExiv->CopyMetadata(output);
	return false;
}

bool CMetadataExiv2::HasThumbnail()
{
	if (metaExiv != nullptr)
		return metaExiv->HasThumbnail();
	return false;
}

void CMetadataExiv2::SetDateTime(const wxString& dateTime)
{
	if (metaExiv != nullptr)
		metaExiv->SetDateTime(dateTime);
}

void CMetadataExiv2::SetOrientation(const int& orientation)
{
	if (metaExiv != nullptr)
		metaExiv->SetOrientation(orientation);
}

void CMetadataExiv2::SetGpsInfos(const wxString& latitudeRef, const wxString& longitudeRef, const wxString& latitude,
                                 const wxString& longitude)
{
	if (metaExiv != nullptr)
		metaExiv->SetGpsInfos(latitudeRef, longitudeRef, latitude, longitude);
}

void CMetadataExiv2::ReadPicture(bool& hasGps, bool& hasDataTime, wxString& dateTimeInfos, wxString& latitude,
                                 wxString& longitude)
{
	if (metaExiv != nullptr)
		metaExiv->ReadPicture(hasGps, hasDataTime, dateTimeInfos, latitude, longitude);
}

std::vector<CMetadata> CMetadataExiv2::GetMetadata()
{
	CLibPicture libPicture;
	if (libPicture.TestIsVideo(filename))
	{
		return CMediaInfo::ReadMetadata(filename);
	}

	if (metaExiv != nullptr)
		return metaExiv->GetMetadata();

	std::vector<CMetadata> meta;
	return meta;
}


wxImage CMetadataExiv2::DecodeThumbnail(wxString& extension, int& orientation)
{
	wxImage image;
	if (metaExiv != nullptr)
		image = metaExiv->DecodeThumbnail(extension, orientation);
	return image;
}