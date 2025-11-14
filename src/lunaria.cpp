/**
 * @file 
 * @brief Lunaria-cpp
 * @author rezkykam
 *
 * ----------------------------------------------------------------------------
 * LICENSE  
 * ----------------------------------------------------------------------------
 * Lunaria is a derivative work based on several open-source components.
 * The licensing of these components dictates the license of the final work.
 *
 * DEPENDENCIES AND THEIR LICENSES:
 *   - Qt6           (LGPLv3/GPLv2+)      - https://www.qt.io/
 *   - llama.cpp     (MIT License)        - https://github.com/ggerganov/llama.cpp
 *   - whisper.cpp   (MIT License)        - https://github.com/ggerganov/whisper.cpp
 *   - poppler       (GPLv3+ License)     - https://poppler.freedesktop.org/
 *
 * The GNU General Public License Version 3 (GPLv3) is a copyleft license that
 * requires derivative works to be licensed under the same terms. Because Lunaria
 * statically links against poppler (GPLv3), the entire work is considered a
 * derivative and must be distributed under the GPLv3.
 *
 * Qt6 is available under multiple licenses including LGPLv3 and GPLv2+, which
 * are compatible with GPLv3. When using Qt6 with Lunaria, you must comply with
 * the terms of the GPLv3 for the entire application.
 *
 * Therefore, Lunaria is licensed under the GNU GPLv3.
 * See the 'LICENSE' file in this distribution for the full terms and conditions.
 * ----------------------------------------------------------------------------
 */

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
#include <QSplitter>
#include <QMenuBar>
#include <QMainWindow>
#include <QSettings>
#include <QElapsedTimer>

#include <QAudioSource>
#include <QAudioFormat>
#include <QBuffer>
#include <QMediaDevices>

#include "llamaworker.h"
#include "whisperworker.h"
#include "settingsdialog.h"
 
#include <poppler-document.h>
#include <poppler-page.h>


    // Interthread comms. were managed with QThread signal and slotting,
    // for ease of development, avoid using legacy MT ops. since it can interrupt with main UI leading to hard-to trace memory leaks.

    // saved config can be found under .config/Lunaria/Lunaria.conf
    // resetting to default simply by deleting Lunaria.conf
    // we'll add a feature for that later for QOL.

class ChatWindow : public QMainWindow
{
    Q_OBJECT

private:
    struct Constants {
        static constexpr int DEFAULT_WINDOW_WIDTH   = 1200;
        static constexpr int DEFAULT_WINDOW_HEIGHT  = 600;

        static constexpr int LEFT_PANEL_MAX_WIDTH   = 350;
        static constexpr int LEFT_PANEL_MIN_WIDTH   = 300;

        static constexpr int SAMPLE_RATE            = 16000;
        static constexpr int AUDIO_CHANNELS         = 1;
    };
    
    struct Defaults {
        static inline const QString SYSTEM_PROMPT = "You are a helpful AI assistant.";

        static inline QString FEWSHOT_PROMPT() {     
        return                                      "system\n" + SYSTEM_PROMPT + "\n"
                                                    "user\nWhat is the capital of France?\n"
                                                    "assistant\nThe capital of France is Paris.\n"
                                                    "user\nHow many planets are in our solar system?\n"
                                                    "assistant\nThere are 8 planets in our solar system.\n";
        }

        static inline const GenerationSettings GENERATION   = {
                                                                /*maxTokens=*/      512,
                                                                /*temperature=*/    0.3,
                                                                /*topP=*/           0.95,
                                                                /*topK=*/           10
        };
        static inline const ContextSettings CONTEXT         = {
                                                                /*contextSize=*/    2048,
                                                                /*threadCount=*/    8,
                                                                /*batchSize=*/      512
        };
        static constexpr int PDF_TRUNCATION_LENGTH          = 500;  
    };
    
    struct Styles {
 
        static inline const QString COLOR_GRAY          = "gray";
        static inline const QString COLOR_ORANGE        = "orange";
        static inline const QString COLOR_GREEN         = "green";
        static inline const QString COLOR_RED           = "red";
        static inline const QString COLOR_CYAN          = "cyan";
        static inline const QString COLOR_WHITE         = "white";
         
        static inline const QString STATUS_DEFAULT      = "color: gray;";
        static inline const QString STATUS_LOADING      = "color: orange;";
        static inline const QString STATUS_READY        = "color: #90EE90;";  
        static inline const QString STATUS_ERROR        = "color: red;";
        static inline const QString STATUS_RECORDING    = "color: red;";
         
        static inline const QString BUTTON_RECORDING    = "background-color: #ff4444; color: white;";
        static inline const QString BUTTON_NORMAL       = "";
         
        static inline const QString CHAT_SYSTEM         = "color: gray;";
        static inline const QString CHAT_SUCCESS        = "color: green;";
        static inline const QString CHAT_ERROR          = "color: red;";
        static inline const QString CHAT_INFO           = "color: cyan;";
        static inline const QString CHAT_ITALIC         = "font-style: italic;";
         
        static inline const QString HTML_SYSTEM         = "<i style='color: gray;'>%1</i>";
        static inline const QString HTML_SUCCESS        = "<i style='color: green;'>%1</i>";
        static inline const QString HTML_ERROR          = "<b style='color: red;'>Error:</b> %1";
        static inline const QString HTML_INFO           = "<i style='color: cyan;'>%1</i>";
        static inline const QString HTML_LOADING        = "<i>%1</i>";
        static inline const QString HTML_USER           = "<b>You:</b> %1";
        static inline const QString HTML_LLM            = "<b>Assistant:</b> ";
        static inline const QString HTML_TRANSCRIBED    = "<i>Transcribed: %1</i>";
        static inline const QString HTML_PDF_LOADED     = "<i style='color: cyan;'>PDF loaded: %1 (%2 pages)</i>";
 
    };
    
public:
    ChatWindow(QWidget *parent = nullptr) : QMainWindow(parent)

    // Default settings else modified settings.

        , systemPrompt          (Defaults::SYSTEM_PROMPT)
        , generationSettings    (Defaults::GENERATION)
        , contextSettings       (Defaults::CONTEXT)
        , pdfTruncationLength   (Defaults::PDF_TRUNCATION_LENGTH) 
        , modelPathEdit         (nullptr)
        , userInput             (nullptr)
        , chatDisplay           (nullptr)
        , browseButton          (nullptr)
        , loadButton            (nullptr)
        , sendButton            (nullptr)
        , uploadButton          (nullptr)
        , clearButton           (nullptr)
        , progressBar           (nullptr)
        , llmStatusLabel        (nullptr)
        , whisperPathEdit       (nullptr)
        , whisperBrowseButton   (nullptr)
        , whisperLoadButton     (nullptr)
        , recordButton          (nullptr)
        , whisperStatusLabel    (nullptr)
        , whisperProgressBar    (nullptr)
        , worker                (nullptr)
        , whisperWorker         (nullptr)
        , audioInput            (nullptr)
        , audioBuffer           (nullptr)
        , isRecording           (false)
        , savedModelPath        ("")
        , savedWhisperPath      ("")
        , conversationHistory   ("")
        , currentResponse       ("")

        
    {
        setWindowTitle("Lunaria");
        setMinimumSize(Constants::DEFAULT_WINDOW_WIDTH, Constants::DEFAULT_WINDOW_HEIGHT);
         
        loadSavedSettings();
        createMenuBar();
         
        QWidget *centralWidget = new QWidget();
        setCentralWidget(centralWidget);
         
        auto *mainLayout = new QHBoxLayout(centralWidget);
        
        QSplitter *splitter = new QSplitter(Qt::Horizontal);
         
        QWidget *leftPanel = new QWidget();
        auto *leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        
        createModelSection(leftLayout);
        createWhisperSection(leftLayout);
        leftLayout->addStretch();  
        
        leftPanel->setMaximumWidth(Constants::LEFT_PANEL_MAX_WIDTH);
        leftPanel->setMinimumWidth(Constants::LEFT_PANEL_MIN_WIDTH);
         
        QWidget *rightPanel = new QWidget();
        auto *rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        
        createChatSection(rightLayout);
        createInputSection(rightLayout);
        
        splitter->addWidget(leftPanel);
        splitter->addWidget(rightPanel);
        splitter->setStretchFactor(0, 0); 
        splitter->setStretchFactor(1, 1); 
        
        mainLayout->addWidget(splitter);
        
        setupWorker();
        setupWhisperWorker();

        if (!savedModelPath.isEmpty()) {
            modelPathEdit->setText(savedModelPath);
            loadButton->setEnabled(true);
        }
        
        if (!savedWhisperPath.isEmpty()) {
            whisperPathEdit->setText(savedWhisperPath);
            whisperLoadButton->setEnabled(true);
        }
    }
    
    ~ChatWindow() {
        saveSettings();  

        workerThread.quit();
        workerThread.wait();
        whisperThread.quit();
        whisperThread.wait();
        
        if (audioInput) {
            audioInput->stop();
            delete audioInput;
        }
    }

private:
    void createMenuBar() {
        QMenuBar *menuBar = new QMenuBar();
        setMenuBar(menuBar);
        
        QMenu *fileMenu = menuBar->addMenu("&File");
        
        QAction *settingsAction = new QAction("&Generation Settings", this);
        settingsAction->setShortcut(QKeySequence("Ctrl+,"));
        connect(settingsAction, &QAction::triggered, this, &ChatWindow::onSettingsClicked);
        fileMenu->addAction(settingsAction);
        
        fileMenu->addSeparator();
        
        QAction *exitAction = new QAction("E&xit", this);
        exitAction->setShortcut(QKeySequence("Ctrl+Q"));
        connect(exitAction, &QAction::triggered, this, &QWidget::close);
        fileMenu->addAction(exitAction);

        QMenu *helpMenu = menuBar->addMenu("&Help");
        
        QAction *aboutAction = new QAction("&About", this);
        connect(aboutAction, &QAction::triggered, this, &ChatWindow::onAboutClicked);
        helpMenu->addAction(aboutAction);
    }

    void loadSavedSettings() {
        QSettings settings("Lunaria", "Lunaria");
        
        savedModelPath                  = settings.value("model/lastPath", "").toString();
        savedWhisperPath                = settings.value("whisper/lastPath", "").toString();
        
        systemPrompt                    = settings.value("generation/systemPrompt",     Defaults::SYSTEM_PROMPT).toString();
        fewShotExamples                 = settings.value("generation/fewShotExamples",  Defaults::FEWSHOT_PROMPT()).toString();

        generationSettings.maxTokens    = settings.value("generation/maxTokens",        generationSettings.maxTokens).toInt();
        generationSettings.temperature  = settings.value("generation/temperature",      generationSettings.temperature).toDouble();
        generationSettings.topP         = settings.value("generation/topP",             generationSettings.topP).toDouble();
        generationSettings.topK         = settings.value("generation/topK",             generationSettings.topK).toInt();

        contextSettings.contextSize     = settings.value("context/size",                contextSettings.contextSize).toInt();
        contextSettings.threadCount     = settings.value("context/threads",             contextSettings.threadCount).toInt();
        contextSettings.batchSize       = settings.value("context/batchSize",           contextSettings.batchSize).toInt();
                
        pdfTruncationLength             = settings.value("generation/pdfTruncation",    pdfTruncationLength).toInt();

        // TODO: declare constants
        whisperSettings.printRealtime   = settings.value("whisper/printRealtime", false).toBool();
        whisperSettings.printProgress   = settings.value("whisper/printProgress", false).toBool();
        whisperSettings.printTimestamps = settings.value("whisper/printTimestamps", false).toBool();
        whisperSettings.printSpecial    = settings.value("whisper/printSpecial", false).toBool();
        whisperSettings.translate       = settings.value("whisper/translate", false).toBool();
        whisperSettings.language        = settings.value("whisper/language", "en").toString();
        whisperSettings.threads         = settings.value("whisper/threads", 4).toInt();
        whisperSettings.offsetMs        = settings.value("whisper/offsetMs", 0).toInt();
        whisperSettings.durationMs      = settings.value("whisper/durationMs", 0).toInt();
        whisperSettings.tokenTimestamps = settings.value("whisper/tokenTimestamps", false).toBool();
        whisperSettings.maxLen          = settings.value("whisper/maxLen", 1).toInt();
        whisperSettings.splitOnWord     = settings.value("whisper/splitOnWord", true).toBool();
        whisperSettings.suppressBlank   = settings.value("whisper/suppressBlank", true).toBool();

    }
    
    void saveSettings() {
        QSettings settings              ("Lunaria", "Lunaria");
        
        settings.setValue               ("model/lastPath",              modelPathEdit->text());
        settings.setValue               ("whisper/lastPath",            whisperPathEdit->text());
        
        settings.setValue               ("generation/systemPrompt",     systemPrompt);
        settings.setValue               ("generation/fewShotExamples",  fewShotExamples);
        settings.setValue               ("generation/maxTokens",        generationSettings.maxTokens);
        settings.setValue               ("generation/temperature",      generationSettings.temperature);
        settings.setValue               ("generation/topP",             generationSettings.topP);
        settings.setValue               ("generation/topK",             generationSettings.topK);
        
        settings.setValue               ("context/size",                contextSettings.contextSize);
        settings.setValue               ("context/threads",             contextSettings.threadCount);
        settings.setValue               ("context/batchSize",           contextSettings.batchSize);
        
        settings.setValue               ("generation/pdfTruncation",    pdfTruncationLength);  

        settings.setValue               ("whisper/printRealtime",       whisperSettings.printRealtime);
        settings.setValue               ("whisper/printProgress",       whisperSettings.printProgress);
        settings.setValue               ("whisper/printTimestamps",     whisperSettings.printTimestamps);
        settings.setValue               ("whisper/printSpecial",        whisperSettings.printSpecial);
        settings.setValue               ("whisper/translate",           whisperSettings.translate);
        settings.setValue               ("whisper/language",            whisperSettings.language);
        settings.setValue               ("whisper/threads",             whisperSettings.threads);
        settings.setValue               ("whisper/offsetMs",            whisperSettings.offsetMs);
        settings.setValue               ("whisper/durationMs",          whisperSettings.durationMs);
        settings.setValue               ("whisper/tokenTimestamps",     whisperSettings.tokenTimestamps);
        settings.setValue               ("whisper/maxLen",              whisperSettings.maxLen);
        settings.setValue               ("whisper/splitOnWord",         whisperSettings.splitOnWord);
        settings.setValue               ("whisper/suppressBlank",       whisperSettings.suppressBlank);

    }


    void createModelSection(QVBoxLayout *layout) {
        auto *modelGroup    = new QGroupBox ("LLM Settings");
        auto *modelLayout   = new QVBoxLayout();
         
        modelPathEdit       = new QLineEdit();
        modelPathEdit->setPlaceholderText("Model path...");
        modelPathEdit->setReadOnly(true);
        
        browseButton        = new QPushButton("Browse");
        loadButton          = new QPushButton("Load Model");
        loadButton->setEnabled(false);
        
        auto *btnLayout     = new QHBoxLayout();
        btnLayout->addWidget(browseButton);
        btnLayout->addWidget(loadButton);
         
        llmStatusLabel      = new QLabel("Status: Not Loaded");
        llmStatusLabel->setStyleSheet(Styles::STATUS_DEFAULT);
        
        modelLayout->addWidget(new QLabel("Model:"));
        modelLayout->addWidget(modelPathEdit);
        modelLayout->addLayout(btnLayout);
        modelLayout->addWidget(llmStatusLabel);
        
        progressBar         = new QProgressBar();
        progressBar->setVisible(false);
        modelLayout->addWidget(progressBar);
        
        modelGroup->setLayout(modelLayout);
        layout->addWidget(modelGroup);
    }
    
    void createWhisperSection(QVBoxLayout *layout) {
        auto *whisperGroup  = new QGroupBox("Whisper Settings");
        auto *whisperLayout = new QVBoxLayout();
 
        whisperPathEdit     = new QLineEdit();
        whisperPathEdit->setPlaceholderText("Whisper model path...");
        whisperPathEdit->setReadOnly(true);

        whisperBrowseButton = new QPushButton("Browse");
        whisperLoadButton   = new QPushButton("Load Whisper");
        whisperLoadButton->setEnabled(false);

        auto *btnLayout     = new QHBoxLayout();
        btnLayout->addWidget(whisperBrowseButton);
        btnLayout->addWidget(whisperLoadButton);
 
        recordButton        = new QPushButton("Record Audio");
        recordButton->setEnabled(false);
 
        whisperStatusLabel  = new QLabel("Status: Not Loaded");
        whisperStatusLabel->setStyleSheet(Styles::STATUS_DEFAULT);
        
        whisperLayout->addWidget(new QLabel("Model:"));
        whisperLayout->addWidget(whisperPathEdit);
        whisperLayout->addLayout(btnLayout);
        whisperLayout->addWidget(recordButton);
        whisperLayout->addWidget(whisperStatusLabel);

        whisperProgressBar  = new QProgressBar();
        whisperProgressBar->setVisible(false);
        whisperLayout->addWidget(whisperProgressBar);

        whisperGroup->setLayout(whisperLayout);
        layout->addWidget(whisperGroup);
    }
    
    void createChatSection(QVBoxLayout *layout) {
        auto *chatGroup     = new QGroupBox("Chat");
        auto *chatLayout    = new QVBoxLayout();
        
        chatDisplay         = new QTextEdit();
        chatDisplay->setReadOnly(true);
        
        chatLayout->addWidget(chatDisplay);
        chatGroup->setLayout(chatLayout);
        layout->addWidget(chatGroup);
    }
    
    void createInputSection(QVBoxLayout *layout) {
        auto *inputLayout   = new QHBoxLayout();
        
        userInput           = new QLineEdit();
        userInput->setPlaceholderText("Type your message or use voice input...");
        userInput->setEnabled(false);
        
        uploadButton        = new QPushButton("Upload PDF");
        uploadButton->setEnabled(false);
        
        sendButton          = new QPushButton("Send");
        sendButton->setEnabled(false);
        
        clearButton         = new QPushButton("Clear Chat");
        
        inputLayout->addWidget(userInput);
        inputLayout->addWidget(uploadButton);
        inputLayout->addWidget(sendButton);
        inputLayout->addWidget(clearButton);
        
        layout->addLayout(inputLayout);
    }
    
    void setStatus(QLabel *statusLabel, const QString &text, const QString &style) {
        statusLabel->setText(QString("Status: %1").arg(text));
        statusLabel->setStyleSheet(style);
    }
    
    void setStatusWithColor(QLabel *statusLabel, const QString &text, const QString &color) {
        statusLabel->setText(QString("Status: %1").arg(text));
        statusLabel->setStyleSheet(QString("color: %1;").arg(color));
    }
     
    void setProgressBarVisible(QProgressBar *progressBar, bool visible, bool indeterminate = true) {
        if (visible && indeterminate) {
            progressBar->setRange(0, 0);   
        }
        progressBar->setVisible(visible);
    }
     
    void setModelControlsEnabled(QPushButton *browseBtn, QPushButton *loadBtn, bool enabled) {
        browseBtn->setEnabled(enabled);
        loadBtn->setEnabled(enabled);
    }
    
    QString extractTextFromPDF(const QString &filePath, int maxChars = -1) {
        if (maxChars == -1) {
            maxChars = pdfTruncationLength;  
        }
        
        std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(filePath.toStdString()));
        
        if (!doc) {
            return QString();
        }
        
        QString extractedText;
        int pageCount = doc->pages();
        
        for (int i = 0; i < pageCount; ++i) {
            if (extractedText.length() >= maxChars) {
                break;  
            }
            
            std::unique_ptr<poppler::page> page(doc->create_page(i));
            if (page) {
                poppler::byte_array textBytes = page->text().to_utf8();
                QString pageText = QString::fromUtf8(textBytes.data(), textBytes.size());
                extractedText += pageText + "\n\n";
            }
        }
        
        if (extractedText.length() > maxChars) {
            extractedText = extractedText.left(maxChars) + "\n...[truncated]";
        }
        
        return extractedText.trimmed();
    }
    
    void setupWorker() {
        worker = new LlamaWorker();
        worker->moveToThread(&workerThread);
        
        connect(&workerThread,  &QThread::finished, worker, &QObject::deleteLater);
        
        connect(browseButton,   &QPushButton::clicked,          this, &ChatWindow::onBrowseClicked);
        connect(loadButton,     &QPushButton::clicked,          this, &ChatWindow::onLoadModelClicked);
        connect(sendButton,     &QPushButton::clicked,          this, &ChatWindow::onSendClicked);
        connect(uploadButton,   &QPushButton::clicked,          this, &ChatWindow::onUploadPDFClicked);
        connect(clearButton,    &QPushButton::clicked,          this, &ChatWindow::onClearChatClicked);
        connect(userInput,      &QLineEdit::returnPressed,      this, &ChatWindow::onSendClicked);
        
        connect(this,           &ChatWindow::loadModel,         worker, &LlamaWorker::loadModel);
        connect(this,           &ChatWindow::generateResponse,  worker, &LlamaWorker::generateResponse);
        
        connect(worker,         &LlamaWorker::modelLoaded,      this, &ChatWindow::onModelLoaded);
        connect(worker,         &LlamaWorker::responseGenerated,this, &ChatWindow::onResponseGenerated);
        connect(worker,         &LlamaWorker::partialResponse,  this, &ChatWindow::onPartialResponse);
        connect(worker,         &LlamaWorker::errorOccurred,    this, &ChatWindow::onError);
        
        workerThread.start();
    }
    
    void setupWhisperWorker() {
        whisperWorker = new WhisperWorker();
        whisperWorker->moveToThread(&whisperThread);

        connect(&whisperThread,     &QThread::finished, whisperWorker, &QObject::deleteLater);
        
        connect(whisperBrowseButton, &QPushButton::clicked,             this, &ChatWindow::onWhisperBrowseClicked);
        connect(whisperLoadButton,   &QPushButton::clicked,             this, &ChatWindow::onWhisperLoadClicked);
        connect(recordButton,        &QPushButton::clicked,             this, &ChatWindow::onRecordClicked);
        
        connect(this,               &ChatWindow::loadWhisperModel,      whisperWorker, &WhisperWorker::loadModel);
        connect(this,               &ChatWindow::transcribeAudio,       whisperWorker, &WhisperWorker::transcribe);
        
        connect(whisperWorker,      &WhisperWorker::modelLoaded,        this, &ChatWindow::onWhisperModelLoaded);
        connect(whisperWorker,      &WhisperWorker::transcriptionReady, this, &ChatWindow::onTranscriptionReady);
        connect(whisperWorker,      &WhisperWorker::errorOccurred,      this, &ChatWindow::onWhisperError);
        
        whisperThread.start();
    }
    
    void setupAudioInput() {
        QAudioFormat format;
        format.setSampleRate(Constants::SAMPLE_RATE);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Float);
        
        audioInput = new QAudioSource(QMediaDevices::defaultAudioInput(), format, this);
        audioBuffer = new QBuffer(&audioData, this);
    }

private slots:
    void onBrowseClicked() {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Select GGUF Model",
            savedModelPath.isEmpty() ? QDir::homePath() : QFileInfo(savedModelPath).absolutePath(),
            "GGUF Models (*.gguf);;All Files (*)"
        );
        if (!fileName.isEmpty()) {
            modelPathEdit->setText(fileName);
            loadButton->setEnabled(true);
            savedModelPath = fileName;
            saveSettings();  
        }
    }
    
    void onWhisperBrowseClicked() {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Select Whisper Model",
            savedWhisperPath.isEmpty() ? QDir::homePath() : QFileInfo(savedWhisperPath).absolutePath(),
            "Whisper Models (*.bin);;All Files (*)"
        );
        if (!fileName.isEmpty()) {
            whisperPathEdit->setText(fileName);
            whisperLoadButton->setEnabled(true);
            savedWhisperPath = fileName;
            saveSettings();  
        }
    }
    
    void onLoadModelClicked() {
        QString modelPath = modelPathEdit->text();
        
        if (modelPath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please select a model file");
            return;
        }
         
        modelLoadTimer.start();
        
        setProgressBarVisible(progressBar, true);
        setModelControlsEnabled(browseButton, loadButton, false);
        setStatus(llmStatusLabel, "Loading...", Styles::STATUS_LOADING);
        
        chatDisplay->append(Styles::HTML_LOADING.arg("Loading model..."));
        
        emit loadModel(modelPath, contextSettings);
    }

    void onWhisperLoadClicked() {
        QString modelPath = whisperPathEdit->text();
        
        if (modelPath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please select a Whisper model file");
            return;
        }
         
        whisperLoadTimer.start();
        
        setProgressBarVisible(whisperProgressBar, true);
        setModelControlsEnabled(whisperBrowseButton, whisperLoadButton, false);
        setStatus(whisperStatusLabel, "Loading...", Styles::STATUS_LOADING);

        chatDisplay->append(Styles::HTML_LOADING.arg("Loading model..."));
        
        emit loadWhisperModel(modelPath);
    }
        
    void onModelLoaded() {
        qint64 loadTimeMs = modelLoadTimer.elapsed();
        
        setProgressBarVisible(progressBar, false);
        setModelControlsEnabled(browseButton, loadButton, true);
        setStatus(llmStatusLabel, "Ready", Styles::STATUS_READY);
        
        userInput->setEnabled(true);
        sendButton->setEnabled(true);
        uploadButton->setEnabled(true);
        
        chatDisplay->append(Styles::HTML_SUCCESS.arg("Success."));
        chatDisplay->append(Styles::HTML_SYSTEM.arg(QString("Model loaded in %1 ms").arg(loadTimeMs)));
        chatDisplay->append(Styles::HTML_LOADING.arg("You can now start chatting.") + "\n");
        
        conversationHistory.clear();
    }

    void onWhisperModelLoaded() {
        qint64 loadTimeMs = whisperLoadTimer.elapsed();
        
        setProgressBarVisible(whisperProgressBar, false);
        setModelControlsEnabled(whisperBrowseButton, whisperLoadButton, true);
        setStatus(whisperStatusLabel, "Ready", Styles::STATUS_READY);
        
        recordButton->setEnabled(true);

        chatDisplay->append(Styles::HTML_SUCCESS.arg("Success."));
 
        chatDisplay->append(Styles::HTML_SYSTEM.arg(QString("Whisper model loaded in %1 ms").arg(loadTimeMs)));
        
        chatDisplay->append(Styles::HTML_LOADING.arg("You can now start talking by pressing 'Record Audio'.") + "\n");

        setupAudioInput();
    }
    
    void onRecordClicked() {
        if (!isRecording) {
            audioData.clear();
            audioBuffer->open(QIODevice::WriteOnly);
            audioInput->start(audioBuffer);
            
            recordButton->setText("Stop Recording");
            recordButton->setStyleSheet(Styles::BUTTON_RECORDING);
            setStatus(whisperStatusLabel, "Recording...", Styles::STATUS_RECORDING);
            isRecording = true;
        } else {
 
            audioInput->stop();
            audioBuffer->close();
            
            recordButton->setText("Record Audio");
            recordButton->setStyleSheet(Styles::BUTTON_NORMAL);
            setStatus(whisperStatusLabel, "Transcribing...", Styles::STATUS_LOADING);
            isRecording = false;
             
            QByteArray data = audioData;
            std::vector<float> audioFloats;
            audioFloats.resize(data.size() / sizeof(float));
            memcpy(audioFloats.data(), data.data(), data.size());
            
            emit transcribeAudio(audioFloats, whisperSettings);
        }
    }
    
    void onUploadPDFClicked() {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Select PDF Document",
            QDir::homePath(),
            "PDF Files (*.pdf);;All Files (*)"
        );
        
        if (fileName.isEmpty()) {
            return;
        }
        
        chatDisplay->append(Styles::HTML_LOADING.arg("Extracting text from PDF..."));
        
        QString extractedText = extractTextFromPDF(fileName);
        
        if (extractedText.isEmpty()) {
            chatDisplay->append(Styles::HTML_ERROR.arg("Failed to extract text from PDF or PDF is empty."));
            return;
        }
        
        std::unique_ptr<poppler::document> doc(
            poppler::document::load_from_file(fileName.toStdString())
        );
        
        if (!doc) {
            chatDisplay->append(Styles::HTML_ERROR.arg("Failed to load PDF document."));
            return;
        }
        
        int pageCount = doc->pages();   
        
        QFileInfo fileInfo(fileName);
          
        QString pdfContext = QString("\n\n[PDF Content from %1]:\n%2\n").arg(fileInfo.fileName()).arg(extractedText);
         
        conversationHistory += QString("<|user|>\n%1<|end|>\n"
                                    "<|assistant|>\nI've loaded the PDF document '%2'. How can I help you with this content?<|end|>\n")
                                .arg(pdfContext)
                                .arg(fileInfo.fileName());
         
        
        chatDisplay->append(Styles::HTML_PDF_LOADED.arg(fileInfo.fileName()).arg(pageCount));
        chatDisplay->append(Styles::HTML_INFO.arg(
            QString("Extracted %1 characters and appended to input.")
            .arg(extractedText.length())
        ));
         
        chatDisplay->append(Styles::HTML_SYSTEM.arg(
            QString("PDF '%1' has been loaded into the conversation context. The LLM can now reference this content.")
            .arg(fileInfo.fileName())
        ));
    }
    
    void onSendClicked() {
        QString message = userInput->text().trimmed();
        
        if (message.isEmpty()) {
            return;
        }
        
        lastUserMessage = message;
        
        chatDisplay->append(Styles::HTML_USER.arg(message));
        userInput->clear();
        userInput->setEnabled(false);
        sendButton->setEnabled(false);
        uploadButton->setEnabled(false);
        setStatus(llmStatusLabel, "Generating...", Styles::STATUS_LOADING);
        
        QString fullPrompt;
        
        if (!systemPrompt.isEmpty()) {
            fullPrompt += QString("<|system|>\n%1<|end|>\n").arg(systemPrompt);
        }
        
        if (!fewShotExamples.isEmpty()) {
            QString updatedExamples = fewShotExamples;
            updatedExamples.replace("<|im_start|>", "<|");
            updatedExamples.replace("<|im_end|>", "<|end|>");
            fullPrompt += updatedExamples;
        }
        
        if (!conversationHistory.isEmpty()) {
            fullPrompt += conversationHistory;
        }
        
        fullPrompt += QString("<|user|>\n%1<|end|>\n").arg(message);
        fullPrompt += QString("<|assistant|>\n");
        
        chatDisplay->append(Styles::HTML_LLM);
        currentResponse.clear();
        
        emit generateResponse(fullPrompt, generationSettings);
    }

    void onPartialResponse(const QString &token) {
        currentResponse += token;
         
        QTextCursor cursor = chatDisplay->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(token);
        chatDisplay->setTextCursor(cursor);
        chatDisplay->ensureCursorVisible();
    }
    
    void onResponseGenerated(const QString &response) {
        chatDisplay->append("\n");
         
        conversationHistory += QString("<|user|>\n%1<|end|>\n"
                                    "<|assistant|>\n%2<|end|>\n")
                                .arg(lastUserMessage)
                                .arg(currentResponse);
        
        userInput->setEnabled(true);
        sendButton->setEnabled(true);
        uploadButton->setEnabled(true);
        setStatus(llmStatusLabel, "Ready", Styles::STATUS_READY);
        userInput->setFocus();
    }
    
    void onTranscriptionReady(const QString &text) {
        setStatus(whisperStatusLabel, "Ready", Styles::STATUS_READY);
        
        if (!text.isEmpty()) {
            userInput->setText(text);
            chatDisplay->append(Styles::HTML_TRANSCRIBED.arg(text));
        } else {
            QMessageBox::information(this, "Transcription", "No speech detected in the recording.");
        }
    }
    
    void onClearChatClicked() {
        chatDisplay->clear();
        conversationHistory.clear();
        chatDisplay->append(Styles::HTML_LOADING.arg("Chat history cleared. Starting fresh conversation.") + "\n");
    }
    
    void onSettingsClicked() {
        SettingsDialog dialog(this);
        
            dialog.setSystemPrompt          (systemPrompt);
            dialog.setFewShotExamples       (fewShotExamples);  
            dialog.setMaxTokens             (generationSettings.maxTokens);
            dialog.setContextSize           (contextSettings.contextSize);
            dialog.setThreadCount           (contextSettings.threadCount);
            dialog.setBatchSize             (contextSettings.batchSize);
            dialog.setTemperature           (generationSettings.temperature);
            dialog.setTopP                  (generationSettings.topP);
            dialog.setTopK                  (generationSettings.topK);
            dialog.setPdfTruncationLength   (pdfTruncationLength); 
            
            dialog.setWhisperPrintRealtime  (whisperSettings.printRealtime);
            dialog.setWhisperPrintProgress  (whisperSettings.printProgress);
            dialog.setWhisperPrintTimestamps(whisperSettings.printTimestamps);
            dialog.setWhisperPrintSpecial   (whisperSettings.printSpecial);
            dialog.setWhisperTranslate      (whisperSettings.translate);
            dialog.setWhisperLanguage       (whisperSettings.language);
            dialog.setWhisperThreads        (whisperSettings.threads);
            dialog.setWhisperOffsetMs       (whisperSettings.offsetMs);
            dialog.setWhisperDurationMs     (whisperSettings.durationMs);
            dialog.setWhisperTokenTimestamps(whisperSettings.tokenTimestamps);
            dialog.setWhisperMaxLen         (whisperSettings.maxLen);
            dialog.setWhisperSplitOnWord    (whisperSettings.splitOnWord);
            dialog.setWhisperSuppressBlank  (whisperSettings.suppressBlank);
        
        if (dialog.exec() == QDialog::Accepted) {

            systemPrompt                    = dialog.getSystemPrompt();
            fewShotExamples                 = dialog.getFewShotExamples(); 
            generationSettings.maxTokens    = dialog.getMaxTokens();
            generationSettings.temperature  = dialog.getTemperature();
            generationSettings.topP         = dialog.getTopP();
            generationSettings.topK         = dialog.getTopK();
            pdfTruncationLength             = dialog.getPdfTruncationLength();  

            ContextSettings newContextSettings;
            newContextSettings.contextSize  = dialog.getContextSize();
            newContextSettings.threadCount  = dialog.getThreadCount();
            newContextSettings.batchSize    = dialog.getBatchSize();

            whisperSettings.printRealtime   = dialog.getWhisperPrintRealtime();
            whisperSettings.printProgress   = dialog.getWhisperPrintProgress();
            whisperSettings.printTimestamps = dialog.getWhisperPrintTimestamps();
            whisperSettings.printSpecial    = dialog.getWhisperPrintSpecial();
            whisperSettings.translate       = dialog.getWhisperTranslate();
            whisperSettings.language        = dialog.getWhisperLanguage();
            whisperSettings.threads         = dialog.getWhisperThreads();
            whisperSettings.offsetMs        = dialog.getWhisperOffsetMs();
            whisperSettings.durationMs      = dialog.getWhisperDurationMs();
            whisperSettings.tokenTimestamps = dialog.getWhisperTokenTimestamps();
            whisperSettings.maxLen          = dialog.getWhisperMaxLen();
            whisperSettings.splitOnWord     = dialog.getWhisperSplitOnWord();
            whisperSettings.suppressBlank   = dialog.getWhisperSuppressBlank();
            
            if (newContextSettings.contextSize  != contextSettings.contextSize ||
                newContextSettings.threadCount  != contextSettings.threadCount ||
                newContextSettings.batchSize    != contextSettings.batchSize) {
                
                contextSettings = newContextSettings;
                
                if (worker && !modelPathEdit->text().isEmpty()) {
                    QMessageBox::information(this, "Settings Changed", 
                        "Context settings have changed. Please reload the model for changes to take effect.");
                }
            }
            
            saveSettings();
            
            chatDisplay->append(Styles::HTML_INFO.arg("Settings updated."));
        }
    }

    void onAboutClicked() {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("About Lunaria");
 
        msgBox.setText("<h2>Lunaria v1.0</h2>"
                    "<p><a href=\"https://github.com/RezkyKam50/Lunaria\">https://github.com/RezkyKam50/Lunaria</a></p>");
 
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);
        msgBox.setStandardButtons(QMessageBox::Ok);

        msgBox.exec();
    }
    
    void onError(const QString &error) {
        setProgressBarVisible(progressBar, false);
        setModelControlsEnabled(browseButton, loadButton, true);
        setStatus(llmStatusLabel, "Error", Styles::STATUS_ERROR);
        
        chatDisplay->append(Styles::HTML_ERROR.arg(error));
         
        if (worker) {
            userInput->setEnabled(true);
            sendButton->setEnabled(true);
            uploadButton->setEnabled(true);
        }
    }
    
    void onWhisperError(const QString &error) {
        setProgressBarVisible(whisperProgressBar, false);
        setModelControlsEnabled(whisperBrowseButton, whisperLoadButton, true);
        setStatus(whisperStatusLabel, "Error", Styles::STATUS_ERROR);
        
        QMessageBox::critical(this, "Whisper Error", error);
    }



// Initialization signals (Qt macro).
// Send signals to execute a task
signals:
    void loadModel(const QString &modelPath, const ContextSettings &settings);
    void generateResponse(const QString &prompt, const GenerationSettings &settings);
    void loadWhisperModel(const QString &modelPath);
    void transcribeAudio(const std::vector<float> &audioData, const WhisperSettings &settings);
    

private:
    
    // Objects 
    QLineEdit       *modelPathEdit;
    QLineEdit       *userInput;
    QTextEdit       *chatDisplay;
    QPushButton     *browseButton;
    QPushButton     *loadButton;
    QPushButton     *sendButton;
    QPushButton     *uploadButton;
    QPushButton     *clearButton;
    QProgressBar    *progressBar;
    QLabel          *llmStatusLabel;
    QLineEdit       *whisperPathEdit;
    QPushButton     *whisperBrowseButton;
    QPushButton     *whisperLoadButton;
    QPushButton     *recordButton;
    QLabel          *whisperStatusLabel;
    QProgressBar    *whisperProgressBar;

 
    // Variables
    QAudioSource    *audioInput     = nullptr;
    QBuffer         *audioBuffer    = nullptr;

    QElapsedTimer   modelLoadTimer;
    QElapsedTimer   whisperLoadTimer;
    QByteArray      audioData;

    QString         savedModelPath;
    QString         savedWhisperPath;
    
    QString         currentResponse;
    QString         systemPrompt;
    QString         fewShotExamples;
    QString         conversationHistory;
    QString         lastUserMessage;


    // Thread comms.
    QThread         whisperThread;
    QThread         workerThread;

    WhisperWorker   *whisperWorker;
    LlamaWorker     *worker;

    // Settings
    GenerationSettings  generationSettings;
    ContextSettings     contextSettings;

    WhisperSettings     whisperSettings;

    bool isRecording = false;
    int pdfTruncationLength;
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