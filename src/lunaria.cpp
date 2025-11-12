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
#include "llamaworker.h"

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    ChatWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Lunaria");
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

        auto *modelGroup    = new QGroupBox("Model Settings");
        auto *modelLayout   = new QHBoxLayout();
        
        modelPathEdit       = new QLineEdit();
        modelPathEdit   ->setPlaceholderText("Path to GGUF model file");
        modelPathEdit   ->setReadOnly(true);
        
        browseButton        = new QPushButton("Browse");
        loadButton          = new QPushButton("Load Model");

        loadButton      ->setEnabled(false);
        
        modelLayout     ->addWidget(new QLabel("Model:"));
        modelLayout     ->addWidget(modelPathEdit);
        modelLayout     ->addWidget(browseButton);
        modelLayout     ->addWidget(loadButton);
        
        modelGroup      ->setLayout(modelLayout);
        mainLayout      ->addWidget(modelGroup);
         
        progressBar         = new QProgressBar();
        progressBar     ->setVisible(false);
        mainLayout      ->addWidget(progressBar);

    }
    
    void createChatSection(QVBoxLayout *mainLayout) {

        auto *chatGroup     = new QGroupBox("Chat");
        auto *chatLayout    = new QVBoxLayout();
        
        chatDisplay         = new QTextEdit();
        chatDisplay     ->setReadOnly(true);
        chatDisplay     ->setMinimumHeight(300);
        
        chatLayout      ->addWidget(chatDisplay);
        chatGroup       ->setLayout(chatLayout);
        mainLayout      ->addWidget(chatGroup);

    }
    
    void createInputSection(QVBoxLayout *mainLayout) {

        auto *inputLayout   = new QHBoxLayout();
        
        userInput           = new QLineEdit();
        userInput       ->setPlaceholderText("Type your message...");
        userInput       ->setEnabled(false);
        
        sendButton          = new QPushButton("Send");
        sendButton      ->setEnabled(false);
        
        clearButton         = new QPushButton("Clear Chat");
        
        inputLayout     ->addWidget(userInput);
        inputLayout     ->addWidget(sendButton);
        inputLayout     ->addWidget(clearButton);
        
        mainLayout      ->addLayout(inputLayout);

    }
    
    void setupWorker() {

        worker = new LlamaWorker();
        worker->moveToThread(&workerThread);
        
        connect(&workerThread,  &QThread::finished, worker, &QObject::deleteLater);
        
        connect(browseButton,   &QPushButton::clicked,          this, &ChatWindow::onBrowseClicked);
        connect(loadButton,     &QPushButton::clicked,          this, &ChatWindow::onLoadModelClicked);
        connect(sendButton,     &QPushButton::clicked,          this, &ChatWindow::onSendClicked);
        connect(clearButton,    &QPushButton::clicked,          chatDisplay, &QTextEdit::clear);
        connect(userInput,      &QLineEdit::returnPressed,      this, &ChatWindow::onSendClicked);
        
        connect(this,           &ChatWindow::loadModel,         worker, &LlamaWorker::loadModel);
        connect(this,           &ChatWindow::generateResponse,  worker, &LlamaWorker::generateResponse);
        
        connect(worker,         &LlamaWorker::modelLoaded,      this, &ChatWindow::onModelLoaded);
        connect(worker,         &LlamaWorker::responseGenerated,this, &ChatWindow::onResponseGenerated);
        connect(worker,         &LlamaWorker::partialResponse,  this, &ChatWindow::onPartialResponse);
        connect(worker,         &LlamaWorker::errorOccurred,    this, &ChatWindow::onError);
        
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
        
        progressBar     ->setVisible(true);
        progressBar     ->setRange(0, 0);   
        loadButton      ->setEnabled(false);
        browseButton    ->setEnabled(false);
        
        chatDisplay->append("<b>Loading model...</b>");
        
        emit loadModel(modelPath);

    }
    
    void onModelLoaded() {

        progressBar     ->setVisible(false);
        loadButton      ->setEnabled(true);
        browseButton    ->setEnabled(true);
        userInput       ->setEnabled(true);
        sendButton      ->setEnabled(true);
        
        chatDisplay     ->append("<b style='color: green;'>Model loaded successfully!</b>");
        chatDisplay     ->append("<i>You can now start chatting.</i>\n");

    }
    
    void onSendClicked() {

        QString message = userInput->text().trimmed();
        
        if (message.isEmpty()) {
            return;
        }
        
        chatDisplay     ->append(QString("<b>You:</b> %1").arg(message));

        userInput       ->clear();
        userInput       ->setEnabled(false);
        sendButton      ->setEnabled(false);
        
        QString prompt = QString("<|user|>\n%1\n<|assistant|>\n").arg(message);
        
        chatDisplay     ->append("<b>LLM:</b> ");
        currentResponse.clear();
        
        emit generateResponse(prompt);

    }
    
    void onPartialResponse(const QString &token) {

        currentResponse += token;
         
        QTextCursor cursor = chatDisplay->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(token);
        chatDisplay     ->setTextCursor(cursor);
        chatDisplay     ->ensureCursorVisible();

    }
    
    void onResponseGenerated(const QString &response) {

        chatDisplay     ->append("\n");
        userInput       ->setEnabled(true);
        sendButton      ->setEnabled(true);
        userInput       ->setFocus();
    }
    
    void onError(const QString &error) {

        progressBar     ->setVisible(false);
        loadButton      ->setEnabled(true);
        browseButton    ->setEnabled(true);
        
        chatDisplay->append(QString("<b style='color: red;'>Error:</b> %1").arg(error));
         
        if (worker) {
            userInput   ->setEnabled(true);
            sendButton  ->setEnabled(true);
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
    
    app.setApplicationName("Lunaria");
    app.setApplicationVersion("1.0");

    ChatWindow window;
    window.show();

    return app.exec();
}

#include "lunaria.moc"