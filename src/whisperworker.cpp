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

void WhisperWorker::transcribe(const std::vector<float> &audioData)
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
         
        wparams.print_realtime   = false;
        wparams.print_progress   = false;
        wparams.print_timestamps = false;
        wparams.print_special    = false;
        wparams.translate        = false;
        wparams.language         = "en"; // Change to "auto" for auto-detection
        wparams.n_threads        = 4;
        wparams.offset_ms        = 0;
        wparams.duration_ms      = 0;
        wparams.token_timestamps = false;
        wparams.max_len          = 1;
        wparams.split_on_word    = true;
        wparams.suppress_blank   = true;
         
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