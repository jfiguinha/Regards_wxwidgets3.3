#include <header.h>
#include "VideoConverterFrame.h"
#include <CompressionAudioVideoOption.h>
#include <VideoCompressOption.h>
#include <wx/filename.h>
#include <MediaInfo.h>
#include <MediaExtractor.h>
#include <ConvertUtility.h>
#include <FileUtility.h>
#include <LibResource.h>
#include <libPicture.h>
#include <ConfigRegards.h>
#include <RegardsConfigParam.h>
#include <ParamInit.h>
#if defined(__WXMSW__)
#include "../include/window_id.h"
#else
#include <window_id.h>
#endif
#include "FFmpegTranscoding.h"
#include <SliderVideoSelection.h>
#include <AudioEncoder.h>
#include <wx/progdlg.h>
#include <wx/evtloop.h>
using namespace Regards::Picture;
// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {
    void RemoveIfExists(const wxString& path) {
        if (!path.empty() && wxFileExists(path)) wxRemoveFile(path);
    }

    wxString FormatFFmpegError(int errnum) {
        char message[AV_ERROR_MAX_STRING_SIZE];

        av_make_error_string(message, AV_ERROR_MAX_STRING_SIZE, errnum);

        return wxString::FromUTF8(message);
    }
}  // namespace

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

CVideoConverterFrame::CVideoConverterFrame(
    IVideoConverterInterface* videoInterface) {
    this->videoInterface = videoInterface;

    CRegardsConfigParam* regardsParam = CParamInit::getInstance();

    if (regardsParam != nullptr) regardsParam->SetInterpolationType(1);
}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------

CVideoConverterFrame::~CVideoConverterFrame() {
    /*
     * Le thread d'encodage doit impérativement être terminé avant
     * de détruire l'objet CVideoConverterFrame.
     */
    if (m_encodeThread.joinable()) {
        m_encodeThread.join();
    }

    RemoveIfExists(fileOut);
    RemoveIfExists(fileOutAudio);
    RemoveIfExists(fileOutVideo);

    RemoveIfExists(fileOutAudio_encode);
    RemoveIfExists(fileOutVideo_encode);

    if (!fileOut_cut.empty() && fileOut_cut != filename) {
        RemoveIfExists(fileOut_cut);
    }
}

// ----------------------------------------------------------------------------
// Select input file
// ----------------------------------------------------------------------------

wxString CVideoConverterFrame::SelectFile()
{
    wxFileDialog openFileDialog(nullptr, _("Open video file"), "", "", "mp4 files (*.mp4)|*.mp4",  wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    const wxString documentPath = CFileUtility::GetDocumentFolderPath();

    openFileDialog.SetDirectory(documentPath);

    if (openFileDialog.ShowModal() == wxID_CANCEL) return wxEmptyString;

    return openFileDialog.GetPath();
}

// ----------------------------------------------------------------------------
// Select output file
// ----------------------------------------------------------------------------

wxString CVideoConverterFrame::SelectOutputFile(wxString& filename) {
    wxFileName videoFilename(filename);

    const wxString savevideofile =
        CLibResource::LoadStringFromResource(L"LBLSAVEVIDEOFILE", 1);

    const wxString filename_label =
        CLibResource::LoadStringFromResource(L"LBLFILESNAME", 1);

    const wxString filenameToSave = videoFilename.GetName();

    wxFileDialog saveFileDialog(nullptr, savevideofile, "", filenameToSave,
        "mp4 " + filename_label +
        " (*.mp4)|*.mp4|"
        "webm " +
        filename_label +
        " (*.webm)|*.webm|"
        "mov " +
        filename_label +
        " (*.mov)|*.mov|"
        "mkv " +
        filename_label + " (*.mkv)|*.mkv",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    const wxString documentPath = CFileUtility::GetDocumentFolderPath();

    saveFileDialog.SetDirectory(documentPath);

    if (saveFileDialog.ShowModal() == wxID_CANCEL) return wxEmptyString;

    wxString filepath = saveFileDialog.GetPath();

    const int index = saveFileDialog.GetFilterIndex();

    const wxString ext = wxFileName(filepath).GetExt().Lower();

    if (ext != "mp4" && ext != "webm" && ext != "mov" && ext != "mkv") {
        static const wxString extensions[] = { "mp4", "webm", "mov", "mkv" };

        const int safeIndex = std::clamp(index, 0, 3);

        filepath += "." + extensions[safeIndex];
    }

    return filepath;
}

// ----------------------------------------------------------------------------
// Exit application
// ----------------------------------------------------------------------------

void CVideoConverterFrame::ExitApplication() {
    /*
     * Cette fonction est appelée uniquement depuis le thread UI.
     */

    const wxString filesToClean[] = { fileOutVideo, fileOutAudio,
                                     fileOutAudio_encode, fileOutVideo_encode };

    for (const auto& filepath : filesToClean) {
        RemoveIfExists(filepath);
    }

    if (!fileOut_cut.empty() && fileOut_cut != filename) {
        RemoveIfExists(fileOut_cut);
    }

    if (videoInterface != nullptr) {
        videoInterface->Close();
    }

    /*
     * On conserve le comportement original de l'application.
     *
     * Attention :
     * exit(0) termine immédiatement le processus.
     */
    exit(0);
}

// ----------------------------------------------------------------------------
// Encode audio
// ----------------------------------------------------------------------------

int CVideoConverterFrame::EncodeAudioSample(
    CVideoOptionCompress* videoCompressOption, const wxString& input,
    const wxString& output) {
    if (videoCompressOption == nullptr) return AVERROR(EINVAL);

    AudioEncoder encoder;
    AudioEncoderOptions options;

    // ------------------------------------------------------------------------
    // Codec
    // ------------------------------------------------------------------------

    if (videoCompressOption->audioCodec == "AAC") {
        options.codec = AudioCodec::AAC;
    }
    else if (videoCompressOption->audioCodec == "MP3") {
        options.codec = AudioCodec::MP3;
    }
    else {
        options.codec = AudioCodec::VORBIS;
    }

    // ------------------------------------------------------------------------
    // Quality / bitrate
    // ------------------------------------------------------------------------

    if (videoCompressOption->audioBitRate > 0) {
        options.mode = EncodingMode::Bitrate;
        options.bitrateKbps = videoCompressOption->audioBitRate;
    }
    else {
        options.mode = EncodingMode::Quality;
        options.quality = videoCompressOption->audioQuality;
    }

    const std::string strInput = CConvertUtility::ConvertToStdString(input);

    const std::string strOutput = CConvertUtility::ConvertToStdString(output);

    // ------------------------------------------------------------------------
    // State
    // ------------------------------------------------------------------------

    std::atomic<bool> isFinished(false);
    std::atomic<bool> cancelRequested(false);

    std::atomic<int> progressPercent(0);
    std::atomic<int> currentSeconds(0);
    std::atomic<int> totalSeconds(0);

    std::atomic<int> encodeResult(AVERROR(EAGAIN));

    // ------------------------------------------------------------------------
    // Progress dialog
    // ------------------------------------------------------------------------

    auto* audioProgressDlg = new wxProgressDialog(
        "Encoding Audio", "Starting encoding...", 100, nullptr,
        wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_ELAPSED_TIME |
        wxPD_REMAINING_TIME);

    // ------------------------------------------------------------------------
    // Worker
    // ------------------------------------------------------------------------

    std::thread workerThread([&]() {
        const int result = encoder.EncodeAudioOnly(
            strInput, strOutput, options,
            [&](double curSec, double totSec) -> bool {
                if (cancelRequested.load()) return false;

                currentSeconds.store(static_cast<int>(curSec));

                totalSeconds.store(static_cast<int>(totSec));

                if (totSec > 0.0) {
                    int percent = static_cast<int>((curSec / totSec) * 100.0);

                    percent = std::clamp(percent, 0, 100);

                    progressPercent.store(percent);
                }

                return true;
            });

        encodeResult.store(result);
        isFinished.store(true);
        });

    // ------------------------------------------------------------------------
    // UI loop
    // ------------------------------------------------------------------------

    while (!isFinished.load()) {
        if (wxEventLoopBase::GetActive()) {
            wxEventLoopBase::GetActive()->DispatchTimeout(30);

            if (wxTheApp != nullptr) {
                wxTheApp->ProcessPendingEvents();
            }
        }
        else {
            wxMilliSleep(30);
        }

        if (audioProgressDlg != nullptr) {
            const wxString message =
                wxString::Format("Processing: %d / %d seconds", currentSeconds.load(),
                    totalSeconds.load());

            if (!audioProgressDlg->Update(progressPercent.load(), message)) {
                cancelRequested.store(true);
            }
        }
    }

    // ------------------------------------------------------------------------
    // Wait worker
    // ------------------------------------------------------------------------

    if (workerThread.joinable()) {
        workerThread.join();
    }

    // ------------------------------------------------------------------------
    // Close dialog
    // ------------------------------------------------------------------------

    if (audioProgressDlg != nullptr) {
        if (encodeResult.load() >= 0 && !cancelRequested.load()) {
            audioProgressDlg->Update(100, "Encoding completed!");
        }

        audioProgressDlg->Hide();
        audioProgressDlg->Destroy();

        audioProgressDlg = nullptr;
    }

    if (wxTheApp != nullptr) {
        wxTheApp->ProcessPendingEvents();
    }

    return encodeResult.load();
}

// ----------------------------------------------------------------------------
// Encode video asynchronously
// ----------------------------------------------------------------------------

void CVideoConverterFrame::EncodeFile(CVideoOptionCompress* videoCompressOption,
    const wxString& input,
    const wxString& output, int rotation,
    std::function<void(int)> onComplete) {
    if (videoCompressOption == nullptr) {
        if (onComplete) onComplete(AVERROR(EINVAL));

        return;
    }

    // ------------------------------------------------------------------------
    // Wait previous worker
    // ------------------------------------------------------------------------

    if (m_encodeThread.joinable()) {
        m_encodeThread.join();
    }

    // ------------------------------------------------------------------------
    // Create progress dialog on UI thread
    // ------------------------------------------------------------------------

    m_dlgProgress = std::make_unique<CompressVideo>(nullptr, rotation);

    m_dlgProgress->SetFocus();
    m_dlgProgress->Raise();
    m_dlgProgress->Show();

    CompressVideo* progressDlg = m_dlgProgress.get();

    // ------------------------------------------------------------------------
    // Start worker
    // ------------------------------------------------------------------------

    m_encodeThread = std::thread([this, videoCompressOption, input, output,
        progressDlg, onComplete]() {
            int ret = AVERROR(EAGAIN);

            try {
                // --------------------------------------------------------
                // OpenCL context belongs exclusively to worker
                // --------------------------------------------------------

                auto openCLContext = std::make_unique<COpenCLContext>();

                if (!openCLContext->CreateDefaultOpenCLContext()) {
                    ret = AVERROR(EIO);
                }
                else {
                    CFFmpegTranscoding ffmpegtranscoding(openCLContext.get());

                    ret = ffmpegtranscoding.EncodeFile(input, output, progressDlg,
                        videoCompressOption);
                }
            }
            catch (...) {
                ret = AVERROR(EFAULT);
            }

            // ------------------------------------------------------------
            // Return to UI thread
            // ------------------------------------------------------------

            if (wxTheApp == nullptr) return;

            wxTheApp->CallAfter([this, ret, onComplete]() {
                // ----------------------------------------------------
                // Close progress dialog
                // ----------------------------------------------------

                bool wasProgressOk = false;

                if (m_dlgProgress) {
                    wasProgressOk = m_dlgProgress->IsOk();

                    m_dlgProgress->Close();

                    m_dlgProgress.reset();
                }

                // ----------------------------------------------------
                // Encoding failed
                // ----------------------------------------------------

                if (ret < 0) {
                    const wxString errorTitle =
                        CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

                    wxMessageBox(FormatFFmpegError(ret), errorTitle, wxICON_ERROR);

                    if (onComplete) onComplete(ret);

                    return;
                }

                // ----------------------------------------------------
                // Verify video output
                // ----------------------------------------------------

                if (fileOutVideo.empty() || !wxFileExists(fileOutVideo)) {
                    const wxString errorTitle =
                        CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

                    wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);

                    if (onComplete) onComplete(AVERROR(EIO));

                    return;
                }

                // ----------------------------------------------------
                // Mux audio + video
                // ----------------------------------------------------

                bool muxResult = true;

                if (needToRemux) {
                    if (fileOutAudio.empty() || !wxFileExists(fileOutAudio)) {
                        muxResult = false;
                    }
                    else {
                        muxResult = Regards::Media::ExecuteFFmpegMuxVideoAudio(
                            fileOutVideo.utf8_string(), fileOutAudio.utf8_string(),
                            fileOutputPath.utf8_string());
                    }
                }
                else {
                    /*
                     * Pas de muxage nécessaire.
                     * Le fichier vidéo encodé devient
                     * directement le résultat final.
                     */
                    RemoveIfExists(fileOutputPath);

                    muxResult = wxCopyFile(fileOutVideo, fileOutputPath);
                }

                // ----------------------------------------------------
                // Mux failed
                // ----------------------------------------------------

                if (!muxResult || !wxFileExists(fileOutputPath)) {
                    const wxString errorTitle =
                        CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

                    wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);

                    if (onComplete) onComplete(AVERROR(EIO));

                    return;
                }

                // ----------------------------------------------------
                // Success
                // ----------------------------------------------------

                const wxString infos =
                    CLibResource::LoadStringFromResource("LBLINFORMATIONS", 1);

                const wxString completed =
                    CLibResource::LoadStringFromResource("LBLFILEENCODINGCOMPLETED", 1);

                if (wasProgressOk) {
                    wxMessageBox(completed, infos);
                }

                // ----------------------------------------------------
                // Callback
                // ----------------------------------------------------

                if (onComplete) {
                    onComplete(0);
                }
                });
        });
}

// ----------------------------------------------------------------------------
// Export video
// ----------------------------------------------------------------------------

void CVideoConverterFrame::ExportVideo(const wxString& fileIn) {
    CMediaInfo metadata;
    CLibPicture libPicture;

    fileOut.clear();
    filename = fileIn;

    // ------------------------------------------------------------------------
    // Input file
    // ------------------------------------------------------------------------

    if (!wxFileExists(filename)) {
        filename = SelectFile();
    }

    if (filename.empty() || !libPicture.TestIsVideo(filename)) {
        ExitApplication();
        return;
    }

    // ------------------------------------------------------------------------
    // Rotation
    // ------------------------------------------------------------------------

    const int rotation = metadata.GetVideoRotation(filename);

    // ------------------------------------------------------------------------
    // Output file
    // ------------------------------------------------------------------------

    fileOutputPath = SelectOutputFile(filename);

    if (fileOutputPath.empty()) {
        ExitApplication();
        return;
    }

    // ------------------------------------------------------------------------
    // Compression options
    // ------------------------------------------------------------------------

    m_compressAudioVideoOption = std::make_unique<CompressionAudioVideoOption>();

    m_compressAudioVideoOption->SetFile(filename, fileOutputPath);

    m_compressAudioVideoOption->ShowModal();

    if (!m_compressAudioVideoOption->IsOk()) {
        ExitApplication();
        return;
    }

    CVideoOptionCompress* videoCompressOption =
        m_compressAudioVideoOption->GetVideoCompressionPt();

    if (videoCompressOption == nullptr) {
        ExitApplication();
        return;
    }

    // ------------------------------------------------------------------------
    // Temporary files
    // ------------------------------------------------------------------------

    const wxFileName file_temp(fileOutputPath);

    fileOut = CFileUtility::GetTempFile("temp." + file_temp.GetExt(), true);

    fileOut_cut =
        CFileUtility::GetTempFile("temp_cut." + file_temp.GetExt(), true);

    fileOutAudio.clear();
    fileOutVideo.clear();
    fileOutAudio_encode.clear();
    fileOutVideo_encode.clear();

    // ------------------------------------------------------------------------
    // Cut video
    // ------------------------------------------------------------------------

    wxString timeInput = "00:00:00";
    wxString timeOutput = "00:00:00";

    if (videoCompressOption->startTime != 0 ||
        videoCompressOption->endTime != 0) {
        timeInput = CConvertUtility::GetTimeLibelle(videoCompressOption->startTime);

        timeOutput = CConvertUtility::GetTimeLibelle(videoCompressOption->endTime);
    }

    bool result = true;

    if (timeInput == "00:00:00" && timeOutput == "00:00:00") {
        fileOut_cut = filename;
    }
    else {
        result = Regards::Media::ExecuteFFmpegCutVideo(
            filename.utf8_string(), timeInput.utf8_string(),
            timeOutput.utf8_string(), fileOut_cut.utf8_string());
    }

    if (!result) {
        if (!m_compressAudioVideoOption->IsCancel()) {
            const wxString errorTitle =
                CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

            wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);
        }

        ExitApplication();
        return;
    }

    // ------------------------------------------------------------------------
    // CASE 1
    // Audio + Video direct copy
    // ------------------------------------------------------------------------

    if (videoCompressOption->audioDirectCopy &&
        videoCompressOption->videoDirectCopy) {
        RemoveIfExists(fileOutputPath);

        result = wxCopyFile(fileOut_cut, fileOutputPath);

        if (!result) {
            const wxString errorTitle =
                CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

            wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);

            ExitApplication();
            return;
        }

        const wxString completed =
            CLibResource::LoadStringFromResource("LBLFILEENCODINGCOMPLETED", 1);

        const wxString infos =
            CLibResource::LoadStringFromResource("LBLINFORMATIONS", 1);

        wxMessageBox(completed, infos);

        ExitApplication();
        return;
    }

    // ------------------------------------------------------------------------
    // Temporary audio/video output
    // ------------------------------------------------------------------------

    fileOutVideo =
        CFileUtility::GetTempFile("temp_video." + file_temp.GetExt(), true);

    fileOutAudio =
        CFileUtility::GetTempFile("temp_audio." + file_temp.GetExt(), true);

    RemoveIfExists(fileOutVideo);
    RemoveIfExists(fileOutAudio);

    // ------------------------------------------------------------------------
    // CASE 2
    // Copy audio / encode video
    // ------------------------------------------------------------------------

    if (videoCompressOption->audioDirectCopy) {
        result = Regards::Media::ExecuteFFmpegExtractAudio(
            fileOut_cut.utf8_string(), fileOutAudio.utf8_string());

        if (!result || !wxFileExists(fileOutAudio)) {
            if (!m_compressAudioVideoOption->IsCancel()) {
                const wxString errorTitle =
                    CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

                wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);
            }

            ExitApplication();
            return;
        }

        isAudio = true;
        needToRemux = true;

        EncodeFile(videoCompressOption, fileOut_cut, fileOutVideo, rotation,
            [this](int ret) {
                if (ret != 0) {
                    ExitApplication();
                    return;
                }

                ExitApplication();
            });

        return;
    }

    // ------------------------------------------------------------------------
    // CASE 3
    // Copy video / encode audio
    // ------------------------------------------------------------------------

    if (videoCompressOption->videoDirectCopy) {
        result = Regards::Media::ExecuteFFmpegExtractVideo(
            fileOut_cut.utf8_string(), fileOutVideo.utf8_string());

        if (!result || !wxFileExists(fileOutVideo)) {
            if (!m_compressAudioVideoOption->IsCancel()) {
                const wxString errorTitle =
                    CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

                wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);
            }

            ExitApplication();
            return;
        }

        // ------------------------------------------------------------
        // Encode audio
        // ------------------------------------------------------------

        const int audioResult =
            EncodeAudioSample(videoCompressOption, fileOut_cut, fileOutAudio);

        if (audioResult < 0 || !wxFileExists(fileOutAudio)) {
            if (!m_compressAudioVideoOption->IsCancel()) {
                const wxString errorTitle =
                    CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

                wxMessageBox(FormatFFmpegError(audioResult), errorTitle, wxICON_ERROR);
            }

            ExitApplication();
            return;
        }

        // ------------------------------------------------------------
        // Mux
        // ------------------------------------------------------------

        result = Regards::Media::ExecuteFFmpegMuxVideoAudio(
            fileOutVideo.utf8_string(), fileOutAudio.utf8_string(),
            fileOutputPath.utf8_string());

        if (!result || !wxFileExists(fileOutputPath)) {
            const wxString errorTitle =
                CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

            wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);

            ExitApplication();
            return;
        }

        const wxString completed =
            CLibResource::LoadStringFromResource("LBLFILEENCODINGCOMPLETED", 1);

        const wxString infos =
            CLibResource::LoadStringFromResource("LBLINFORMATIONS", 1);

        wxMessageBox(completed, infos);

        ExitApplication();
        return;
    }

    // ------------------------------------------------------------------------
    // CASE 4
    // Encode audio + encode video
    // ------------------------------------------------------------------------

    /*
     * IMPORTANT :
     *
     * Ici on utilise les MEMBRES de la classe.
     *
     * L'ancienne version faisait :
     *
     * wxString fileOutAudio_encode = ...
     * wxString fileOutVideo_encode = ...
     *
     * ce qui masquait les membres portant probablement
     * les mêmes noms.
     *
     * Cela empêchait notamment ExitApplication() de connaître
     * ces fichiers temporaires.
     */

    fileOutAudio_encode =
        CFileUtility::GetTempFile("temp_audio_enc." + file_temp.GetExt(), true);

    fileOutVideo_encode =
        CFileUtility::GetTempFile("temp_video_enc." + file_temp.GetExt(), true);

    RemoveIfExists(fileOutAudio_encode);
    RemoveIfExists(fileOutVideo_encode);

    // ------------------------------------------------------------------------
    // Extract audio
    // ------------------------------------------------------------------------

    result = Regards::Media::ExecuteFFmpegExtractAudio(
        fileOut_cut.utf8_string(), fileOutAudio_encode.utf8_string());

    if (!result || !wxFileExists(fileOutAudio_encode)) {
        if (!m_compressAudioVideoOption->IsCancel()) {
            const wxString errorTitle =
                CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

            wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);
        }

        ExitApplication();
        return;
    }

    // ------------------------------------------------------------------------
    // Extract video
    // ------------------------------------------------------------------------

    result = Regards::Media::ExecuteFFmpegExtractVideo(
        fileOut_cut.utf8_string(), fileOutVideo_encode.utf8_string());

    if (!result || !wxFileExists(fileOutVideo_encode)) {
        if (!m_compressAudioVideoOption->IsCancel()) {
            const wxString errorTitle =
                CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

            wxMessageBox(errorTitle, errorTitle, wxICON_ERROR);
        }

        ExitApplication();
        return;
    }

    // ------------------------------------------------------------------------
    // Encode audio
    // ------------------------------------------------------------------------

    const int audioResult =
        EncodeAudioSample(videoCompressOption, fileOutAudio_encode, fileOutAudio);

    if (audioResult < 0 || !wxFileExists(fileOutAudio)) {
        if (!m_compressAudioVideoOption->IsCancel()) {
            const wxString errorTitle =
                CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

            wxMessageBox(FormatFFmpegError(audioResult), errorTitle, wxICON_ERROR);
        }

        ExitApplication();
        return;
    }

    isAudio = true;
    needToRemux = true;

    // ------------------------------------------------------------------------
    // Encode video asynchronously
    // ------------------------------------------------------------------------

    EncodeFile(videoCompressOption, fileOutVideo_encode, fileOutVideo, rotation,
        [this](int ret) {
            /*
             * Le muxage est effectué dans EncodeFile,
             * sur le thread UI, après la fin de l'encodage vidéo.
             */

            if (ret != 0) {
                ExitApplication();
                return;
            }

            ExitApplication();
        });
}