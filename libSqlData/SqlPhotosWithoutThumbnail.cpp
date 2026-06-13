#include <header.h>
#include "SqlPhotosWithoutThumbnail.h"
#include "SqlResult.h"
#include <SqlParameter.h>
using namespace Regards::Sqlite;


CSqlPhotosWithoutThumbnail::CSqlPhotosWithoutThumbnail()
	: CSqlExecuteRequest(L"RegardsDB")
{
	typeResult = 0;
	priority = 0;
	photoList = nullptr;
}

CSqlPhotosWithoutThumbnail::~CSqlPhotosWithoutThumbnail()
{
}

int CSqlPhotosWithoutThumbnail::GetPhotoElement()
{
	nbElement = 0;
	typeResult = 1;
	ExecuteRequest("SELECT count(*) as nbElement from PHOTOSWIHOUTTHUMBNAIL_VIEW");
	return nbElement;
}

void CSqlPhotosWithoutThumbnail::GetPhotoList(std::deque<wxString> * photoList, int nbElement)
{
	this->nbElement = nbElement;
	this->photoList = photoList;
	typeResult = 0;
	if (nbElement > 0)
	{
		std::vector<std::unique_ptr<CSqlParameter>> parameter;
		parameter.push_back(std::make_unique<CSqlInt>(nbElement));
		ExecuteSqlWithStatementBool("SELECT DISTINCT FullPath from PHOTOSWIHOUTTHUMBNAIL_VIEW LIMIT ?", parameter);
	}
	else
		ExecuteRequest("SELECT DISTINCT FullPath from PHOTOSWIHOUTTHUMBNAIL_VIEW");
}

int CSqlPhotosWithoutThumbnail::TraitementResult(CSqlResult* sqlResult)
{
	int nbResult = 0;
	nbElement = 0;
	while (sqlResult->Next())
	{
		switch (typeResult)
		{
		case 0:
			photoList->push_back(sqlResult->GetText("FullPath"));
			nbResult++;
			break;
		case 1:
			nbElement = priority = sqlResult->GetInt("nbElement");
			nbResult++;
			break;
		}

		if (typeResult == 1 && nbResult > 0)
			break;
	}
	return nbResult;
}
