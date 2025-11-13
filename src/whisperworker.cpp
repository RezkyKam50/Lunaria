#include "whisperworker.h"
#include <QDebug>

WhisperWorker::WhisperWorker(QObject *parent)
    : QObject(parent)
{
}

WhisperWorker::~WhisperWorker()
{
    if (ctx) {
        whisper_free(ctx);
        ctx = nullptr;
    }
}

void WhisperWorker::loadModel(const QString &modelPath)
{
    try {
        if (ctx) {
            whisper_free(ctx);
            ctx = nullptr;
        }
         
        struct whisper_context_params cparams = whisper_context_default_params();
        cparams.use_gpu = true;  
        
        ctx = whisper_init_from_file_with_params(modelPath.toStdString().c_str(), cparams);
        
        if (!ctx) {
            emit errorOccurred("Failed to load Whisper model. Check if the file is valid.");
            return;
        }
        
        isModelLoaded = true;
        emit modelLoaded();
        
    } catch (const std::exception &e) {
        emit errorOccurred(QString("Exception loading model: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Unknown error loading Whisper model");
    }
}

void WhisperWorker::transcribe(const std::vector<float> &audioData, const WhisperSettings &settings)
{
    if (!ctx || !isModelLoaded) {
        emit errorOccurred("Whisper model not loaded");
        return;
    }
    
    if (audioData.empty()) {
        emit errorOccurred("No audio data to transcribe");
        return;
    }
    
    try {
        struct whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
         
        wparams.print_realtime = settings.printRealtime;
        wparams.print_progress = settings.printProgress;
        wparams.print_timestamps = settings.printTimestamps;
        wparams.print_special = settings.printSpecial;
        wparams.translate = settings.translate;
        wparams.language = settings.language.toStdString().c_str();
        wparams.n_threads = settings.threads;
        wparams.offset_ms = settings.offsetMs;
        wparams.duration_ms = settings.durationMs;
        wparams.token_timestamps = settings.tokenTimestamps;
        wparams.max_len = settings.maxLen;
        wparams.split_on_word = settings.splitOnWord;
        wparams.suppress_blank = settings.suppressBlank;
         
        int result = whisper_full(ctx, wparams, audioData.data(), audioData.size());
        
        if (result != 0) {
            emit errorOccurred("Whisper transcription failed");
            return;
        }
         
        QString transcription;
        const int n_segments = whisper_full_n_segments(ctx);
        
        for (int i = 0; i < n_segments; ++i) {
            const char *text = whisper_full_get_segment_text(ctx, i);
            if (text) {
                transcription += QString::fromUtf8(text);
            }
        }
        
        transcription = transcription.trimmed();
        emit transcriptionReady(transcription);
        
    } catch (const std::exception &e) {
        emit errorOccurred(QString("Exception during transcription: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Unknown error during transcription");
    }
}