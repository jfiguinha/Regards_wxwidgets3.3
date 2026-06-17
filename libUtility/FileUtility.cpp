#include "header.h"
#include "FileUtility.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/dir.h>
#include "ConvertUtility.h"


CFileUtility::CFileUtility(void)
{
}


CFileUtility::~CFileUtility(void)
{
}

std::string CFileUtility::GetFullpathModel(const wxString& filename)
{
	wxFileName esrgan_param = wxFileName(CFileUtility::GetDocumentFolderPath());
	esrgan_param.AppendDir("model");
	esrgan_param.SetFullName(filename);
	return esrgan_param.GetFullPath().ToStdString();
}

wxString CFileUtility::GetOrCreateSubFolder(const wxString& folderName)
{
	wxFileName path(GetDocumentFolderPath(), "");

	path.AppendDir(folderName);

	if (!wxDir::Exists(path.GetFullPath()))
	{
		wxDir::Make(path.GetFullPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}

	return path.GetFullPath();
}

wxString CFileUtility::GetResourcesFolderFontPathWithExt(const wxString& szFile)
{
	wxString path = GetResourcesFolderPathWithExt("font");
	path += wxFILE_SEP_PATH;
    path.append(szFile);
    return path;
}

wxString CFileUtility::GetResourcesFolderPathWithExt(const wxString & folder)
{
	wxFileName file(GetResourcesFolderPath(),
		folder);
    return file.GetFullPath();
}

wxString CFileUtility::GetTempFile(wxString filename, wxString folder, const bool& removeFile)
{
	wxFileName file(GetOrCreateSubFolder(folder),
		filename);
	if (removeFile && wxFileExists(file.GetFullPath()))
	{
		if (!wxRemoveFile(file.GetFullPath()))
		{
			wxLogWarning("Impossible de supprimer le fichier : %s", file.GetFullPath());
		}
	}
	return file.GetFullPath();
}

wxString CFileUtility::GetTempFile(wxString filename, const bool& removeFile)
{
	return GetTempFile(filename, "temp", removeFile);
}


wxString CFileUtility::GetFileExtension(const wxString& szFilePath)
{
	auto filename = wxFileName(szFilePath);
	return filename.GetExt();
}

wxString CFileUtility::GetFileName(const wxString& szFilePath)
{
	auto filename = wxFileName(szFilePath);
	return filename.GetFullName();
}

wxString CFileUtility::GetFolder(const wxString& szFilePath)
{
	auto filename = wxFileName(szFilePath);
	return filename.GetPath();
}

#if defined(__APPLE__)
wxString CFileUtility::GetAppleExecFolderPath(const wxString& folder)
{
	wxString exeFolderMacOs = ".app/Contents/";
	wxString path = wxStandardPaths::Get().GetExecutablePath();
	int index = path.Find(exeFolderMacOs);
	if (index == wxNOT_FOUND)
		return GetFolder(path);

	wxString realPath;
	realPath.append(path.begin(), path.begin() + index + exeFolderMacOs.size());
	realPath.append(folder);

	return realPath;
}
#endif

wxString CFileUtility::GetResourcesFolderPath()
{
#if defined(__APPLE__)
    return GetAppleExecFolderPath("Resources");

#else

	wxFileName path(GetProgramFolderPath(), "");
	path.AppendDir("Resources");
	return path.GetFullPath();

#endif
}

wxString CFileUtility::GetFaceZScorePath(const int& numFace)
{
	wxFileName file(GetOrCreateSubFolder("Face"),
		wxString::Format("%d.bin", numFace));

	return file.GetFullPath();
}

wxString CFileUtility::GetFaceThumbnailPath(const int& numFace)
{
	wxFileName file(GetOrCreateSubFolder("Face"),
		wxString::Format("%d.jpg", numFace));

	return file.GetFullPath();
}

wxString CFileUtility::GetVideoThumbnailPath(const wxString& videoPath, const int& numFrame)
{
	wxFileName sourceFile(videoPath);

	wxString fileName = wxString::Format(
		"%s_%d.jpg",
		sourceFile.GetName(),
		numFrame
	);

	wxFileName thumbnailFile(
		GetOrCreateSubFolder("ThumbnailVideo"),
		fileName
	);

	return thumbnailFile.GetFullPath();
}

wxString CFileUtility::GetThumbnailPath(const wxString& path)
{
	wxFileName sourceFile(path);

	wxString fileName = wxString::Format(
		"%s.jpg",
		sourceFile.GetName());

	wxFileName thumbnailFile(
		GetOrCreateSubFolder("Thumbnail"),
		fileName
	);

	return thumbnailFile.GetFullPath();
}

wxString CFileUtility::GetDocumentFolderPath()
{
	wxString documentPath = wxStandardPaths::Get().GetDocumentsDir();
	documentPath += wxFILE_SEP_PATH;
	documentPath.append("Regards");
	if (!wxDir::Exists(documentPath))
	{
		wxDir::Make(documentPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}
	return documentPath;
}

wxString CFileUtility::GetDocumentFolderPathWithFilename(const wxString & filename)
{
	wxFileName path(GetDocumentFolderPath(), filename);
	return path.GetFullPath();
}


wxString CFileUtility::GetProgramFolderPath()
{
#if defined(__APPLE__)
	return GetAppleExecFolderPath("MacOS");
#else
    wxString path = wxStandardPaths::Get().GetExecutablePath();
    return GetFolder(path);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

wxString CFileUtility::GetFileTime(const wxString& szFileName)
{
	wxStructStat strucStat;
	if (wxStat(szFileName, &strucStat) != 0)
		return "";
	//wxFileOffset filelen = strucStat.st_size;
	wxDateTime last_modified_time(strucStat.st_mtime);
	return last_modified_time.Format(wxT("%Y.%m.%d"));
}
