#pragma once
#include <memory>
#include <ffmpeg_transcoding.h>

namespace Regards::Viewer
{
    class CExportDiaporama
    {
    public:
        explicit CExportDiaporama() = default;
        ~CExportDiaporama() = default;

        static void OnExportDiaporama(wxWindow* parent);

    };
}