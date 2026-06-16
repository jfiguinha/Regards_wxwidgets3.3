#include <header.h>
#include "SqlPhotoGPS.h"
#include "SqlResult.h"
#include <SqlParameter.h>
using namespace Regards::Sqlite;

CSqlPhotoGPS::CSqlPhotoGPS(CSqlLib* _sqlLibTransaction, const bool& useTransaction)
	: CSqlExecuteRequest(L"RegardsDB"), numPhoto(0), numFolderId(0), nbResult(0)
{
	this->m_transaction = _sqlLibTransaction;
	this->m_useTransaction = useTransaction;
}


CSqlPhotoGPS::~CSqlPhotoGPS()
{
}

bool CSqlPhotoGPS::InsertPhoto(const int& numPhoto, const wxString& filepath, const int& numFolderId)
{
    std::vector<std::unique_ptr<CSqlParameter>> parameter;
    parameter.push_back(std::make_unique<CSqlInt>(numPhoto));
    parameter.push_back(std::make_unique<CSqlString>(filepath));
    parameter.push_back(std::make_unique<CSqlInt>(numFolderId));
    return ExecuteSqlWithStatementNoResult("INSERT INTO PHOTOSGPS (NumPhoto, FullPath, NumFolderId) VALUES (?,?,?)", parameter);
}

bool CSqlPhotoGPS::DeletePhoto(const int64_t& numPhoto)
{
    std::vector<std::unique_ptr<CSqlParameter>> parameter;
    parameter.push_back(std::make_unique<CSqlInt>(numPhoto));
    return ExecuteSqlWithStatementNoResult("DELETE FROM PHOTOSGPS WHERE NumPhoto = ? ", parameter);
}


bool CSqlPhotoGPS::DeleteListOfPhoto(const vector<wxString>& listPhoto)
{

	for (int i = 0; i < listPhoto.size(); i++)
	{
		DeletePhoto(i);
	}

	return false;
}

 int CSqlPhotoGPS::GetListPhoto(GpsPhotosVector * photoGpsVec)
 {
    type = 0;
    m_photoGpsVec = photoGpsVec;
	ExecuteRequest("SELECT NumPhoto, FullPath, NumFolderId FROM PHOTOSGPS");
	return nbResult;   
 }

 int CSqlPhotoGPS::GetNbPhoto()
 {
     type = 1;
     nbResultRequest = 0;
     ExecuteRequest("SELECT count(*) as nb FROM PHOTOSGPS");
     return nbResultRequest;
 }

int CSqlPhotoGPS::GetFirstPhoto(int& numPhoto, wxString& filepath, int& numFolderId)
{
    type = 2;
	ExecuteRequest("SELECT NumPhoto, FullPath, NumFolderId FROM PHOTOSGPS Limit 1");
	numPhoto = this->numPhoto;
	filepath = this->filepath;
	numFolderId = this->numFolderId;
	return nbResult;
}

int CSqlPhotoGPS::TraitementResult(CSqlResult* sqlResult)
{
	nbResult = 0;
	while (sqlResult->Next())
	{
        switch (type)
		{
            case 2:
            {
                numPhoto = sqlResult->ColumnDataInt(0);
                filepath = sqlResult->ColumnDataText(1);
                numFolderId = sqlResult->ColumnDataInt(2);
                break;
            }

            case 0:
            {
                GpsPhoto gpsPhoto;
                gpsPhoto.numPhoto = sqlResult->ColumnDataInt(0);
                gpsPhoto.filepath = sqlResult->ColumnDataText(1);
                gpsPhoto.numFolderId = sqlResult->ColumnDataInt(2);
                m_photoGpsVec->push_back(gpsPhoto);
                break;
            }
           
            case 1:
            {
               nbResultRequest = sqlResult->ColumnDataInt(0);
               break;
            }
        }
        nbResult++;
	}
	return nbResult;
}
