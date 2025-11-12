#ifndef WHISPERWORKER_H
#define WHISPERWORKER_H

#include <QObject>
#include <QString>
#include <vector>
#include "whisper.h"

class WhisperWorker : public QObject
{
    Q_OBJECT

public:
    explicit WhisperWorker(QObject *parent = nullptr);
    ~WhisperWorker();

public slots:
    void loadModel(const QString &modelPath);
    void transcribe(const std::vector<float> &audioData);

signals:
    void modelLoaded();
    void transcriptionReady(const QString &text);
    void errorOccurred(const QString &error);

private:
    whisper_context *ctx = nullptr;
    bool isModelLoaded = false;
};

#endif // WHISPERWORKER_H