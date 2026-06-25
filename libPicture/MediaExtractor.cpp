#include <header.h>
#include "MediaExtractor.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstring>
#include <format>
#include <ranges>
#include <stdexcept>
#include <vector>

extern "C" {
#include <libavutil/mathematics.h>
}

// ─────────────────────────────────────────────────────────────
//  Helpers statiques
// ─────────────────────────────────────────────────────────────
std::string AvError(int code)
{
    std::array<char, AV_ERROR_MAX_STRING_SIZE> buf{};
    av_strerror(code, buf.data(), buf.size());
    return { buf.data() };
}

void ThrowIfError(int code, std::string_view context)
{
    if (code < 0)
        throw std::runtime_error(std::format("[FFmpeg] {} : {}", context, AvError(code)));
}


namespace Regards::Media {

    using namespace detail;

    // ─────────────────────────────────────────────────────────────
    //  Timecode — implémentation
    // ─────────────────────────────────────────────────────────────

    // Parse un entier depuis [begin, end[. Retourne false en cas d'échec.
    static bool ParseInt(const char* begin, const char* end, int& out) noexcept
    {
        auto [ptr, ec] = std::from_chars(begin, end, out);
        return ec == std::errc{} && ptr == end;
    }

    Timecode Timecode::FromString(std::string_view s)
    {
        // Formats acceptés :
        //   "HH:MM:SS"       → 8 caractères minimum
        //   "HH:MM:SS.mmm"   → 12 caractères (millisecondes à 1, 2 ou 3 chiffres)

        if (s.size() < 8 || s[2] != ':' || s[5] != ':')
            throw std::invalid_argument(
                std::format("Timecode invalide '{}' : format attendu HH:MM:SS ou HH:MM:SS.mmm", s));

        Timecode tc;
        const char* p = s.data();

        if (!ParseInt(p, p + 2, tc.hours) ||
            !ParseInt(p + 3, p + 5, tc.minutes) ||
            !ParseInt(p + 6, p + 8, tc.seconds))
            throw std::invalid_argument(
                std::format("Timecode invalide '{}' : champs non numériques", s));

        // Millisecondes optionnelles après '.'
        if (s.size() > 8) {
            if (s[8] != '.')
                throw std::invalid_argument(
                    std::format("Timecode invalide '{}' : séparateur de ms attendu '.'", s));

            // Prendre jusqu'à 3 chiffres après le point
            const std::string_view msPart = s.substr(9, 3);
            if (msPart.empty() || msPart.size() > 3)
                throw std::invalid_argument(
                    std::format("Timecode invalide '{}' : millisecondes sur 1 à 3 chiffres", s));

            int raw = 0;
            if (!ParseInt(msPart.data(), msPart.data() + msPart.size(), raw))
                throw std::invalid_argument(
                    std::format("Timecode invalide '{}' : ms non numériques", s));

            // Normaliser à 3 chiffres : "1" → 100 ms, "12" → 120 ms, "123" → 123 ms
            static constexpr int scale[] = { 100, 10, 1 };
            tc.millis = raw * scale[msPart.size() - 1];
        }

        if (!tc.IsValid())
            throw std::invalid_argument(
                std::format("Timecode invalide '{}' : valeurs hors limites", s));

        return tc;
    }

    std::string Timecode::ToString() const
    {
        return std::format("{:02d}:{:02d}:{:02d}.{:03d}", hours, minutes, seconds, millis);
    }


    // ─────────────────────────────────────────────────────────────
    //  Constructeur – ouvre et sonde le fichier source
    // ─────────────────────────────────────────────────────────────
    MediaExtractor::MediaExtractor(std::string inputPath)
        : m_inputPath(std::move(inputPath))
    {
        ProbeInput();
    }

    void MediaExtractor::ProbeInput()
    {
        AVFormatContext* raw = nullptr;
        ThrowIfError(
            avformat_open_input(&raw, m_inputPath.c_str(), nullptr, nullptr),
            "avformat_open_input"
        );
        UniqueInputFmt ctx(raw);

        ThrowIfError(
            avformat_find_stream_info(ctx.get(), nullptr),
            "avformat_find_stream_info"
        );

        if (ctx->duration != AV_NOPTS_VALUE)
            m_durationSecs = static_cast<double>(ctx->duration) / AV_TIME_BASE;

        for (unsigned i = 0; i < ctx->nb_streams; ++i) {
            const auto type = ctx->streams[i]->codecpar->codec_type;
            if (type == AVMEDIA_TYPE_VIDEO) ++m_videoStreamCount;
            else if (type == AVMEDIA_TYPE_AUDIO) ++m_audioStreamCount;
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  Sélection automatique de flux
    // ─────────────────────────────────────────────────────────────
    namespace {

        int FindBestStream(AVFormatContext* ctx, AVMediaType type, int requestedIdx) noexcept
        {
            if (requestedIdx >= 0) return requestedIdx;

            // av_find_best_stream retourne l'index ou une erreur négative
            const int idx = av_find_best_stream(ctx, type, -1, -1, nullptr, 0);
            return (idx >= 0) ? idx : -1;
        }

    } // anonymous namespace
    ExtractionResult MediaExtractor::Extract(
        const std::string& outputPath,
        const TimeRange& range,
        const ExtractionOptions& opts) const
    {
        ExtractionResult result;
        result.outputPath = outputPath;

        if (range.start > 0 && range.end > 0 && !range.IsValid())
        {
            result.errorMessage =
                "TimeRange invalide (start >= end ou valeurs négatives).";
            return result;
        }

        try
        {
            //------------------------------------------------------------------
            // Ouverture entrée
            //------------------------------------------------------------------

            AVFormatContext* rawIn = nullptr;

            ThrowIfError(
                avformat_open_input(
                    &rawIn,
                    m_inputPath.c_str(),
                    nullptr,
                    nullptr),
                "avformat_open_input");

            UniqueInputFmt inCtx(rawIn);

            ThrowIfError(
                avformat_find_stream_info(
                    inCtx.get(),
                    nullptr),
                "avformat_find_stream_info");

            //------------------------------------------------------------------
            // Sélection des flux
            //------------------------------------------------------------------

            const int vIdx =
                (opts.mode != ExtractionMode::AudioOnly)
                ? FindBestStream(
                    inCtx.get(),
                    AVMEDIA_TYPE_VIDEO,
                    opts.videoIdx)
                : -1;

            const int aIdx =
                (opts.mode != ExtractionMode::VideoOnly)
                ? FindBestStream(
                    inCtx.get(),
                    AVMEDIA_TYPE_AUDIO,
                    opts.audioIdx)
                : -1;

            if (vIdx < 0 && aIdx < 0)
                throw std::runtime_error(
                    "Aucun flux vidéo/audio trouvé.");

            //------------------------------------------------------------------
            // Sortie
            //------------------------------------------------------------------

            const unsigned nbStreams = inCtx->nb_streams;

            std::vector<int> streamMap(nbStreams, -1);

            AVFormatContext* rawOut = nullptr;

            ThrowIfError(
                avformat_alloc_output_context2(
                    &rawOut,
                    nullptr,
                    nullptr,
                    outputPath.c_str()),
                "avformat_alloc_output_context2");

            UniqueOutputFmt outCtx(rawOut);

            int outIndex = 0;

            for (unsigned i = 0; i < nbStreams; ++i)
            {
                bool keep =
                    (static_cast<int>(i) == vIdx) ||
                    (static_cast<int>(i) == aIdx);

                if (!keep)
                    continue;

                AVStream* inStream =
                    inCtx->streams[i];

                AVStream* outStream =
                    avformat_new_stream(
                        outCtx.get(),
                        nullptr);

                if (!outStream)
                    throw std::runtime_error(
                        "avformat_new_stream");

                ThrowIfError(
                    avcodec_parameters_copy(
                        outStream->codecpar,
                        inStream->codecpar),
                    "avcodec_parameters_copy");

                outStream->codecpar->codec_tag = 0;

                streamMap[i] = outIndex++;
            }

            if (outIndex == 0)
                throw std::runtime_error(
                    "Aucun flux à copier.");

            //------------------------------------------------------------------
            // Ouverture sortie
            //------------------------------------------------------------------

            if (!(outCtx->oformat->flags & AVFMT_NOFILE))
            {
                ThrowIfError(
                    avio_open(
                        &outCtx->pb,
                        outputPath.c_str(),
                        AVIO_FLAG_WRITE),
                    "avio_open");
            }

            ThrowIfError(
                avformat_write_header(
                    outCtx.get(),
                    nullptr),
                "avformat_write_header");

            //------------------------------------------------------------------
            // Seek
            //------------------------------------------------------------------

            const int64_t seekTarget =
                static_cast<int64_t>(
                    (range.start > 0 ? range.start : 1) * AV_TIME_BASE);

            if (opts.accurate)
            {
                ThrowIfError(
                    avformat_seek_file(
                        inCtx.get(),
                        -1,
                        INT64_MIN,
                        seekTarget,
                        seekTarget,
                        0),
                    "avformat_seek_file");
            }
            else
            {
                ThrowIfError(
                    av_seek_frame(
                        inCtx.get(),
                        -1,
                        seekTarget,
                        AVSEEK_FLAG_BACKWARD),
                    "av_seek_frame");
            }

            //------------------------------------------------------------------
            // Offsets temporels
            //------------------------------------------------------------------

            std::vector<int64_t> firstTs(
                nbStreams,
                AV_NOPTS_VALUE);

            //------------------------------------------------------------------
            // Progression
            //------------------------------------------------------------------

            double endTime = range.end > 0 ? range.end.value_or(m_durationSecs) : m_durationSecs;

            const double totalRange =
                endTime - range.start;

            //------------------------------------------------------------------
            // Lecture
            //------------------------------------------------------------------

            UniquePacket pkt(av_packet_alloc());

            if (!pkt)
                throw std::runtime_error(
                    "av_packet_alloc");

            int64_t bytesWritten = 0;

            while (true)
            {
                int ret =
                    av_read_frame(
                        inCtx.get(),
                        pkt.get());

                if (ret == AVERROR_EOF)
                    break;

                ThrowIfError(
                    ret,
                    "av_read_frame");

                const int srcIdx =
                    pkt->stream_index;

                const int dstIdx =
                    (srcIdx < static_cast<int>(nbStreams))
                    ? streamMap[srcIdx]
                    : -1;

                if (dstIdx < 0)
                {
                    av_packet_unref(pkt.get());
                    continue;
                }

                AVStream* inStream =
                    inCtx->streams[srcIdx];

                AVStream* outStream =
                    outCtx->streams[dstIdx];

                //------------------------------------------------------------------
                // Timestamp source
                //------------------------------------------------------------------

                int64_t ts =
                    (pkt->pts != AV_NOPTS_VALUE)
                    ? pkt->pts
                    : pkt->dts;

                if (ts == AV_NOPTS_VALUE)
                {
                    av_packet_unref(pkt.get());
                    continue;
                }

                const double pktTimeSecs =
                    ts * av_q2d(
                        inStream->time_base);

                //------------------------------------------------------------------
                // Avant début
                //------------------------------------------------------------------

                if (pktTimeSecs < range.start)
                {
                    av_packet_unref(pkt.get());
                    continue;
                }

                //------------------------------------------------------------------
                // Après fin
                //------------------------------------------------------------------

                if (pktTimeSecs > endTime)
                {
                    av_packet_unref(pkt.get());
                    break;
                }

                //------------------------------------------------------------------
                // Initialisation offset flux
                //------------------------------------------------------------------

                if (firstTs[srcIdx] == AV_NOPTS_VALUE)
                    firstTs[srcIdx] = ts;

                //------------------------------------------------------------------
                // Recentrage à zéro
                //------------------------------------------------------------------

                if (pkt->pts != AV_NOPTS_VALUE)
                    pkt->pts -= firstTs[srcIdx];

                if (pkt->dts != AV_NOPTS_VALUE)
                    pkt->dts -= firstTs[srcIdx];

                //------------------------------------------------------------------
                // Conversion time_base
                //------------------------------------------------------------------

                av_packet_rescale_ts(
                    pkt.get(),
                    inStream->time_base,
                    outStream->time_base);

                pkt->stream_index =
                    dstIdx;

                pkt->pos = -1;

                //------------------------------------------------------------------
                // Ecriture
                //------------------------------------------------------------------

                ThrowIfError(
                    av_interleaved_write_frame(
                        outCtx.get(),
                        pkt.get()),
                    "av_interleaved_write_frame");

                bytesWritten += pkt->size;

                //------------------------------------------------------------------
                // Progression
                //------------------------------------------------------------------

                if (opts.progressCallback &&
                    totalRange > 0.0)
                {
                    const double progress =
                        std::clamp(
                            (pktTimeSecs - range.start) /
                            totalRange,
                            0.0,
                            1.0);

                    if (!opts.progressCallback(progress))
                    {
                        av_packet_unref(pkt.get());
                        break;
                    }
                }

                av_packet_unref(pkt.get());
            }

            //------------------------------------------------------------------
            // Finalisation
            //------------------------------------------------------------------

            ThrowIfError(
                av_write_trailer(
                    outCtx.get()),
                "av_write_trailer");

            result.success = true;
            result.bytesWritten = bytesWritten;
            result.durationSecs = totalRange;

            if (opts.progressCallback)
                opts.progressCallback(1.0);
        }
        catch (const std::exception& e)
        {
            result.success = false;
            result.errorMessage = e.what();
        }

        return result;
    }

    // ─────────────────────────────────────────────────────────────
    //  Helper interne commun aux trois fonctions utilitaires
    // ─────────────────────────────────────────────────────────────
    namespace {

        bool RunExtraction(
            const std::string& fileIn,
            const std::string& timestart,
            const std::string& timestop,
            const std::string& fileOut,
            ExtractionMode     mode)
        {
            try {
                const double start = Timecode::FromString(timestart).ToSeconds();
                const double stop = Timecode::FromString(timestop).ToSeconds();

                if(start != 0 && stop != 0)
                    if (stop <= start)
                        return false;

                MediaExtractor extractor(fileIn);

                const TimeRange range{ .start = start, .end = stop };

                ExtractionOptions opts;
                opts.mode = mode;
                opts.accurate = true;

                const ExtractionResult res = extractor.Extract(fileOut, range, opts);
                return res.success;
            }
            catch (...) {
                return false;
            }
        }

    } // anonymous namespace

    // ─────────────────────────────────────────────────────────────
    //  ExecuteFFmpegCutVideo
    //  Coupe la vidéo entre timestart et timestop en conservant
    //  vidéo ET audio (stream copy, sans ré-encodage).
    // ─────────────────────────────────────────────────────────────
    bool ExecuteFFmpegCutVideo(
        const std::string& fileIn,
        const std::string& timestart,
        const std::string& timestop,
        const std::string& fileOut)
    {
        return RunExtraction(fileIn, timestart, timestop, fileOut,
            ExtractionMode::Both);
    }

    // ─────────────────────────────────────────────────────────────
    //  ExecuteFFmpegExtractVideo
    //  Extrait le flux vidéo uniquement (pas d'audio en sortie).
    // ─────────────────────────────────────────────────────────────
    bool ExecuteFFmpegExtractVideo(
        const std::string& fileIn,
        const std::string& timestart,
        const std::string& timestop,
        const std::string& fileOut)
    {
        return RunExtraction(fileIn, timestart, timestop, fileOut,
            ExtractionMode::VideoOnly);
    }

    // ─────────────────────────────────────────────────────────────
    //  ExecuteFFmpegExtractAudio
    //  Extrait le flux audio uniquement (pas de vidéo en sortie).
    // ─────────────────────────────────────────────────────────────
    bool ExecuteFFmpegExtractAudio(
        const std::string& fileIn,
        const std::string& timestart,
        const std::string& timestop,
        const std::string& fileOut)
    {
        return RunExtraction(fileIn, timestart, timestop, fileOut,
            ExtractionMode::AudioOnly);
    }

    // ─────────────────────────────────────────────────────────────
    //  ExecuteFFmpegMuxVideoAudio
    //  Combine le flux vidéo de fileVideo et le flux audio de fileAudio
    //  dans fileOutput (stream copy, sans ré-encodage).
    //
    //  Stratégie :
    //    • On ouvre deux contextes d'entrée indépendants.
    //    • On sélectionne le meilleur flux vidéo dans inVideo
    //      et le meilleur flux audio dans inAudio.
    //    • On crée deux flux dans le contexte de sortie.
    //    • On lit en alternance les paquets des deux sources,
    //      en recalant les timestamps pour qu'ils démarrent à 0.
    // ─────────────────────────────────────────────────────────────
    bool ExecuteFFmpegMuxVideoAudio(
        const std::string& fileVideo,
        const std::string& fileAudio,
        const std::string& fileOutput)
    {
        try
        {
            //==================================================================
            // Ouverture vidéo
            //==================================================================

            AVFormatContext* rawVideo = nullptr;

            ThrowIfError(
                avformat_open_input(
                    &rawVideo,
                    fileVideo.c_str(),
                    nullptr,
                    nullptr),
                "avformat_open_input(video)");

            UniqueInputFmt inVideo(rawVideo);

            ThrowIfError(
                avformat_find_stream_info(
                    inVideo.get(),
                    nullptr),
                "avformat_find_stream_info(video)");

            //==================================================================
            // Ouverture audio
            //==================================================================

            AVFormatContext* rawAudio = nullptr;

            ThrowIfError(
                avformat_open_input(
                    &rawAudio,
                    fileAudio.c_str(),
                    nullptr,
                    nullptr),
                "avformat_open_input(audio)");

            UniqueInputFmt inAudio(rawAudio);

            ThrowIfError(
                avformat_find_stream_info(
                    inAudio.get(),
                    nullptr),
                "avformat_find_stream_info(audio)");

            //==================================================================
            // Recherche des flux
            //==================================================================

            const int vIdx =
                av_find_best_stream(
                    inVideo.get(),
                    AVMEDIA_TYPE_VIDEO,
                    -1,
                    -1,
                    nullptr,
                    0);

            if (vIdx < 0)
                throw std::runtime_error("Flux vidéo introuvable");

            const int aIdx =
                av_find_best_stream(
                    inAudio.get(),
                    AVMEDIA_TYPE_AUDIO,
                    -1,
                    -1,
                    nullptr,
                    0);

            if (aIdx < 0)
                throw std::runtime_error("Flux audio introuvable");

            AVStream* srcVideo = inVideo->streams[vIdx];
            AVStream* srcAudio = inAudio->streams[aIdx];

            //==================================================================
            // Sortie
            //==================================================================

            AVFormatContext* rawOut = nullptr;

            ThrowIfError(
                avformat_alloc_output_context2(
                    &rawOut,
                    nullptr,
                    nullptr,
                    fileOutput.c_str()),
                "avformat_alloc_output_context2");

            UniqueOutputFmt outCtx(rawOut);

            AVStream* outVideo =
                avformat_new_stream(outCtx.get(), nullptr);

            if (!outVideo)
                throw std::runtime_error("Création flux vidéo impossible");

            ThrowIfError(
                avcodec_parameters_copy(
                    outVideo->codecpar,
                    srcVideo->codecpar),
                "copy video codecpar");

            outVideo->codecpar->codec_tag = 0;

            AVStream* outAudio =
                avformat_new_stream(outCtx.get(), nullptr);

            if (!outAudio)
                throw std::runtime_error("Création flux audio impossible");

            ThrowIfError(
                avcodec_parameters_copy(
                    outAudio->codecpar,
                    srcAudio->codecpar),
                "copy audio codecpar");

            outAudio->codecpar->codec_tag = 0;

            //==================================================================
            // Ouverture fichier sortie
            //==================================================================

            if (!(outCtx->oformat->flags & AVFMT_NOFILE))
            {
                ThrowIfError(
                    avio_open(
                        &outCtx->pb,
                        fileOutput.c_str(),
                        AVIO_FLAG_WRITE),
                    "avio_open");
            }

            ThrowIfError(
                avformat_write_header(
                    outCtx.get(),
                    nullptr),
                "avformat_write_header");

            //==================================================================
            // Paquets indépendants
            //==================================================================

            UniquePacket videoPkt(av_packet_alloc());
            UniquePacket audioPkt(av_packet_alloc());

            if (!videoPkt || !audioPkt)
                throw std::runtime_error("av_packet_alloc");

            bool videoEof = false;
            bool audioEof = false;

            bool haveVideo = false;
            bool haveAudio = false;

            auto ReadVideoPacket = [&]()
                {
                    av_packet_unref(videoPkt.get());

                    while (true)
                    {
                        int ret = av_read_frame(
                            inVideo.get(),
                            videoPkt.get());

                        if (ret == AVERROR_EOF)
                        {
                            videoEof = true;
                            return false;
                        }

                        ThrowIfError(ret, "read video");

                        if (videoPkt->stream_index == vIdx)
                            return true;

                        av_packet_unref(videoPkt.get());
                    }
                };

            auto ReadAudioPacket = [&]()
                {
                    av_packet_unref(audioPkt.get());

                    while (true)
                    {
                        int ret = av_read_frame(
                            inAudio.get(),
                            audioPkt.get());

                        if (ret == AVERROR_EOF)
                        {
                            audioEof = true;
                            return false;
                        }

                        ThrowIfError(ret, "read audio");

                        if (audioPkt->stream_index == aIdx)
                            return true;

                        av_packet_unref(audioPkt.get());
                    }
                };

            haveVideo = ReadVideoPacket();
            haveAudio = ReadAudioPacket();

            //==================================================================
            // Offsets pour démarrer à t=0
            //==================================================================

            int64_t firstVideoDts = AV_NOPTS_VALUE;
            int64_t firstAudioDts = AV_NOPTS_VALUE;

            //==================================================================
            // Mux
            //==================================================================

            while (haveVideo || haveAudio)
            {
                bool writeVideo;

                if (!haveAudio)
                {
                    writeVideo = true;
                }
                else if (!haveVideo)
                {
                    writeVideo = false;
                }
                else
                {
                    int64_t vDts =
                        (videoPkt->dts == AV_NOPTS_VALUE)
                        ? INT64_MAX
                        : av_rescale_q(
                            videoPkt->dts,
                            srcVideo->time_base,
                            AV_TIME_BASE_Q);

                    int64_t aDts =
                        (audioPkt->dts == AV_NOPTS_VALUE)
                        ? INT64_MAX
                        : av_rescale_q(
                            audioPkt->dts,
                            srcAudio->time_base,
                            AV_TIME_BASE_Q);

                    writeVideo = (vDts <= aDts);
                }

                if (writeVideo)
                {
                    if (firstVideoDts == AV_NOPTS_VALUE)
                    {
                        firstVideoDts =
                            (videoPkt->dts != AV_NOPTS_VALUE)
                            ? videoPkt->dts
                            : 0;
                    }

                    if (videoPkt->pts != AV_NOPTS_VALUE)
                        videoPkt->pts -= firstVideoDts;

                    if (videoPkt->dts != AV_NOPTS_VALUE)
                        videoPkt->dts -= firstVideoDts;

                    av_packet_rescale_ts(
                        videoPkt.get(),
                        srcVideo->time_base,
                        outVideo->time_base);

                    videoPkt->stream_index = outVideo->index;
                    videoPkt->pos = -1;

                    ThrowIfError(
                        av_interleaved_write_frame(
                            outCtx.get(),
                            videoPkt.get()),
                        "write video");

                    haveVideo = ReadVideoPacket();
                }
                else
                {
                    if (firstAudioDts == AV_NOPTS_VALUE)
                    {
                        firstAudioDts =
                            (audioPkt->dts != AV_NOPTS_VALUE)
                            ? audioPkt->dts
                            : 0;
                    }

                    if (audioPkt->pts != AV_NOPTS_VALUE)
                        audioPkt->pts -= firstAudioDts;

                    if (audioPkt->dts != AV_NOPTS_VALUE)
                        audioPkt->dts -= firstAudioDts;

                    av_packet_rescale_ts(
                        audioPkt.get(),
                        srcAudio->time_base,
                        outAudio->time_base);

                    audioPkt->stream_index = outAudio->index;
                    audioPkt->pos = -1;

                    ThrowIfError(
                        av_interleaved_write_frame(
                            outCtx.get(),
                            audioPkt.get()),
                        "write audio");

                    haveAudio = ReadAudioPacket();
                }
            }

            ThrowIfError(
                av_write_trailer(outCtx.get()),
                "av_write_trailer");

            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    // ─────────────────────────────────────────────────────────────
//  ExecuteFFmpegMuxVideoAudio
//  Combine le flux vidéo de fileVideo et le flux audio de fileAudio
//  dans fileOutput (stream copy, sans ré-encodage).
//
//  Stratégie :
//    • On ouvre deux contextes d'entrée indépendants.
//    • On sélectionne le meilleur flux vidéo dans inVideo
//      et le meilleur flux audio dans inAudio.
//    • On crée deux flux dans le contexte de sortie.
//    • On lit en alternance les paquets des deux sources,
//      en recalant les timestamps pour qu'ils démarrent à 0.
// ─────────────────────────────────────────────────────────────
    bool CreateLoopedAudio(
        const std::string& inputAudioFile,
        const std::string& outputFile,
        const std::string& timeVideo)
    {
        try
        {
            //------------------------------------------------------------------
            // Ouverture entrée
            //------------------------------------------------------------------
            const double targetDurationSeconds = Timecode::FromString(timeVideo).ToSeconds();
            AVFormatContext* rawIn = nullptr;

            ThrowIfError(
                avformat_open_input(
                    &rawIn,
                    inputAudioFile.c_str(),
                    nullptr,
                    nullptr),
                "avformat_open_input");

            UniqueInputFmt inCtx(rawIn);

            ThrowIfError(
                avformat_find_stream_info(
                    inCtx.get(),
                    nullptr),
                "avformat_find_stream_info");

            //------------------------------------------------------------------
            // Flux audio
            //------------------------------------------------------------------

            const int audioIdx =
                av_find_best_stream(
                    inCtx.get(),
                    AVMEDIA_TYPE_AUDIO,
                    -1,
                    -1,
                    nullptr,
                    0);

            if (audioIdx < 0)
                throw std::runtime_error(
                    "Aucun flux audio trouvé.");

            AVStream* inStream =
                inCtx->streams[audioIdx];

            //------------------------------------------------------------------
            // Durée audio
            //------------------------------------------------------------------

            int64_t streamDurationTs =
                inStream->duration;

            if (streamDurationTs <= 0)
            {
                throw std::runtime_error(
                    "Durée du flux audio inconnue.");
            }

            const double streamDurationSecs =
                streamDurationTs *
                av_q2d(inStream->time_base);

            if (streamDurationSecs <= 0.0)
            {
                throw std::runtime_error(
                    "Durée audio invalide.");
            }

            //------------------------------------------------------------------
            // Sortie
            //------------------------------------------------------------------

            AVFormatContext* rawOut = nullptr;

            ThrowIfError(
                avformat_alloc_output_context2(
                    &rawOut,
                    nullptr,
                    nullptr,
                    outputFile.c_str()),
                "avformat_alloc_output_context2");

            UniqueOutputFmt outCtx(rawOut);

            AVStream* outStream =
                avformat_new_stream(
                    outCtx.get(),
                    nullptr);

            if (!outStream)
                throw std::runtime_error(
                    "avformat_new_stream");

            ThrowIfError(
                avcodec_parameters_copy(
                    outStream->codecpar,
                    inStream->codecpar),
                "avcodec_parameters_copy");

            outStream->codecpar->codec_tag = 0;

            //------------------------------------------------------------------
            // Ouverture sortie
            //------------------------------------------------------------------

            if (!(outCtx->oformat->flags & AVFMT_NOFILE))
            {
                ThrowIfError(
                    avio_open(
                        &outCtx->pb,
                        outputFile.c_str(),
                        AVIO_FLAG_WRITE),
                    "avio_open");
            }

            ThrowIfError(
                avformat_write_header(
                    outCtx.get(),
                    nullptr),
                "avformat_write_header");

            //------------------------------------------------------------------
            // Variables boucle
            //------------------------------------------------------------------

            UniquePacket pkt(av_packet_alloc());

            if (!pkt)
                throw std::runtime_error(
                    "av_packet_alloc");

            int64_t ptsOffset = 0;
            int64_t firstPts = AV_NOPTS_VALUE;

            double producedDuration = 0.0;

            //------------------------------------------------------------------
            // Boucle principale
            //------------------------------------------------------------------

            while (producedDuration < targetDurationSeconds)
            {
                ThrowIfError(
                    av_seek_frame(
                        inCtx.get(),
                        audioIdx,
                        0,
                        AVSEEK_FLAG_BACKWARD),
                    "av_seek_frame");

                while (true)
                {
                    int ret =
                        av_read_frame(
                            inCtx.get(),
                            pkt.get());

                    if (ret == AVERROR_EOF)
                        break;

                    ThrowIfError(
                        ret,
                        "av_read_frame");

                    if (pkt->stream_index != audioIdx)
                    {
                        av_packet_unref(pkt.get());
                        continue;
                    }

                    //----------------------------------------------------------
                    // Timestamp de référence
                    //----------------------------------------------------------

                    int64_t ts =
                        (pkt->pts != AV_NOPTS_VALUE)
                        ? pkt->pts
                        : pkt->dts;

                    if (ts == AV_NOPTS_VALUE)
                    {
                        av_packet_unref(pkt.get());
                        continue;
                    }

                    if (firstPts == AV_NOPTS_VALUE)
                        firstPts = ts;

                    //----------------------------------------------------------
                    // Temps absolu produit
                    //----------------------------------------------------------

                    const double currentTime =
                        producedDuration +
                        ((ts - firstPts) *
                            av_q2d(inStream->time_base));

                    if (currentTime >= targetDurationSeconds)
                    {
                        av_packet_unref(pkt.get());
                        goto finish;
                    }

                    //----------------------------------------------------------
                    // Décalage timestamps
                    //----------------------------------------------------------

                    if (pkt->pts != AV_NOPTS_VALUE)
                    {
                        pkt->pts =
                            pkt->pts -
                            firstPts +
                            ptsOffset;
                    }

                    if (pkt->dts != AV_NOPTS_VALUE)
                    {
                        pkt->dts =
                            pkt->dts -
                            firstPts +
                            ptsOffset;
                    }

                    //----------------------------------------------------------
                    // Conversion vers time_base sortie
                    //----------------------------------------------------------

                    av_packet_rescale_ts(
                        pkt.get(),
                        inStream->time_base,
                        outStream->time_base);

                    pkt->stream_index =
                        outStream->index;

                    pkt->pos = -1;

                    //----------------------------------------------------------
                    // Écriture
                    //----------------------------------------------------------

                    ThrowIfError(
                        av_interleaved_write_frame(
                            outCtx.get(),
                            pkt.get()),
                        "av_interleaved_write_frame");

                    av_packet_unref(pkt.get());
                }

                //--------------------------------------------------------------
                // Boucle suivante
                //--------------------------------------------------------------

                ptsOffset += streamDurationTs;
                producedDuration += streamDurationSecs;
            }

        finish:

            ThrowIfError(
                av_write_trailer(
                    outCtx.get()),
                "av_write_trailer");

            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

} // namespace Regards::Media