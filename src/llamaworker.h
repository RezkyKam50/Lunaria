#ifndef LLAMAWORKER_H
#define LLAMAWORKER_H

#include <QObject>
#include <QString>
#include "llama.h"

struct GenerationSettings {
    int maxTokens       = 512;
    double temperature  = 0.7;
    double topP         = 0.9;
    int topK            = 40;
};

struct ContextSettings {
    int contextSize     = 2048;
    int threadCount     = 8;
    int batchSize       = 512;
};  

class LlamaWorker : public QObject
{
    Q_OBJECT

public:
    LlamaWorker();
    ~LlamaWorker();

public slots:
    void loadModel(const QString &modelPath, const ContextSettings &settings);
    void generateResponse(const QString &prompt, const GenerationSettings &settings);
    void cleanup();

signals:
    void modelLoaded();
    void responseGenerated(const QString &response);
    void partialResponse(const QString &token);
    void errorOccurred(const QString &error);

private:
    llama_context *ctx;
    llama_model *model;
    llama_sampler *sampler;
    
    void updateSampler(const GenerationSettings &settings);
};

#endif // LLAMAWORKER_H