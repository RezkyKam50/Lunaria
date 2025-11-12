#ifndef LLAMAWORKER_H
#define LLAMAWORKER_H

#include <QObject>
#include <QString>
#include "llama.h"

class LlamaWorker : public QObject
{
    Q_OBJECT

public:
    LlamaWorker();
    ~LlamaWorker();

public slots:
    void loadModel(const QString &modelPath);
    void generateResponse(const QString &prompt);
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
};

#endif // LLAMAWORKER_H