#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QMessageBox>
#include <QThread>
#include <QFileDialog>
#include <QProgressBar>
#include <QGroupBox>
#include "llama.h"
 
class LlamaWorker : public QObject
{
    Q_OBJECT

public:
    LlamaWorker() : ctx(nullptr), model(nullptr), sampler(nullptr) {}
    
    ~LlamaWorker() {
        cleanup();
    }

public slots:
    void loadModel(const QString &modelPath) {
        cleanup();
         
        llama_backend_init();
         
        llama_model_params model_params = llama_model_default_params();
         
        model = llama_model_load_from_file(modelPath.toStdString().c_str(), model_params);
        
        if (!model) {
            emit errorOccurred("Can't load nigguh");
            return;
        }
         
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 2048;   
        ctx_params.n_threads = 4;   
        ctx_params.n_batch = 512;
         
        ctx = llama_init_from_model(model, ctx_params);
        
        if (!ctx) {
            emit errorOccurred("Model stoopid");
            return;
        }
         
        llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
        sampler = llama_sampler_chain_init(sampler_params);
        llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
        
        emit modelLoaded();
    }
    
    void generateResponse(const QString &prompt) {
        if (!ctx || !model) {
            emit errorOccurred("Model not loaded");
            return;
        }
        
        // Get vocab
        const llama_vocab *vocab = llama_model_get_vocab(model);
        
        // Tokenize prompt
        std::vector<llama_token> tokens;
        tokens.resize(prompt.length() + 256);
        
        int n_tokens = llama_tokenize(
            vocab,
            prompt.toStdString().c_str(),
            prompt.length(),
            tokens.data(),
            tokens.size(),
            true,   // add_bos
            false   // special
        );
        
        if (n_tokens < 0) {
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(
                vocab,
                prompt.toStdString().c_str(),
                prompt.length(),
                tokens.data(),
                tokens.size(),
                true,
                false
            );
        }
        
        tokens.resize(n_tokens);
        
        // Create batch for prompt evaluation
        llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
        
        // Evaluate prompt
        if (llama_decode(ctx, batch) != 0) {
            emit errorOccurred("Failed to evaluate prompt");
            return;
        }
        
        // Generate response
        QString response;
        int max_tokens = 512;
        int n_cur = tokens.size();
        int n_ctx = llama_n_ctx(ctx);
        
        for (int i = 0; i < max_tokens; i++) {
            // Sample next token
            llama_token new_token = llama_sampler_sample(sampler, ctx, -1);
            
            // Check for end of generation
            if (llama_vocab_is_eog(vocab, new_token)) {
                break;
            }
            
            // Convert token to text
            char buf[256];
            int n = llama_token_to_piece(vocab, new_token, buf, sizeof(buf), 0, false);
            
            if (n < 0) {
                break;
            }
            
            QString token_str = QString::fromUtf8(buf, n);
            response += token_str;
            
            // Emit partial response for streaming effect
            emit partialResponse(token_str);
            
            // Check if we're approaching context limit
            if (n_cur >= n_ctx - 1) {
                emit errorOccurred("Context limit reached");
                break;
            }
            
            // Accept the sampled token
            llama_sampler_accept(sampler, new_token);
            
            // Add token to batch and decode
            batch = llama_batch_get_one(&new_token, 1);
            
            if (llama_decode(ctx, batch) != 0) {
                emit errorOccurred("Failed to decode token");
                break;
            }
            
            n_cur++;
        }
        
        emit responseGenerated(response);
    }
    
    void cleanup() {
        if (sampler) {
            llama_sampler_free(sampler);
            sampler = nullptr;
        }
        if (ctx) {
            llama_free(ctx);
            ctx = nullptr;
        }
        if (model) {
            llama_model_free(model);
            model = nullptr;
        }
        llama_backend_free();
    }

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

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    ChatWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Lunaria - AI Chatbot");
        setMinimumSize(600, 500);
        
        auto *mainLayout = new QVBoxLayout(this);
        
        createModelSection(mainLayout);
        createChatSection(mainLayout);
        createInputSection(mainLayout);
        
        setupWorker();
    }
    
    ~ChatWindow() {
        workerThread.quit();
        workerThread.wait();
    }

private:
    void createModelSection(QVBoxLayout *mainLayout) {
        auto *modelGroup = new QGroupBox("Model Settings");
        auto *modelLayout = new QHBoxLayout();
        
        modelPathEdit = new QLineEdit();
        modelPathEdit->setPlaceholderText("Path to GGUF model file");
        modelPathEdit->setReadOnly(true);
        
        browseButton = new QPushButton("Browse");
        loadButton = new QPushButton("Load Model");
        loadButton->setEnabled(false);
        
        modelLayout->addWidget(new QLabel("Model:"));
        modelLayout->addWidget(modelPathEdit);
        modelLayout->addWidget(browseButton);
        modelLayout->addWidget(loadButton);
        
        modelGroup->setLayout(modelLayout);
        mainLayout->addWidget(modelGroup);
        
        // Progress bar
        progressBar = new QProgressBar();
        progressBar->setVisible(false);
        mainLayout->addWidget(progressBar);
    }
    
    void createChatSection(QVBoxLayout *mainLayout) {
        auto *chatGroup = new QGroupBox("Chat");
        auto *chatLayout = new QVBoxLayout();
        
        chatDisplay = new QTextEdit();
        chatDisplay->setReadOnly(true);
        chatDisplay->setMinimumHeight(300);
        
        chatLayout->addWidget(chatDisplay);
        chatGroup->setLayout(chatLayout);
        mainLayout->addWidget(chatGroup);
    }
    
    void createInputSection(QVBoxLayout *mainLayout) {
        auto *inputLayout = new QHBoxLayout();
        
        userInput = new QLineEdit();
        userInput->setPlaceholderText("Type your message...");
        userInput->setEnabled(false);
        
        sendButton = new QPushButton("Send");
        sendButton->setEnabled(false);
        
        clearButton = new QPushButton("Clear Chat");
        
        inputLayout->addWidget(userInput);
        inputLayout->addWidget(sendButton);
        inputLayout->addWidget(clearButton);
        
        mainLayout->addLayout(inputLayout);
    }
    
    void setupWorker() {
        worker = new LlamaWorker();
        worker->moveToThread(&workerThread);
        
        connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
        
        connect(browseButton, &QPushButton::clicked, this, &ChatWindow::onBrowseClicked);
        connect(loadButton, &QPushButton::clicked, this, &ChatWindow::onLoadModelClicked);
        connect(sendButton, &QPushButton::clicked, this, &ChatWindow::onSendClicked);
        connect(clearButton, &QPushButton::clicked, chatDisplay, &QTextEdit::clear);
        connect(userInput, &QLineEdit::returnPressed, this, &ChatWindow::onSendClicked);
        
        connect(this, &ChatWindow::loadModel, worker, &LlamaWorker::loadModel);
        connect(this, &ChatWindow::generateResponse, worker, &LlamaWorker::generateResponse);
        
        connect(worker, &LlamaWorker::modelLoaded, this, &ChatWindow::onModelLoaded);
        connect(worker, &LlamaWorker::responseGenerated, this, &ChatWindow::onResponseGenerated);
        connect(worker, &LlamaWorker::partialResponse, this, &ChatWindow::onPartialResponse);
        connect(worker, &LlamaWorker::errorOccurred, this, &ChatWindow::onError);
        
        workerThread.start();
    }

private slots:
    void onBrowseClicked() {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Select GGUF Model",
            QDir::homePath(),
            "GGUF Models (*.gguf);;All Files (*)"
        );
        
        if (!fileName.isEmpty()) {
            modelPathEdit->setText(fileName);
            loadButton->setEnabled(true);
        }
    }
    
    void onLoadModelClicked() {
        QString modelPath = modelPathEdit->text();
        
        if (modelPath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please select a model file");
            return;
        }
        
        progressBar->setVisible(true);
        progressBar->setRange(0, 0);  // Indeterminate progress
        loadButton->setEnabled(false);
        browseButton->setEnabled(false);
        
        chatDisplay->append("<b>Loading model...</b>");
        
        emit loadModel(modelPath);
    }
    
    void onModelLoaded() {
        progressBar->setVisible(false);
        loadButton->setEnabled(true);
        browseButton->setEnabled(true);
        userInput->setEnabled(true);
        sendButton->setEnabled(true);
        
        chatDisplay->append("<b style='color: green;'>Model loaded successfully!</b>");
        chatDisplay->append("<i>You can now start chatting.</i>\n");
    }
    
    void onSendClicked() {
        QString message = userInput->text().trimmed();
        
        if (message.isEmpty()) {
            return;
        }
        
        chatDisplay->append(QString("<b>You:</b> %1").arg(message));
        userInput->clear();
        userInput->setEnabled(false);
        sendButton->setEnabled(false);
        
        // Format prompt for chat (adjust based on your model)
        QString prompt = QString("<|user|>\n%1\n<|assistant|>\n").arg(message);
        
        chatDisplay->append("<b>AI:</b> ");
        currentResponse.clear();
        
        emit generateResponse(prompt);
    }
    
    void onPartialResponse(const QString &token) {
        currentResponse += token;
        
        // Update display with streaming effect
        QTextCursor cursor = chatDisplay->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(token);
        chatDisplay->setTextCursor(cursor);
        chatDisplay->ensureCursorVisible();
    }
    
    void onResponseGenerated(const QString &response) {
        chatDisplay->append("\n");
        userInput->setEnabled(true);
        sendButton->setEnabled(true);
        userInput->setFocus();
    }
    
    void onError(const QString &error) {
        progressBar->setVisible(false);
        loadButton->setEnabled(true);
        browseButton->setEnabled(true);
        
        chatDisplay->append(QString("<b style='color: red;'>Error:</b> %1").arg(error));
        
        // Re-enable input if model was loaded before
        if (worker) {
            userInput->setEnabled(true);
            sendButton->setEnabled(true);
        }
    }

signals:
    void loadModel(const QString &modelPath);
    void generateResponse(const QString &prompt);

private:
    QLineEdit *modelPathEdit;
    QLineEdit *userInput;
    QTextEdit *chatDisplay;
    QPushButton *browseButton;
    QPushButton *loadButton;
    QPushButton *sendButton;
    QPushButton *clearButton;
    QProgressBar *progressBar;
    
    QThread workerThread;
    LlamaWorker *worker;
    QString currentResponse;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Lunaria Chatbot");
    app.setApplicationVersion("2.0");

    ChatWindow window;
    window.show();

    return app.exec();
}

#include "lunaria.moc"