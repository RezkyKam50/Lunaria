#include "settingsdialog.h"
#include "whisperworker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QFrame>
#include <QScrollArea>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumSize(600, 700);
    
    setupUI();
    loadDefaults();
}

void SettingsDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->setDocumentMode(true);
       
    QWidget *promptTab = new QWidget();
    auto *promptLayout = new QVBoxLayout(promptTab);
    promptLayout->setContentsMargins(8, 8, 8, 8);
     
    auto *systemPromptGroup = new QGroupBox("System Prompt");
    auto *systemPromptLayout = new QVBoxLayout(systemPromptGroup);
    systemPromptLayout->setSpacing(8);
    
    QLabel *systemPromptLabel = new QLabel(
        "The system prompt sets the AI's behavior and personality. "
        "This will be prepended to every conversation."
    );
    systemPromptLabel->setWordWrap(true);
    systemPromptLabel->setStyleSheet("color: #666; font-size: 11px; padding: 4px;");
    
    systemPromptEdit = new QTextEdit();
    systemPromptEdit->setPlaceholderText("Enter system prompt here...");
    systemPromptEdit->setMinimumHeight(120);
    systemPromptEdit->setAcceptRichText(false);
    
    systemPromptLayout->addWidget(systemPromptLabel);
    systemPromptLayout->addWidget(systemPromptEdit);
     
    auto *fewShotGroup = new QGroupBox("Few-Shot Examples");
    auto *fewShotLayout = new QVBoxLayout(fewShotGroup);
    fewShotLayout->setSpacing(8);
    
    QLabel *fewShotLabel = new QLabel(
        "Provide example conversations to guide the AI's responses. "
        "Format: System prompt, then user/assistant exchanges. "
    );
    fewShotLabel->setWordWrap(true);
    fewShotLabel->setStyleSheet("color: #666; font-size: 11px; padding: 4px;");
    
    fewShotExamplesEdit = new QTextEdit();
    fewShotExamplesEdit->setPlaceholderText("Enter few-shot examples here...");
    fewShotExamplesEdit->setMinimumHeight(200);
    fewShotExamplesEdit->setAcceptRichText(false);
    
    fewShotLayout->addWidget(fewShotLabel);
    fewShotLayout->addWidget(fewShotExamplesEdit);
    
    promptLayout->addWidget(systemPromptGroup);
    promptLayout->addWidget(fewShotGroup);
    promptLayout->addStretch();
       
    QWidget *paramsTab = new QWidget();
    auto *paramsLayout = new QVBoxLayout(paramsTab);
    paramsLayout->setContentsMargins(8, 8, 8, 8);
    paramsLayout->setSpacing(16);
     
    auto *generationGroup = new QGroupBox("Generation Settings");
    auto *generationForm = new QFormLayout(generationGroup);
    generationForm->setHorizontalSpacing(20);
    generationForm->setVerticalSpacing(12);
    generationForm->setLabelAlignment(Qt::AlignRight);
    generationForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    
    maxTokensSpin = new QSpinBox();
    maxTokensSpin->setRange(1, 8192);
    maxTokensSpin->setSingleStep(128);
    generationForm->addRow("Max Tokens:", maxTokensSpin);
    
    temperatureSpin = new QDoubleSpinBox();
    temperatureSpin->setRange(0.0, 2.0);
    temperatureSpin->setSingleStep(0.1);
    temperatureSpin->setDecimals(2);
    generationForm->addRow("Temperature:", temperatureSpin);
     
    QLabel *tempDesc = new QLabel("Higher values = more random, lower values = more deterministic");
    tempDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");
    generationForm->addRow("", tempDesc);
    
    topPSpin = new QDoubleSpinBox();
    topPSpin->setRange(0.0, 1.0);
    topPSpin->setSingleStep(0.05);
    topPSpin->setDecimals(2);
    generationForm->addRow("Top P:", topPSpin);
    
    QLabel *topPDesc = new QLabel("Nucleus sampling: consider only top P probability mass");
    topPDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");
    generationForm->addRow("", topPDesc);
    
    topKSpin = new QSpinBox();
    topKSpin->setRange(1, 100);
    topKSpin->setSingleStep(5);
    generationForm->addRow("Top K:", topKSpin);
    
    QLabel *topKDesc = new QLabel("Consider only top K tokens");
    topKDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");
    generationForm->addRow("", topKDesc);
    
    pdfTruncationSpin = new QSpinBox();
    pdfTruncationSpin->setRange(100, 10000);
    pdfTruncationSpin->setSingleStep(100);
    pdfTruncationSpin->setSuffix(" characters");
    generationForm->addRow("PDF Truncation:", pdfTruncationSpin);
    
    QLabel *pdfDesc = new QLabel("Maximum characters to extract from PDF files");
    pdfDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");

    QLabel *pdfWarn = new QLabel("*Changing default value upwards may cause Lunaria to crash due to OOM.");
    pdfWarn->setStyleSheet("color: #cc0000; font-size: 10px; font-style: italic; padding-left: 4px;");
    QWidget *pdfLabelsContainer = new QWidget();
    auto *pdfLabelsLayout = new QVBoxLayout(pdfLabelsContainer);
    pdfLabelsLayout->setContentsMargins(0, 0, 0, 0);
    pdfLabelsLayout->setSpacing(2);
    pdfLabelsLayout->addWidget(pdfDesc);
    pdfLabelsLayout->addWidget(pdfWarn);

    generationForm->addRow("", pdfLabelsContainer);
     
    auto *contextGroup = new QGroupBox("Context Settings");
    auto *contextForm = new QFormLayout(contextGroup);
    contextForm->setHorizontalSpacing(20);
    contextForm->setVerticalSpacing(12);
    contextForm->setLabelAlignment(Qt::AlignRight);
    
    contextSizeSpin = new QSpinBox();
    contextSizeSpin->setRange(512, 32768);
    contextSizeSpin->setSingleStep(512);
    contextForm->addRow("Context Size:", contextSizeSpin);
    
    batchSizeSpin = new QSpinBox();
    batchSizeSpin->setRange(128, 2048);
    batchSizeSpin->setSingleStep(128);
    contextForm->addRow("Batch Size:", batchSizeSpin);
    
    threadCountSpin = new QSpinBox();
    threadCountSpin->setRange(1, 32);
    threadCountSpin->setSingleStep(1);
    contextForm->addRow("Thread Count:", threadCountSpin);
    
    paramsLayout->addWidget(generationGroup);
    paramsLayout->addWidget(contextGroup);
    paramsLayout->addStretch();
    
    // ========== WHISPER SETTINGS TAB ==========
    QWidget *whisperTab = new QWidget();
    QScrollArea *whisperScrollArea = new QScrollArea();
    whisperScrollArea->setWidgetResizable(true);
    whisperScrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget *whisperContent = new QWidget();
    auto *whisperLayout = new QVBoxLayout(whisperContent);
    whisperLayout->setContentsMargins(8, 8, 8, 8);
    whisperLayout->setSpacing(16);
    
    auto *whisperGroup = new QGroupBox("Whisper Transcription Settings");
    auto *whisperForm = new QFormLayout(whisperGroup);
    whisperForm->setHorizontalSpacing(20);
    whisperForm->setVerticalSpacing(12);
    whisperForm->setLabelAlignment(Qt::AlignRight);
    whisperForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    
    // Language
    whisperLanguageCombo = new QComboBox();
    whisperLanguageCombo->addItem("Auto-detect", "auto");
    whisperLanguageCombo->addItem("English", "en");
    whisperLanguageCombo->addItem("Spanish", "es");
    whisperLanguageCombo->addItem("French", "fr");
    whisperLanguageCombo->addItem("German", "de");
    whisperLanguageCombo->addItem("Italian", "it");
    whisperLanguageCombo->addItem("Portuguese", "pt");
    whisperLanguageCombo->addItem("Russian", "ru");
    whisperLanguageCombo->addItem("Chinese", "zh");
    whisperLanguageCombo->addItem("Japanese", "ja");
    whisperLanguageCombo->addItem("Korean", "ko");
    whisperForm->addRow("Language:", whisperLanguageCombo);
    
    QLabel *langDesc = new QLabel("Language for transcription (auto-detect recommended)");
    langDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");
    whisperForm->addRow("", langDesc);
    
    // Thread Count
    whisperThreadsSpin = new QSpinBox();
    whisperThreadsSpin->setRange(1, 16);
    whisperThreadsSpin->setSingleStep(1);
    whisperForm->addRow("Thread Count:", whisperThreadsSpin);
    
    QLabel *threadsDesc = new QLabel("Number of CPU threads to use for processing");
    threadsDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");
    whisperForm->addRow("", threadsDesc);
    
    // Max Length
    whisperMaxLenSpin = new QSpinBox();
    whisperMaxLenSpin->setRange(0, 10);
    whisperMaxLenSpin->setSingleStep(1);
    whisperForm->addRow("Max Length:", whisperMaxLenSpin);
    
    QLabel *maxLenDesc = new QLabel("Maximum segment length (0 = unlimited)");
    maxLenDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");
    whisperForm->addRow("", maxLenDesc);
    
    // Offset
    whisperOffsetMsSpin = new QSpinBox();
    whisperOffsetMsSpin->setRange(0, 60000);
    whisperOffsetMsSpin->setSingleStep(1000);
    whisperOffsetMsSpin->setSuffix(" ms");
    whisperForm->addRow("Offset:", whisperOffsetMsSpin);
    
    QLabel *offsetDesc = new QLabel("Start transcription from this time offset");
    offsetDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");
    whisperForm->addRow("", offsetDesc);
    
    // Duration
    whisperDurationMsSpin = new QSpinBox();
    whisperDurationMsSpin->setRange(0, 300000);
    whisperDurationMsSpin->setSingleStep(1000);
    whisperDurationMsSpin->setSuffix(" ms");
    whisperForm->addRow("Duration:", whisperDurationMsSpin);
    
    QLabel *durationDesc = new QLabel("Transcribe only this duration (0 = full audio)");
    durationDesc->setStyleSheet("color: #666; font-size: 10px; font-style: italic; padding-left: 4px;");
    whisperForm->addRow("", durationDesc);
    
    whisperLayout->addWidget(whisperGroup);
    
    // Boolean settings group
    auto *whisperBoolGroup = new QGroupBox("Output & Processing Options");
    auto *whisperBoolLayout = new QVBoxLayout(whisperBoolGroup);
    whisperBoolLayout->setSpacing(8);
    
    whisperTranslateCheck = new QCheckBox("Translate to English");
    whisperTranslateCheck->setToolTip("Translate transcription to English");
    whisperBoolLayout->addWidget(whisperTranslateCheck);
    
    whisperSplitOnWordCheck = new QCheckBox("Split on Word Boundaries");
    whisperSplitOnWordCheck->setToolTip("Split segments on word boundaries (recommended)");
    whisperBoolLayout->addWidget(whisperSplitOnWordCheck);
    
    whisperSuppressBlankCheck = new QCheckBox("Suppress Blank Output");
    whisperSuppressBlankCheck->setToolTip("Suppress blank/empty segments (recommended)");
    whisperBoolLayout->addWidget(whisperSuppressBlankCheck);
    
    whisperPrintTimestampsCheck = new QCheckBox("Print Timestamps");
    whisperPrintTimestampsCheck->setToolTip("Include timestamps in output");
    whisperBoolLayout->addWidget(whisperPrintTimestampsCheck);
    
    whisperTokenTimestampsCheck = new QCheckBox("Token Timestamps");
    whisperTokenTimestampsCheck->setToolTip("Include timestamps for each token");
    whisperBoolLayout->addWidget(whisperTokenTimestampsCheck);
    
    whisperPrintSpecialCheck = new QCheckBox("Print Special Tokens");
    whisperPrintSpecialCheck->setToolTip("Include special tokens in output");
    whisperBoolLayout->addWidget(whisperPrintSpecialCheck);
    
    whisperPrintRealtimeCheck = new QCheckBox("Print Realtime");
    whisperPrintRealtimeCheck->setToolTip("Print output in real-time during transcription");
    whisperBoolLayout->addWidget(whisperPrintRealtimeCheck);
    
    whisperPrintProgressCheck = new QCheckBox("Print Progress");
    whisperPrintProgressCheck->setToolTip("Print progress information during transcription");
    whisperBoolLayout->addWidget(whisperPrintProgressCheck);
    
    whisperLayout->addWidget(whisperBoolGroup);
    whisperLayout->addStretch();
    
    whisperScrollArea->setWidget(whisperContent);
    
    auto *whisperTabLayout = new QVBoxLayout(whisperTab);
    whisperTabLayout->setContentsMargins(0, 0, 0, 0);
    whisperTabLayout->addWidget(whisperScrollArea);
      
    tabWidget->addTab(promptTab, "System Prompt");
    tabWidget->addTab(paramsTab, "LLM Parameters");
    tabWidget->addTab(whisperTab, "Whisper Parameters");
    
    mainLayout->addWidget(tabWidget);
     
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("color: #ddd;");
    mainLayout->addWidget(line);
      
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    buttonBox->setCenterButtons(true);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}
 
void SettingsDialog::loadDefaults()
{
    systemPromptEdit->setText("You are a helpful AI assistant.");
      
    QString defaultFewShot = 
        "system\nYou are a helpful AI assistant.\n"
        "user\nWhat is the capital of France?\n"
        "assistant\nThe capital of France is Paris.\n"
        "user\nHow many planets are in our solar system?\n"
        "assistant\nThere are 8 planets in our solar system.\n";
    
    fewShotExamplesEdit->setText(defaultFewShot);
    
    maxTokensSpin->setValue(512);
    contextSizeSpin->setValue(2048);
    threadCountSpin->setValue(8);
    batchSizeSpin->setValue(512);
    temperatureSpin->setValue(0.7);
    topPSpin->setValue(0.9);
    topKSpin->setValue(40);
    pdfTruncationSpin->setValue(500);
    
    // Whisper defaults
    whisperPrintRealtimeCheck->setChecked(false);
    whisperPrintProgressCheck->setChecked(false);
    whisperPrintTimestampsCheck->setChecked(false);
    whisperPrintSpecialCheck->setChecked(false);
    whisperTranslateCheck->setChecked(false);
    whisperLanguageCombo->setCurrentIndex(1); // "en"
    whisperThreadsSpin->setValue(4);
    whisperOffsetMsSpin->setValue(0);
    whisperDurationMsSpin->setValue(0);
    whisperTokenTimestampsCheck->setChecked(false);
    whisperMaxLenSpin->setValue(1);
    whisperSplitOnWordCheck->setChecked(true);
    whisperSuppressBlankCheck->setChecked(true);
}

# Getters for LLM
void SettingsDialog::setSystemPrompt(const QString &prompt) {
    systemPromptEdit->setText(prompt);
}

void SettingsDialog::setFewShotExamples(const QString &examples) {
    fewShotExamplesEdit->setText(examples);
}

void SettingsDialog::setMaxTokens(int tokens) {
    maxTokensSpin->setValue(tokens);
}

void SettingsDialog::setContextSize(int size) {
    contextSizeSpin->setValue(size);
}

void SettingsDialog::setThreadCount(int threads) {
    threadCountSpin->setValue(threads);
}

void SettingsDialog::setBatchSize(int size) {
    batchSizeSpin->setValue(size);
}

void SettingsDialog::setTemperature(double temp) {
    temperatureSpin->setValue(temp);
}

void SettingsDialog::setTopP(double p) {
    topPSpin->setValue(p);
}

void SettingsDialog::setTopK(int k) {
    topKSpin->setValue(k);
}

void SettingsDialog::setPdfTruncationLength(int length) {
    pdfTruncationSpin->setValue(length);
}


# Setters for LLM
QString SettingsDialog::getSystemPrompt() const {
    return systemPromptEdit->toPlainText();
}

QString SettingsDialog::getFewShotExamples() const {
    return fewShotExamplesEdit->toPlainText();
}

int SettingsDialog::getMaxTokens() const {
    return maxTokensSpin->value();
}

int SettingsDialog::getContextSize() const {
    return contextSizeSpin->value();
}

int SettingsDialog::getThreadCount() const {
    return threadCountSpin->value();
}

int SettingsDialog::getBatchSize() const {
    return batchSizeSpin->value();
}

double SettingsDialog::getTemperature() const {
    return temperatureSpin->value();
}

double SettingsDialog::getTopP() const {
    return topPSpin->value();
}

int SettingsDialog::getTopK() const {
    return topKSpin->value();
}

int SettingsDialog::getPdfTruncationLength() const {
    return pdfTruncationSpin->value();
}


# Getters for Whisper

bool SettingsDialog::getWhisperPrintRealtime() const {
    return whisperPrintRealtimeCheck->isChecked();
}

bool SettingsDialog::getWhisperPrintProgress() const {
    return whisperPrintProgressCheck->isChecked();
}

bool SettingsDialog::getWhisperPrintTimestamps() const {
    return whisperPrintTimestampsCheck->isChecked();
}

bool SettingsDialog::getWhisperPrintSpecial() const {
    return whisperPrintSpecialCheck->isChecked();
}

bool SettingsDialog::getWhisperTranslate() const {
    return whisperTranslateCheck->isChecked();
}

QString SettingsDialog::getWhisperLanguage() const {
    return whisperLanguageCombo->currentData().toString();
}

int SettingsDialog::getWhisperThreads() const {
    return whisperThreadsSpin->value();
}

int SettingsDialog::getWhisperOffsetMs() const {
    return whisperOffsetMsSpin->value();
}

int SettingsDialog::getWhisperDurationMs() const {
    return whisperDurationMsSpin->value();
}

bool SettingsDialog::getWhisperTokenTimestamps() const {
    return whisperTokenTimestampsCheck->isChecked();
}

int SettingsDialog::getWhisperMaxLen() const {
    return whisperMaxLenSpin->value();
}

bool SettingsDialog::getWhisperSplitOnWord() const {
    return whisperSplitOnWordCheck->isChecked();
}

bool SettingsDialog::getWhisperSuppressBlank() const {
    return whisperSuppressBlankCheck->isChecked();
}


# Setters for Whisper
void SettingsDialog::setWhisperPrintRealtime(bool value) {
    whisperPrintRealtimeCheck->setChecked(value);
}

void SettingsDialog::setWhisperPrintProgress(bool value) {
    whisperPrintProgressCheck->setChecked(value);
}

void SettingsDialog::setWhisperPrintTimestamps(bool value) {
    whisperPrintTimestampsCheck->setChecked(value);
}

void SettingsDialog::setWhisperPrintSpecial(bool value) {
    whisperPrintSpecialCheck->setChecked(value);
}

void SettingsDialog::setWhisperTranslate(bool value) {
    whisperTranslateCheck->setChecked(value);
}

void SettingsDialog::setWhisperLanguage(const QString &lang) {
    int index = whisperLanguageCombo->findData(lang);
    if (index >= 0) {
        whisperLanguageCombo->setCurrentIndex(index);
    }
}

void SettingsDialog::setWhisperThreads(int threads) {
    whisperThreadsSpin->setValue(threads);
}

void SettingsDialog::setWhisperOffsetMs(int offset) {
    whisperOffsetMsSpin->setValue(offset);
}

void SettingsDialog::setWhisperDurationMs(int duration) {
    whisperDurationMsSpin->setValue(duration);
}

void SettingsDialog::setWhisperTokenTimestamps(bool value) {
    whisperTokenTimestampsCheck->setChecked(value);
}

void SettingsDialog::setWhisperMaxLen(int len) {
    whisperMaxLenSpin->setValue(len);
}

void SettingsDialog::setWhisperSplitOnWord(bool value) {
    whisperSplitOnWordCheck->setChecked(value);
}

void SettingsDialog::setWhisperSuppressBlank(bool value) {
    whisperSuppressBlankCheck->setChecked(value);
}




