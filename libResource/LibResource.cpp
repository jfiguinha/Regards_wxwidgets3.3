// DllResource.cpp : définit les fonctions exportées pour l'application DLL.
//
#include "header.h"
#include "LibResource.h"
#include <SqlResource.h>
#include <SqlLibResource.h>
#include <SqlEngine.h>
#include <ConvertUtility.h>
#include <FileUtility.h>
#include <libPicture.h>
#include <wx/sstream.h>
#include <RegardsConfigParam.h>
#include <ParamInit.h>
#include <wx/filename.h>
using namespace Regards::Sqlite;

wxString CLibResource::GetPhotoCancel()
{
	wxFileName file(CFileUtility::GetResourcesFolderPath(), "photo_cancel.png");
	return file.GetFullPath();
}

bool CLibResource::InitializeSQLServerDatabase(const wxString& folder)
{
	auto libResource = new CSqlLibResource(true, true);
	wxFileName file(folder, "resource.db");
	printf("ResourceDB %s \n", CConvertUtility::ConvertToStdString(file.GetFullPath()).c_str());
	return CSqlEngine::Initialize(file.GetFullPath(), L"ResourceDB", libResource);
}

void CLibResource::KillSqlEngine()
{
	CSqlEngine::kill(L"ResourceDB");
}



vector<wxString> CLibResource::GetSavePictureFormat()
{
	CSqlResource sqlResource;
	return sqlResource.GetSavePictureFormat();
}

vector<wxString> CLibResource::GetSavePictureExtension()
{
	CSqlResource sqlResource;
	return sqlResource.GetSavePictureExtension();
}

wxImage CLibResource::CreatePictureFromSVG(const wxString& idName, const int& buttonWidth, const int& buttonHeight)
{
	CSqlResource sqlResource;
	return Regards::Picture::CLibPicture::CreatePictureFromSVGFilename(sqlResource.GetFilepath(idName), buttonWidth, buttonHeight);
}

wxString CLibResource::LoadExifNameFromResource(const wxString& id)
{
	CSqlResource sqlResource;
	return sqlResource.GetExifLibelle(id);
}

wxString CLibResource::LoadBitmapFromResource(const wxString& idName)
{
	CSqlResource sqlResource;
	return sqlResource.GetBitmapResourcePath(idName);
}

wxString CLibResource::LoadStringFromResource(const wxString& idName, const int& idLang)
{
	CSqlResource sqlResource;
	int numLanguage = idLang;

	CRegardsConfigParam* config = CParamInit::getInstance();
	if (config != nullptr)
		numLanguage = config->GetNumLanguage();

	return sqlResource.GetLibelle(idName, numLanguage);
}

wxString CLibResource::GetVector(const wxString& idName)
{
	CSqlResource sqlResource;
	return sqlResource.GetVectorFromFile(idName);
}

int CLibResource::GetExtensionId(const wxString& extension)
{
	CSqlResource sqlResource;
	int id = sqlResource.GetExtensionId(extension);
	return id;
}

wxString CLibResource::GetOpenGLShaderFromDB(const wxString& idName)
{
	CSqlResource sqlResource;
	return  sqlResource.GetText(idName);
}

wxString CLibResource::GetOpenGLShaderProgram(const wxString& idName)
{
	CSqlResource sqlResource;
	return sqlResource.GetOpenGLFromFile(idName);
}

wxString CLibResource::GetOpenCLFloatProgram(const wxString& idName)
{
	CSqlResource sqlResource;
	return sqlResource.GetOpenCLFloatFromFile(idName);
}

wxString CLibResource::GetOpenCLUcharProgram(const wxString& idName)
{
	CSqlResource sqlResource;
	return sqlResource.GetOpenCLUcharFromFile(idName);
}
