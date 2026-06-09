#include <header.h>
#include "ThumbnailProcess.h"
#include <SqlThumbnail.h>
#include <libPicture.h>
#include <ImageLoadingFormat.h>
#include <ConvertUtility.h>
#include <ParamInit.h>
#include <RegardsConfigParam.h>

#include <window_id.h>
#include <opencv2/imgcodecs.hpp>
#include <wx/filename.h>
using namespace Regards::Sqlite;
using namespace Regards::Picture;
using namespace Regards::Viewer;

// ---------------------------------------------------------------------------
CThumbnailProcess::CThumbnailProcess(CMainWindow* parent,
                                     int maxConcurrent)
    : m_parent(parent)
{
    if (maxConcurrent <= 0)
    {
        int cfg = 1;
        if (CRegardsConfigParam* config = CParamInit::getInstance(); config != nullptr)
            cfg = config->GetThumbnailProcess() + 1;
        m_maxConcurrent = cfg;
    }
    else
    {
        m_maxConcurrent = maxConcurrent;
    }

    // Pool sized to the allowed concurrency so the OS scheduler does the work.
    m_pool = std::make_unique<CThreadPool>(static_cast<size_t>(m_maxConcurrent));
}

// ---------------------------------------------------------------------------
bool CThumbnailProcess::EnqueueThumbnail(const wxString& filename, int type,
                                          long longWindow)
{
    if (filename.empty())
        return false;

    if (m_inFlight >= m_maxConcurrent)
        return false;

    ++m_inFlight;

    auto task      = new CThreadLoadingBitmap();
    task->filename  = filename;
    task->window    = m_parent;
    task->longWindow = longWindow;
    task->type      = type;

    m_pool->Enqueue([this, task]() mutable
    {
        LoadPicture(task);
        --m_inFlight;
    });

    return true;
}

// ---------------------------------------------------------------------------
void CThumbnailProcess::CancelAll()
{
    m_pool->RequestStop();
}

// ---------------------------------------------------------------------------
size_t CThumbnailProcess::ActiveCount() const
{
    return static_cast<size_t>(m_inFlight.load());
}

// ---------------------------------------------------------------------------
/*static*/
void CThumbnailProcess::LoadPicture(CThreadLoadingBitmap * task)
{
    CLibPicture libPicture;

    CImageLoadingFormat* imageLoad = libPicture.LoadThumbnail(task->filename);
    if (imageLoad != nullptr)
    {
        task->bitmapIcone = imageLoad->GetMatrix().getMat();
        delete imageLoad;
    }

    if (!task->bitmapIcone.empty())
    {
        CSqlThumbnail sqlThumbnail;
        wxFileName    file(task->filename);
        wxULongLong   sizeFile = file.GetSize();
        wxString      hash     = sizeFile.ToString();

        wxString localName = sqlThumbnail.InsertThumbnail(
            task->filename,
            task->bitmapIcone.size().width,
            task->bitmapIcone.size().height,
            hash);

        if (!localName.empty())
            cv::imwrite(CConvertUtility::ConvertToStdString(localName), task->bitmapIcone);
    }

    // Notify UI — ownership of CThumbnailTask transferred to the event.
    auto event = new wxCommandEvent(wxEVENT_ICONEUPDATE);
    event->SetClientData(task); // caller must keep task alive; use shared_ptr refcount
    wxQueueEvent(task->window, event);
}
