#pragma once
#include "MainWindow.h"
#include "ThumbnailProcess.h"
#include "InfosSeparationBarExplorer.h"
#include "TreatmentData.h"
#include <thread>
#include <atomic>
#include <memory>

using namespace Regards::Window;

namespace Regards::Viewer
{
    class CMainWindow;

    // -----------------------------------------------------------------------
    // Thread context for the "check existing files" background scan.
    // Previously embedded sleep + per-file events; now uses CUIEventBatcher.
    // -----------------------------------------------------------------------
    class CThreadCheckFile
    {
    public:
        CThreadCheckFile() : mainWindow(nullptr) {}
        ~CThreadCheckFile() = default;

        static void CheckFile(void* param);

        std::unique_ptr<std::thread> checkFile;
        CMainWindow* mainWindow = nullptr;
    };

    // -----------------------------------------------------------------------
    // Helpers kept for compatibility with other translation units.
    // -----------------------------------------------------------------------
    class CFolderFiles
    {
    public:
        std::vector<wxString> pictureFiles;
        wxString              folderName;
    };

    class CThreadVideoData
    {
    public:
        CMainWindow* mainWindow = nullptr;
        wxString     video;
    };

    class CThreadPhotoLoading
    {
    public:
        CThreadPhotoLoading()
            : _pictures(new PhotosVector())
            , _listSeparator(new InfosSeparationBarVector())
        {}
        ~CThreadPhotoLoading() = default;

        Regards::Viewer::CMainWindow* mainWindow       = nullptr;
        CIconeList*                   iconeListLocal    = nullptr;
        InfosSeparationBarVector*     _listSeparator;
        CIconeList*                   iconeListThumbnail = nullptr;
        int                           typeAffichage      = 0;
        PhotosVector*                 _pictures;
    };

    // -----------------------------------------------------------------------
    // Orchestrates folder refresh: delegates SQL to CSqlBatchOps,
    // filesystem checks to CFileSystemValidator, UI events to CUIEventBatcher.
    // -----------------------------------------------------------------------
    class CFolderProcess
    {
    public:
        explicit CFolderProcess(CMainWindow* mainWindow);
        ~CFolderProcess() = default;

        void UpdateCriteria(bool criteriaSendMessage);

        // Scans watched folders, removes stale records, imports new files.
        // Sets folderChange=true if any record was added or removed.
        void RefreshFolder(bool& folderChange, int& nbFile);

    private:
        CMainWindow* mainWindow;
        wxString     m_oldRequest;
    };
}
