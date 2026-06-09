#pragma once
#include "ThreadPool.h"
#include <MainWindow.h>
#include <ThreadLoadingBitmap.h>
namespace Regards::Viewer
{

    // Manages asynchronous thumbnail loading through a bounded ThreadPool.
    // No raw thread creation; no per-task sleep.
    class CThumbnailProcess
    {
    public:
        explicit CThumbnailProcess(CMainWindow* parent,
                                   int maxConcurrent = 0); // 0 = read from config
        ~CThumbnailProcess() = default;

        // Enqueue a thumbnail load. Returns false if the pool is at capacity.
        bool EnqueueThumbnail(const wxString& filename, int type, long longWindow);

        // Cancel all pending work (e.g. on window close).
        void CancelAll();

        // Number of tasks currently in flight.
        size_t ActiveCount() const;

    private:
        static void LoadPicture(CThreadLoadingBitmap * task);

        CMainWindow* m_parent      = nullptr;
        int                           m_maxConcurrent;
        std::unique_ptr<CThreadPool>  m_pool;
        std::atomic<int>              m_inFlight{0};
    };
}
