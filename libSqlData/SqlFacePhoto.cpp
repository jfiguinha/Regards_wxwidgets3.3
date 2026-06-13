#include <header.h>
#include "SqlFacePhoto.h"
#include "SqlFaceLabel.h"
#include "SqlFaceRecognition.h"
#include "SqlFindFacePhoto.h"
#include "SqlResult.h"
#include "SqlPhotos.h"
#include <FileUtility.h>
#include <ImageLoadingFormat.h>
#include <libPicture.h>
#include "ThumbnailBuffer.h"
#include <wx/file.h>
#include <wx/dir.h>
#include <SqlParameter.h>
using namespace Regards::Sqlite;
using namespace Regards::Picture;

#define VIDEO_POSITION 1
#define NUM_FACE 2
#define LISTE_NUM_FACE 3
#define FULLPATH_FACE 4
#define LISTE_NUM_FACE_COMPATIBLE 5
#define LISTE_FACE_COMPATIBLE 6
#define LISTE_FULLPATH_FACE 7

CSqlFacePhoto::CSqlFacePhoto()
	: CSqlExecuteRequest(L"RegardsDB"), numFace(0), type(0)
{
}


CSqlFacePhoto::~CSqlFacePhoto()
{
}

int CSqlFacePhoto::UpdateVideoFace(const int& numFace, const int& videoPosition)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numFace));
	parameter.push_back(std::make_unique<CSqlInt>(videoPosition));
	return ExecuteSqlWithStatementBool("INSERT INTO FACEVIDEO (NumFace, videoPosition) VALUES (? , ?)", parameter);
}

int CSqlFacePhoto::GetVideoFacePosition(const int& numFaceid)
{
	videoPosition = 0;
	type = VIDEO_POSITION;
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numFace));
	ExecuteSqlWithStatement("SELECT videoPosition FROM FACEVIDEO WHERE NumFace = ?", parameter);
	return videoPosition;
}

bool CSqlFacePhoto::DeleteNumFaceMaster(const int& numFace)
{
	listFace.clear();
	type = LISTE_NUM_FACE;
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numFace));
	ExecuteSqlWithStatement("Select distinct NumFace FROM FACE_RECOGNITION WHERE NumFaceCompatible = ?", parameter);

	if (listFace.size() > 0)
	{
		for (int faceId : listFace)
		{
			wxString thumbnail = CFileUtility::GetFaceThumbnailPath(faceId);
			if (wxFileExists(thumbnail))
				wxRemoveFile(thumbnail);

			thumbnail = CFileUtility::GetFaceZScorePath(faceId);
			if (wxFileExists(thumbnail))
				wxRemoveFile(thumbnail);

			std::vector<std::unique_ptr<CSqlParameter>> parameter;
			parameter.push_back(std::make_unique<CSqlInt>(faceId));
			ExecuteSqlWithStatement("DELETE FROM FACEPHOTO WHERE NumFace = ?", parameter);
			ExecuteSqlWithStatement("DELETE FROM FACEVIDEO WHERE NumFace = ?", parameter);
		}
	}

	{
		std::vector<std::unique_ptr<CSqlParameter>> parameter;
		parameter.push_back(std::make_unique<CSqlInt>(numFace));
		ExecuteSqlWithStatement("DELETE FROM FACE_RECOGNITION WHERE NumFaceCompatible = ?", parameter);
		ExecuteSqlWithStatement("DELETE FROM FACE_NAME WHERE NumFace = ?", parameter);
		DeleteFaceNameAlone();
	}


	return true;
}

void CSqlFacePhoto::EraseFace(const int& numFace)
{
	type = FULLPATH_FACE;
	filename = "";
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numFace));
	ExecuteSqlWithStatement("Select FullPath FROM FACEPHOTO WHERE NumFace = ?", parameter);
	if (filename != "")
	{
		DeletePhotoFaceDatabase(filename);
		InsertFaceTreatment(filename);
		RebuildLink();
	}
}

void CSqlFacePhoto::DeleteNumFace(const int& numFace)
{
	wxString thumbnail = CFileUtility::GetFaceThumbnailPath(numFace);
	if (wxFileExists(thumbnail))
		wxRemoveFile(thumbnail);

	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numFace));
	ExecuteSqlWithStatement("DELETE FROM FACEPHOTO WHERE NumFace = ?", parameter);
	ExecuteSqlWithStatement("DELETE FROM FACEVIDEO WHERE NumFace = ?", parameter);
	ExecuteSqlWithStatement("DELETE FROM FACE_RECOGNITION WHERE NumFace = ?", parameter);
	DeleteFaceNameAlone();
}

int CSqlFacePhoto::GetFaceCompatibleRecognition(const int& numFace)
{
	listFace.clear();
	type = LISTE_NUM_FACE_COMPATIBLE;
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numFace));
	ExecuteSqlWithStatement("Select distinct NumFaceCompatible FROM FACE_RECOGNITION WHERE NumFace = ? ", parameter);
	if (listFace.size() > 0)
		return listFace[0];
	return -1;
}

vector<CFaceRecognitionData> CSqlFacePhoto::GetAllNumFaceRecognition()
{
	listFaceRecognition.clear();
	type = LISTE_FACE_COMPATIBLE;
	ExecuteRequest("Select NumFace, NumFaceCompatible FROM FACE_RECOGNITION ORDER BY NumFaceCompatible");
	return listFaceRecognition;
}

vector<int> CSqlFacePhoto::GetAllThumbnailFace()
{
	listFace.clear();
	type = LISTE_NUM_FACE;
	ExecuteRequest("Select NumFace FROM FACEPHOTO");
	return listFace;
}

vector<int> CSqlFacePhoto::GetAllNumFace()
{
	listFace.clear();
	type = LISTE_NUM_FACE;
	ExecuteRequest("SELECT (Select NumFace FROM FACE_RECOGNITION WHERE FACE_RECOGNITION.NumFaceCompatible = FACEPHOTO.NumFace) as NumFaceCompatible FROM FACEPHOTO");
	return listFace;
}

vector<int> CSqlFacePhoto::GetAllNumFace(const int& numFace)
{
	listFace.clear();
	type = LISTE_NUM_FACE_COMPATIBLE;
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numFace));
	ExecuteSqlWithStatement("SELECT (Select NumFace FROM FACE_RECOGNITION WHERE FACE_RECOGNITION.NumFaceCompatible = FACEPHOTO.NumFace) as NumFaceCompatible FROM FACEPHOTO where NumFace != ?", parameter);
	return listFace;
}



CImageLoadingFormat* CSqlFacePhoto::GetFacePicture(const int& numFace)
{
	CImageLoadingFormat* picture = nullptr;
	wxString thumbnail = CFileUtility::GetFaceThumbnailPath(numFace);
	if (wxFileExists(thumbnail))
	{
		CLibPicture libPicture;
		picture = libPicture.LoadPicture(thumbnail);
		//picture->Flip();
	}
	return picture;

}

bool CSqlFacePhoto::DeleteListOfPhoto(const vector<int>& listNumPhoto)
{
	for(auto i = 0;i < listNumPhoto.size();i++)
	{
		int numPhoto = listNumPhoto[i];
		CSqlPhotos sqlPhoto;
		wxString path = sqlPhoto.GetPhotoPath(numPhoto);

		CSqlFindFacePhoto findFacePhoto;
		std::vector<CFaceName> listFace = findFacePhoto.GetListFaceNum(path);

		for (int i1 = 0; i1 < listFace.size(); ++i1)
		{
			CFaceName facename = listFace[i1];
			DeleteNumFace(facename.numFace);
		}

		std::vector<std::unique_ptr<CSqlParameter>> parameter;
		parameter.push_back(std::make_unique<CSqlInt>(numFace));
		ExecuteSqlWithStatement("DELETE FROM FACE_PROCESSING WHERE fullpath in (select fullpath from Photos where NumPhoto = ?", parameter);
	}
	RebuildLink();
	return false;
}

void CSqlFacePhoto::RebuildLink()
{
	//Recomposition des liens entre photos
	CSqlFaceLabel faceLabel;
	vector<int> listFace = faceLabel.GetFaceLabelAlone();

	for (auto i = 0;i < listFace.size(); i++)
	{
		int oldNumFace = listFace[i];
		numFace = -1;
		type = NUM_FACE;
		std::vector<std::unique_ptr<CSqlParameter>> parameter;
		parameter.push_back(std::make_unique<CSqlInt>(oldNumFace));
		ExecuteSqlWithStatement("SELECT numFace FROM FACE_RECOGNITION WHERE NumFaceCompatible = ? ORDER BY numFace ASC LIMIT 1", parameter);
		if (numFace != -1)
		{
			CSqlFaceRecognition faceRecognition;
			CSqlFaceLabel faceLabel;
			faceLabel.UpdateNumFaceLabel(oldNumFace, numFace);
			faceRecognition.UpdateFaceRecognition(oldNumFace, numFace);
		}
	}   

	DeleteFaceNameAlone();
}

bool CSqlFacePhoto::DeleteListOfPhoto(const vector<wxString>& listPhoto)
{
	for (auto i = 0; i < listPhoto.size(); i++)
	{
		wxString fullpath = listPhoto[i];
		fullpath.Replace("'", "''");
		//ExecuteRequest("SELECT NumFace FROM FACEPHOTO WHERE fullpath = '" + fullpath + "'");

		CSqlFindFacePhoto findFacePhoto;
		std::vector<CFaceName> listFace = findFacePhoto.GetListFaceNum(listPhoto[i]);

		for (auto k = 0; i < listFace.size(); k++)
		{
			//changed line
			CFaceName facename = listFace[k];
			DeleteNumFace(facename.numFace);
		}

		std::vector<std::unique_ptr<CSqlParameter>> parameter;
		parameter.push_back(std::make_unique<CSqlString>(fullpath));
		ExecuteSqlWithStatement("DELETE FROM FACE_PROCESSING WHERE fullpath = ?", parameter);
	}
	RebuildLink();
	return false;
}

void CSqlFacePhoto::DeleteFaceNameAlone()
{
	ExecuteRequestWithNoResult("DELETE FROM FACE_NAME WHERE NumFace not in (select NumFace from FACEPHOTO)");
}

bool CSqlFacePhoto::DeleteFaceTreatmentDatabase()
{
	ExecuteRequestWithNoResult("DELETE FROM FACE_PROCESSING");
	return false;
}

vector<wxString> CSqlFacePhoto::GetPhotoList()
{
	listPhoto.clear();
	type = LISTE_FULLPATH_FACE;
	ExecuteRequest("SELECT FullPath FROM PHOTOS WHERE FullPath not in (select distinct FullPath FROM FACEPHOTO)");
	return listPhoto;
}

vector<wxString> CSqlFacePhoto::GetPhotoListTreatment()
{
	listPhoto.clear();
	type = LISTE_FULLPATH_FACE;
	ExecuteRequest("SELECT FullPath FROM PHOTOS WHERE FullPath not in (select FullPath FROM FACE_PROCESSING) ");
	return listPhoto;
}

int CSqlFacePhoto::InsertFaceTreatment(const wxString& path)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(path));
	return ExecuteSqlWithStatement("INSERT INTO FACE_PROCESSING (FullPath) VALUES(?)", parameter);
}

//--------------------------------------------------------
//Chargement de toutes les données d'un album
//--------------------------------------------------------
int CSqlFacePhoto::InsertFace(const wxString& path, const wxString& gender, const wxString& age, const int& numberface, const int& width, const int& height,
                              const double& pertinence, const uint8_t* zBlob, const int& nBlob)
{
	wxString value = wxString::Format(wxT("%f"), pertinence);

	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(path));
	parameter.push_back(std::make_unique<CSqlInt>(numberface));
	parameter.push_back(std::make_unique<CSqlInt>(width));
	parameter.push_back(std::make_unique<CSqlInt>(height));
	parameter.push_back(std::make_unique<CSqlString>(value));
	parameter.push_back(std::make_unique<CSqlString>(gender));
	parameter.push_back(std::make_unique<CSqlString>(age));

	ExecuteSqlWithStatement("INSERT INTO FACEPHOTO (FullPath, Numberface, width, height, Pertinence, gender, age) VALUES(?,?,?,?,?,?,?)", parameter);

	int numFaceId = GetNumFace(path, numberface);

	wxString thumbnail = CFileUtility::GetFaceThumbnailPath(numFaceId);
	wxFile fileOut;
	fileOut.Create(thumbnail, true);
	fileOut.Write(zBlob, nBlob);
	fileOut.Close();

	return numFaceId;
}

int CSqlFacePhoto::GetNumFace(const wxString& path, const int& numberface)
{
	numFace = 0;
	type = NUM_FACE;
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(path));
	parameter.push_back(std::make_unique<CSqlInt>(numberface));
	ExecuteSqlWithStatement("SELECT NumFace FROM FACEPHOTO WHERE FullPath = ? and Numberface = ?", parameter);
	return numFace;
}

cv::Mat CSqlFacePhoto::GetFace(const int& numFace, bool& isDefault)
{
	wxLogNull logNo;
	wxString thumbnail = CFileUtility::GetFaceThumbnailPath(numFace);
	cv::Mat image;
	if (wxFileExists(thumbnail))
	{
		//image = cv::Mat();
        cv::flip(CThumbnailBuffer::GetPicture(thumbnail),image,-1);
	}

	if (image.empty())
	{
		DeleteNumFace(numFace);
		isDefault = true;
	}
	else
		isDefault = false;

   // cv::flip(image,image,-1);
	return image;//image.Mirror(false);
}


bool CSqlFacePhoto::DeletePhotoFaceDatabase(const wxString& path)
{
	type = LISTE_NUM_FACE;
	listFace.clear();
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(path));
	ExecuteSqlWithStatement("SELECT NumFace FROM FACEPHOTO WHERE FullPath = ?", parameter);
	for (int i : listFace)
	{
		DeleteNumFace(i);
	}

	return true;
}

bool CSqlFacePhoto::DeleteFaceDatabase()
{
	wxString documentPath = CFileUtility::GetDocumentFolderPath();
	documentPath += wxFILE_SEP_PATH;
	documentPath.append("Face");

	wxArrayString files;
	wxDir::GetAllFiles(documentPath, &files, wxEmptyString, wxDIR_FILES);

	tbb::parallel_for(0, static_cast<int>(listFace.size()), 1, [=](int i)
	{
		wxString filename = files[i];
		if (wxFileExists(filename))
			wxRemoveFile(filename);
	});


	return (ExecuteRequestWithNoResult("DELETE FROM FACEPHOTO") != -1) ? true : false;
}

int CSqlFacePhoto::TraitementResult(CSqlResult* sqlResult)
{
	videoPosition = -1;
	listFace.clear();
	numFace = -1;
	filename = "";
	listFaceRecognition.clear();
	listPhoto.clear();

	while (sqlResult->Next())
	{
		switch (type)
		{
		case VIDEO_POSITION:
			videoPosition = sqlResult->GetInt("videoPosition");
			return 1;
		case LISTE_NUM_FACE:
			listFace.push_back(sqlResult->GetInt("NumFace"));
			break;
		case NUM_FACE:
			numFace = sqlResult->GetInt("NumFace");
			return 1;
		case FULLPATH_FACE:
			filename = sqlResult->GetText("FullPath");
			return 1;
		case LISTE_NUM_FACE_COMPATIBLE:
			listFace.push_back(sqlResult->GetInt("NumFaceCompatible"));
			break;
		case LISTE_FACE_COMPATIBLE:
			CFaceRecognitionData data;
			data.numFace = sqlResult->GetInt("NumFace");
			data.numFaceCompatible = sqlResult->GetInt("NumFaceCompatible");
			listFaceRecognition.push_back(data);
			break;
		case LISTE_FULLPATH_FACE:
			listPhoto.push_back(sqlResult->GetText("FullPath"));
			break;
		}
	}

	if (LISTE_FACE_COMPATIBLE)
		return listFaceRecognition.size();

	return listFace.size();
}
