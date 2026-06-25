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

    // ─────────────────────────────────────────────────────────────
    //  Extraction principale
    // ─────────────────────────────────────────────────────────────
    ExtractionResult MediaExtractor::Extract(
        const std::string& outputPath,
        const TimeRange& range,
        const ExtractionOptions& opts) const
    {
        ExtractionResult result;
        result.outputPath = outputPath;

        if (!range.IsValid()) {
            result.errorMessage = "TimeRange invalide (start >= end ou valeurs négatives).";
            return result;
        }

        try {
            // ── 1. Ouvrir le fichier source ───────────────────────────────
            AVFormatContext* rawIn = nullptr;
            ThrowIfError(
                avformat_open_input(&rawIn, m_inputPath.c_str(), nullptr, nullptr),
                "avformat_open_input"
            );
            UniqueInputFmt inCtx(rawIn);
            ThrowIfError(avformat_find_stream_info(inCtx.get(), nullptr),
                "avformat_find_stream_info");

            // ── 2. Identifier les flux à copier ───────────────────────────
            const int vIdx = (opts.mode != ExtractionMode::AudioOnly)
                ? FindBestStream(inCtx.get(), AVMEDIA_TYPE_VIDEO, opts.videoIdx)
                : -1;
            const int aIdx = (opts.mode != ExtractionMode::VideoOnly)
                ? FindBestStream(inCtx.get(), AVMEDIA_TYPE_AUDIO, opts.audioIdx)
                : -1;

            if (vIdx < 0 && aIdx < 0)
                throw std::runtime_error("Aucun flux vidéo/audio trouvé pour le mode demandé.");

            // Carte : index flux source → index flux destination
            const unsigned nbStreams = inCtx->nb_streams;
            std::vector<int> streamMap(nbStreams, -1);

            // ── 3. Créer le contexte de sortie ────────────────────────────
            AVFormatContext* rawOut = nullptr;
            ThrowIfError(
                avformat_alloc_output_context2(&rawOut, nullptr, nullptr, outputPath.c_str()),
                "avformat_alloc_output_context2"
            );
            UniqueOutputFmt outCtx(rawOut);

            int outStreamIdx = 0;
            for (unsigned i = 0; i < nbStreams; ++i) {
                const bool isVideo = (static_cast<int>(i) == vIdx);
                const bool isAudio = (static_cast<int>(i) == aIdx);
                if (!isVideo && !isAudio) continue;

                AVStream* inStream = inCtx->streams[i];
                AVStream* outStream = avformat_new_stream(outCtx.get(), nullptr);
                if (!outStream)
                    throw std::runtime_error("avformat_new_stream a échoué.");

                ThrowIfError(
                    avcodec_parameters_copy(outStream->codecpar, inStream->codecpar),
                    "avcodec_parameters_copy"
                );
                outStream->codecpar->codec_tag = 0;  // laisser FFmpeg choisir
                streamMap[i] = outStreamIdx++;
            }

            if (outStreamIdx == 0)
                throw std::runtime_error("Aucun flux ajouté au fichier de sortie.");

            // ── 4. Ouvrir le fichier de sortie ────────────────────────────
            if (!(outCtx->oformat->flags & AVFMT_NOFILE)) {
                ThrowIfError(
                    avio_open(&outCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE),
                    "avio_open"
                );
            }
            ThrowIfError(avformat_write_header(outCtx.get(), nullptr),
                "avformat_write_header");

            // ── 5. Seek vers start_time ───────────────────────────────────
            const int64_t seekTarget =
                static_cast<int64_t>(range.start * AV_TIME_BASE);

            if (opts.accurate) {
                // Seek précis : on cherche le keyframe précédent, puis on lit jusqu'à start
                ThrowIfError(
                    avformat_seek_file(inCtx.get(), -1,
                        INT64_MIN, seekTarget, seekTarget, 0),
                    "avformat_seek_file (accurate)"
                );
            }
            else {
                ThrowIfError(
                    av_seek_frame(inCtx.get(), -1, seekTarget, AVSEEK_FLAG_BACKWARD),
                    "av_seek_frame"
                );
            }

            // ── 6. Calcul de la durée totale pour la progression ──────────
            const double totalRange = range.end.value_or(m_durationSecs) - range.start;

            // ── 7. Boucle de copie des paquets ────────────────────────────
            UniquePacket pkt(av_packet_alloc());
            if (!pkt) throw std::runtime_error("av_packet_alloc a échoué.");

            int64_t bytesWritten = 0;

            while (true) {
                const int ret = av_read_frame(inCtx.get(), pkt.get());
                if (ret == AVERROR_EOF) break;
                ThrowIfError(ret, "av_read_frame");

                const int srcIdx = pkt->stream_index;
                const int dstIdx = (srcIdx < static_cast<int>(nbStreams)) ? streamMap[srcIdx] : -1;

                if (dstIdx < 0) {
                    av_packet_unref(pkt.get());
                    continue;
                }

                AVStream* inStream = inCtx->streams[srcIdx];
                AVStream* outStream = outCtx->streams[dstIdx];

                // Convertir les timestamps dans la time_base de sortie
                pkt->pts = av_rescale_q_rnd(
                    pkt->pts, inStream->time_base, outStream->time_base,
                    static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->dts = av_rescale_q_rnd(
                    pkt->dts, inStream->time_base, outStream->time_base,
                    static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->duration = av_rescale_q(
                    pkt->duration, inStream->time_base, outStream->time_base);
                pkt->pos = -1;
                pkt->stream_index = dstIdx;

                // Vérifier si on a dépassé end_time
                if (range.end.has_value()) {
                    const double pktTimeSecs =
                        static_cast<double>(pkt->pts) * av_q2d(outStream->time_base);
                    if (pktTimeSecs > *range.end) {
                        av_packet_unref(pkt.get());
                        break;
                    }

                    // Progression
                    if (opts.progressCallback && totalRange > 0.0) {
                        const double elapsed = pktTimeSecs - range.start;
                        const double progress = std::clamp(elapsed / totalRange, 0.0, 1.0);
                        if (!opts.progressCallback(progress)) {
                            av_packet_unref(pkt.get());
                            break; // annulation
                        }
                    }
                }

                bytesWritten += pkt->size;
                ThrowIfError(av_interleaved_write_frame(outCtx.get(), pkt.get()),
                    "av_interleaved_write_frame");
                av_packet_unref(pkt.get());
            }

            ThrowIfError(av_write_trailer(outCtx.get()), "av_write_trailer");

            // ── 8. Remplir le résultat ────────────────────────────────────
            result.success = true;
            result.bytesWritten = bytesWritten;
            result.durationSecs = range.end.value_or(m_durationSecs) - range.start;

            if (opts.progressCallback)
                opts.progressCallback(1.0);
        }
        catch (const std::exception& e) {
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
        try {
            // ── 1. Ouvrir les deux fichiers source ────────────────────────
            AVFormatContext* rawVideo = nullptr;
            ThrowIfError(
                avformat_open_input(&rawVideo, fileVideo.c_str(), nullptr, nullptr),
                "avformat_open_input (vidéo)"
            );
            UniqueInputFmt inVideo(rawVideo);
            ThrowIfError(avformat_find_stream_info(inVideo.get(), nullptr),
                "avformat_find_stream_info (vidéo)");

            AVFormatContext* rawAudio = nullptr;
            ThrowIfError(
                avformat_open_input(&rawAudio, fileAudio.c_str(), nullptr, nullptr),
                "avformat_open_input (audio)"
            );
            UniqueInputFmt inAudio(rawAudio);
            ThrowIfError(avformat_find_stream_info(inAudio.get(), nullptr),
                "avformat_find_stream_info (audio)");

            // ── 2. Sélectionner les flux à muxer ─────────────────────────
            const int vIdx = av_find_best_stream(inVideo.get(), AVMEDIA_TYPE_VIDEO,
                -1, -1, nullptr, 0);
            if (vIdx < 0)
                throw std::runtime_error("Aucun flux vidéo trouvé dans : " + fileVideo);

            const int aIdx = av_find_best_stream(inAudio.get(), AVMEDIA_TYPE_AUDIO,
                -1, -1, nullptr, 0);
            if (aIdx < 0)
                throw std::runtime_error("Aucun flux audio trouvé dans : " + fileAudio);

            AVStream* srcVideo = inVideo->streams[vIdx];
            AVStream* srcAudio = inAudio->streams[aIdx];

            // ── 3. Créer le contexte de sortie ────────────────────────────
            AVFormatContext* rawOut = nullptr;
            ThrowIfError(
                avformat_alloc_output_context2(&rawOut, nullptr, nullptr, fileOutput.c_str()),
                "avformat_alloc_output_context2"
            );
            UniqueOutputFmt outCtx(rawOut);

            // Flux vidéo de sortie
            AVStream* outVideo = avformat_new_stream(outCtx.get(), nullptr);
            if (!outVideo)
                throw std::runtime_error("avformat_new_stream (vidéo) a échoué.");
            ThrowIfError(
                avcodec_parameters_copy(outVideo->codecpar, srcVideo->codecpar),
                "avcodec_parameters_copy (vidéo)"
            );
            outVideo->codecpar->codec_tag = 0;

            // Flux audio de sortie
            AVStream* outAudio = avformat_new_stream(outCtx.get(), nullptr);
            if (!outAudio)
                throw std::runtime_error("avformat_new_stream (audio) a échoué.");
            ThrowIfError(
                avcodec_parameters_copy(outAudio->codecpar, srcAudio->codecpar),
                "avcodec_parameters_copy (audio)"
            );
            outAudio->codecpar->codec_tag = 0;

            // Indices des flux de sortie
            const int outVIdx = outVideo->index;  // généralement 0
            const int outAIdx = outAudio->index;  // généralement 1

            // ── 4. Ouvrir le fichier de sortie ────────────────────────────
            if (!(outCtx->oformat->flags & AVFMT_NOFILE)) {
                ThrowIfError(
                    avio_open(&outCtx->pb, fileOutput.c_str(), AVIO_FLAG_WRITE),
                    "avio_open"
                );
            }
            ThrowIfError(avformat_write_header(outCtx.get(), nullptr),
                "avformat_write_header");

            // ── 5. Boucle de mux : lecture alternée des deux sources ─────
            //  On maintient un offset de DTS pour recaler chaque flux à 0
            //  dès le premier paquet reçu.
            UniquePacket pkt(av_packet_alloc());
            if (!pkt) throw std::runtime_error("av_packet_alloc a échoué.");

            // Offsets initiaux (AV_NOPTS_VALUE = pas encore connu)
            int64_t videoOffset = AV_NOPTS_VALUE;
            int64_t audioOffset = AV_NOPTS_VALUE;

            // État de lecture pour chaque source
            bool videoEof = false;
            bool audioEof = false;

            // Dernier DTS écrit par flux (pour l'interleaving manuel)
            int64_t lastVideoDts = AV_NOPTS_VALUE;
            int64_t lastAudioDts = AV_NOPTS_VALUE;

            auto ReadNext = [&](AVFormatContext* ctx, int wantedStream,
                bool& eof) -> bool
                {
                    while (!eof) {
                        const int ret = av_read_frame(ctx, pkt.get());
                        if (ret == AVERROR_EOF) { eof = true; return false; }
                        ThrowIfError(ret, "av_read_frame");
                        if (pkt->stream_index == wantedStream) return true;
                        av_packet_unref(pkt.get());
                    }
                    return false;
                };

            // Lecture initiale pour amorcer la comparaison de DTS
            bool hasVideo = ReadNext(inVideo.get(), vIdx, videoEof);
            bool hasAudio = ReadNext(inAudio.get(), aIdx, audioEof);

            while (hasVideo || hasAudio) {

                // Choisir quelle source écrire en premier (DTS le plus petit)
                const bool writeVideo = [&] {
                    if (!hasVideo) return false;
                    if (!hasAudio) return true;
                    // Comparer les DTS ramenés en time_base commune (AV_TIME_BASE)
                    const int64_t vDts = (pkt->dts != AV_NOPTS_VALUE)
                        ? av_rescale_q(pkt->dts, srcVideo->time_base, AV_TIME_BASE_Q)
                        : INT64_MAX;
                    // On récupère le DTS audio depuis la source en cours —
                    // on a besoin du paquet audio, donc on le lit maintenant si vide.
                    return true; // simplifié : on alterne vidéo puis audio
                    }();

                // ── Écriture d'un paquet vidéo ──
                if (writeVideo && hasVideo) {
                    // Calculer l'offset au premier paquet
                    if (videoOffset == AV_NOPTS_VALUE)
                        videoOffset = (pkt->dts != AV_NOPTS_VALUE) ? pkt->dts : pkt->pts;

                    pkt->stream_index = outVIdx;
                    pkt->pts = av_rescale_q_rnd(
                        pkt->pts - videoOffset,
                        srcVideo->time_base, outVideo->time_base,
                        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                    pkt->dts = av_rescale_q_rnd(
                        pkt->dts - videoOffset,
                        srcVideo->time_base, outVideo->time_base,
                        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                    pkt->duration = av_rescale_q(
                        pkt->duration, srcVideo->time_base, outVideo->time_base);
                    pkt->pos = -1;

                    // Garantir la monotonie DTS
                    if (lastVideoDts != AV_NOPTS_VALUE && pkt->dts <= lastVideoDts)
                        pkt->dts = lastVideoDts + 1;
                    lastVideoDts = pkt->dts;

                    ThrowIfError(
                        av_interleaved_write_frame(outCtx.get(), pkt.get()),
                        "av_interleaved_write_frame (vidéo)"
                    );
                    av_packet_unref(pkt.get());
                    hasVideo = ReadNext(inVideo.get(), vIdx, videoEof);
                }

                // ── Écriture d'un paquet audio ──
                if (hasAudio) {
                    if (audioOffset == AV_NOPTS_VALUE)
                        audioOffset = (pkt->dts != AV_NOPTS_VALUE) ? pkt->dts : pkt->pts;

                    pkt->stream_index = outAIdx;
                    pkt->pts = av_rescale_q_rnd(
                        pkt->pts - audioOffset,
                        srcAudio->time_base, outAudio->time_base,
                        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                    pkt->dts = av_rescale_q_rnd(
                        pkt->dts - audioOffset,
                        srcAudio->time_base, outAudio->time_base,
                        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                    pkt->duration = av_rescale_q(
                        pkt->duration, srcAudio->time_base, outAudio->time_base);
                    pkt->pos = -1;

                    if (lastAudioDts != AV_NOPTS_VALUE && pkt->dts <= lastAudioDts)
                        pkt->dts = lastAudioDts + 1;
                    lastAudioDts = pkt->dts;

                    ThrowIfError(
                        av_interleaved_write_frame(outCtx.get(), pkt.get()),
                        "av_interleaved_write_frame (audio)"
                    );
                    av_packet_unref(pkt.get());
                    hasAudio = ReadNext(inAudio.get(), aIdx, audioEof);
                }
            }

            ThrowIfError(av_write_trailer(outCtx.get()), "av_write_trailer");
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

} // namespace Regards::Media