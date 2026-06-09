#pragma once
#include "ThreadPool.h"
#include <MainWindow.h>
#include <ThreadLoadingBitmap.h>
namespace Regards::Viewer
{


    class CThumbnailProcess
    {
    public:
        CThumbnailProcess(CMainWindow* parent, int maxConcurrent = 0);
        ~CThumbnailProcess();
            
        void ProcessThumbnail(wxString filename, int type, long longWindow, int& nbProcess);


    private:
        static void LoadPicture(void* param);
        CMainWindow* parent = nullptr;
        int                           m_maxConcurrent;
    };

}
