#pragma once

#include <string>
#include <optional>
#include <stdexcept>
#include <functional>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
}

namespace Regards::Media {

    // ─────────────────────────────────────────────────────────────
    //  Timecode  — format "HH:MM:SS" ou "HH:MM:SS.mmm"
    // ─────────────────────────────────────────────────────────────
    struct Timecode {
        int    hours = 0;
        int    minutes = 0;
        int    seconds = 0;
        int    millis = 0;   // millisecondes (0–999)

        // ── Constructeur depuis une chaîne "HH:MM:SS" ou "HH:MM:SS.mmm" ──
        // Lève std::invalid_argument si le format est incorrect.
        static Timecode FromString(std::string_view s);

        // ── Conversion en secondes (double) ──
        [[nodiscard]] double ToSeconds() const noexcept {
            return hours * 3600.0 + minutes * 60.0 + seconds + millis / 1000.0;
        }

        // ── Reconstitution de la chaîne "HH:MM:SS.mmm" ──
        [[nodiscard]] std::string ToString() const;

        // ── Validation ──
        [[nodiscard]] bool IsValid() const noexcept {
            return hours >= 0
                && minutes >= 0 && minutes < 60
                && seconds >= 0 && seconds < 60
                && millis >= 0 && millis < 1000;
        }
    };

    // ─────────────────────────────────────────────────────────────
    //  Période d'extraction (en secondes, double précision)
    // ─────────────────────────────────────────────────────────────
    struct TimeRange {
        double start = 0.0;             // début en secondes
        std::optional<double> end;      // fin  en secondes (nullopt = jusqu'à la fin)

        [[nodiscard]] bool IsValid() const noexcept {
            return start >= 0.0 && (!end.has_value() || *end > start);
        }

        [[nodiscard]] std::optional<double> Duration() const noexcept {
            if (!end) return std::nullopt;
            return *end - start;
        }
    };

    // ─────────────────────────────────────────────────────────────
    //  Mode d'extraction
    // ─────────────────────────────────────────────────────────────
    enum class ExtractionMode {
        VideoOnly,   // flux vidéo uniquement (stream copy)
        AudioOnly,   // flux audio uniquement (stream copy)
        Both         // vidéo + audio (stream copy)
    };

    // ─────────────────────────────────────────────────────────────
    //  Options avancées
    // ─────────────────────────────────────────────────────────────
    struct ExtractionOptions {
        ExtractionMode mode = ExtractionMode::Both;
        int            videoIdx = -1;   // -1 = sélection automatique
        int            audioIdx = -1;   // -1 = sélection automatique
        bool           accurate = true; // seek précis (plus lent mais exact)

        // Callback de progression : reçoit [0.0 … 1.0], renvoie false pour annuler
        std::function<bool(double)> progressCallback;
    };

    // ─────────────────────────────────────────────────────────────
    //  Résultat
    // ─────────────────────────────────────────────────────────────
    struct ExtractionResult {
        bool        success = false;
        std::string outputPath;
        double      durationSecs = 0.0;
        int64_t     bytesWritten = 0;
        std::string errorMessage;
    };

    // ─────────────────────────────────────────────────────────────
    //  Wrappers RAII pour les ressources FFmpeg
    // ─────────────────────────────────────────────────────────────
    namespace detail {

        struct FormatContextDeleter {
            void operator()(AVFormatContext* ctx) const noexcept {
                if (ctx) avformat_close_input(&ctx);
            }
        };
        struct OutputContextDeleter {
            void operator()(AVFormatContext* ctx) const noexcept {
                if (ctx) {
                    if (!(ctx->oformat->flags & AVFMT_NOFILE))
                        avio_closep(&ctx->pb);
                    avformat_free_context(ctx);
                }
            }
        };
        struct PacketDeleter {
            void operator()(AVPacket* pkt) const noexcept {
                if (pkt) av_packet_free(&pkt);
            }
        };

        using UniqueInputFmt = std::unique_ptr<AVFormatContext, FormatContextDeleter>;
        using UniqueOutputFmt = std::unique_ptr<AVFormatContext, OutputContextDeleter>;
        using UniquePacket = std::unique_ptr<AVPacket, PacketDeleter>;

    } // namespace detail

    // ─────────────────────────────────────────────────────────────
    //  Classe principale
    // ─────────────────────────────────────────────────────────────
    class MediaExtractor {
    public:
        explicit MediaExtractor(std::string inputPath);
        ~MediaExtractor() = default;

        // Non-copiable, déplaçable
        MediaExtractor(const MediaExtractor&) = delete;
        MediaExtractor& operator=(const MediaExtractor&) = delete;
        MediaExtractor(MediaExtractor&&) = default;
        MediaExtractor& operator=(MediaExtractor&&) = default;

        /// Extrait le média vers outputPath selon options et range.
        [[nodiscard]] ExtractionResult Extract(
            const std::string& outputPath,
            const TimeRange& range,
            const ExtractionOptions& opts = {}) const;

        /// Informations sur le fichier source
        [[nodiscard]] double   TotalDuration()    const noexcept { return m_durationSecs; }
        [[nodiscard]] int      VideoStreamCount() const noexcept { return m_videoStreamCount; }
        [[nodiscard]] int      AudioStreamCount() const noexcept { return m_audioStreamCount; }
        [[nodiscard]] const std::string& InputPath() const noexcept { return m_inputPath; }

    private:
        std::string m_inputPath;
        double      m_durationSecs = 0.0;
        int         m_videoStreamCount = 0;
        int         m_audioStreamCount = 0;

        void        ProbeInput();


    };

    // ─────────────────────────────────────────────────────────────
    //  Fonctions utilitaires de haut niveau
    //
    //  timestart / timestop : format "HH:MM:SS" ou "HH:MM:SS.mmm"
    //  Retournent true si l'opération s'est terminée avec succès.
    // ─────────────────────────────────────────────────────────────

    /// Coupe une vidéo entre timestart et timestop en conservant
    /// les flux vidéo ET audio (stream copy, sans ré-encodage).
    [[nodiscard]] bool ExecuteFFmpegCutVideo(
        const std::string& fileIn,
        const std::string& timestart,
        const std::string& timestop,
        const std::string& fileOut);

    /// Extrait le flux vidéo uniquement entre timestart et timestop
    /// (pas d'audio dans le fichier de sortie).
    [[nodiscard]] bool ExecuteFFmpegExtractVideo(
        const std::string& fileIn,
        const std::string& timestart,
        const std::string& timestop,
        const std::string& fileOut);

    /// Extrait le flux audio uniquement entre timestart et timestop
    /// (pas de vidéo dans le fichier de sortie).
    [[nodiscard]] bool ExecuteFFmpegExtractAudio(
        const std::string& fileIn,
        const std::string& timestart,
        const std::string& timestop,
        const std::string& fileOut);

    /// Muxe un fichier vidéo (flux vidéo) et un fichier audio (flux audio)
    /// dans un unique conteneur de sortie (stream copy, sans ré-encodage).
    /// Si fileVideo contient déjà de l'audio, seul le flux vidéo est retenu.
    /// Si fileAudio contient déjà de la vidéo, seul le flux audio est retenu.
    [[nodiscard]] bool ExecuteFFmpegMuxVideoAudio(
        const std::string& fileVideo,
        const std::string& fileAudio,
        const std::string& fileOutput);

} // namespace Regards::Media