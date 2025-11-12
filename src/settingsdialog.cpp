#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumSize(600, 500);
    
    setupUI();
    loadDefaults();
}

void SettingsDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    QTabWidget *tabWidget = new QTabWidget();
    
    // === System Prompt Tab ===
    QWidget *promptTab = new QWidget();
    auto *promptLayout = new QVBoxLayout(promptTab);
    
    auto *promptGroup = new QGroupBox("System Prompt");
    auto *promptGroupLayout = new QVBoxLayout();
    
    QLabel *promptLabel = new QLabel(
        "The system prompt sets the AI's behavior and personality. "
        "This will be prepended to every conversation."
    );
    promptLabel->setWordWrap(true);
    promptLabel->setStyleSheet("color: gray; font-style: italic;");
    
    systemPromptEdit = new QTextEdit();
    systemPromptEdit->setPlaceholderText("Enter system prompt here...");
    systemPromptEdit->setMinimumHeight(250);
    
    promptGroupLayout->addWidget(promptLabel);
    promptGroupLayout->addWidget(systemPromptEdit);
    promptGroup->setLayout(promptGroupLayout);
    
    promptLayout->addWidget(promptGroup);
    promptLayout->addStretch();
    
    // === Generation Parameters Tab ===
    QWidget *paramsTab = new QWidget();
    auto *paramsLayout = new QVBoxLayout(paramsTab);
    
    auto *generationGroup = new QGroupBox("Generation Settings");
    auto *generationForm = new QFormLayout();
    
    maxTokensSpin = new QSpinBox();
    maxTokensSpin->setRange(1, 8192);
    maxTokensSpin->setSingleStep(128);
    generationForm->addRow("Max Tokens:", maxTokensSpin);
    
    temperatureSpin = new QDoubleSpinBox();
    temperatureSpin->setRange(0.0, 2.0);
    temperatureSpin->setSingleStep(0.1);
    temperatureSpin->setDecimals(2);
    
    // Create a widget container for the label and description
    auto *tempLabelWidget = new QWidget();
    auto *tempLabelLayout = new QVBoxLayout(tempLabelWidget);
    tempLabelLayout->setContentsMargins(0, 0, 0, 0);
    tempLabelLayout->setSpacing(0);
    QLabel *tempLabel = new QLabel("Temperature:");
    QLabel *tempDesc = new QLabel("(Higher = more random)");
    tempDesc->setStyleSheet("color: gray; font-size: 10px;");
    tempLabelLayout->addWidget(tempLabel);
    tempLabelLayout->addWidget(tempDesc);
    
    generationForm->addRow(tempLabelWidget, temperatureSpin);
    
    topPSpin = new QDoubleSpinBox();
    topPSpin->setRange(0.0, 1.0);
    topPSpin->setSingleStep(0.05);
    topPSpin->setDecimals(2);
    
    // Create a widget container for the label and description
    auto *topPLabelWidget = new QWidget();
    auto *topPLabelLayout = new QVBoxLayout(topPLabelWidget);
    topPLabelLayout->setContentsMargins(0, 0, 0, 0);
    topPLabelLayout->setSpacing(0);
    QLabel *topPLabel = new QLabel("Top P:");
    QLabel *topPDesc = new QLabel("(Nucleus sampling)");
    topPDesc->setStyleSheet("color: gray; font-size: 10px;");
    topPLabelLayout->addWidget(topPLabel);
    topPLabelLayout->addWidget(topPDesc);
    
    generationForm->addRow(topPLabelWidget, topPSpin);
    
    topKSpin = new QSpinBox();
    topKSpin->setRange(1, 100);
    topKSpin->setSingleStep(5);
    generationForm->addRow("Top K:", topKSpin);
    
    generationGroup->setLayout(generationForm);
    
    // === Context Parameters ===
    auto *contextGroup = new QGroupBox("Context Settings");
    auto *contextForm = new QFormLayout();
    
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
    
    contextGroup->setLayout(contextForm);
    
    paramsLayout->addWidget(generationGroup);
    paramsLayout->addWidget(contextGroup);
    paramsLayout->addStretch();
    
    // Add tabs
    tabWidget->addTab(promptTab, "System Prompt");
    tabWidget->addTab(paramsTab, "Parameters");
    
    mainLayout->addWidget(tabWidget);
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::loadDefaults()
{
    systemPromptEdit->setText("You are a helpful AI assistant.");
    maxTokensSpin->setValue(512);
    contextSizeSpin->setValue(2048);
    threadCountSpin->setValue(8);
    batchSizeSpin->setValue(512);
    temperatureSpin->setValue(0.7);
    topPSpin->setValue(0.9);
    topKSpin->setValue(40);
}

// Getters
QString SettingsDialog::getSystemPrompt() const {
    return systemPromptEdit->toPlainText();
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

// Setters
void SettingsDialog::setSystemPrompt(const QString &prompt) {
    systemPromptEdit->setText(prompt);
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