#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>

struct WhisperSettings;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    
    // Getters  
    QString getSystemPrompt         () const;
    QString getFewShotExamples      () const;
    int getMaxTokens                () const;
    int getContextSize              () const;
    int getThreadCount              () const;
    int getBatchSize                () const;
    double getTemperature           () const;
    double getTopP                  () const;
    int getTopK                     () const;
    int getPdfTruncationLength      () const;
    
    // Whisper getters
    bool getWhisperPrintRealtime    () const;
    bool getWhisperPrintProgress    () const;
    bool getWhisperPrintTimestamps  () const;
    bool getWhisperPrintSpecial     () const;
    bool getWhisperTranslate        () const;
    QString getWhisperLanguage      () const;
    int getWhisperThreads           () const;
    int getWhisperOffsetMs          () const;
    int getWhisperDurationMs        () const;
    bool getWhisperTokenTimestamps  () const;
    int getWhisperMaxLen            () const;
    bool getWhisperSplitOnWord      () const;
    bool getWhisperSuppressBlank    () const;
    
    // Setters 
    void setSystemPrompt            (const QString &prompt);
    void setFewShotExamples         (const QString &examples);
    void setMaxTokens               (int tokens);
    void setContextSize             (int size);
    void setThreadCount             (int threads);
    void setBatchSize               (int size);
    void setTemperature             (double temp);
    void setTopP                    (double p);
    void setTopK                    (int k);
    void setPdfTruncationLength     (int length);
    
    // Whisper setters
    void setWhisperPrintRealtime    (bool value);
    void setWhisperPrintProgress    (bool value);
    void setWhisperPrintTimestamps  (bool value);
    void setWhisperPrintSpecial     (bool value);
    void setWhisperTranslate        (bool value);
    void setWhisperLanguage         (const QString &lang);
    void setWhisperThreads          (int threads);
    void setWhisperOffsetMs         (int offset);
    void setWhisperDurationMs       (int duration);
    void setWhisperTokenTimestamps  (bool value);
    void setWhisperMaxLen           (int len);
    void setWhisperSplitOnWord      (bool value);
    void setWhisperSuppressBlank    (bool value);

private:
    QTextEdit                       *systemPromptEdit;
    QTextEdit                       *fewShotExamplesEdit;
    QSpinBox                        *maxTokensSpin;
    QSpinBox                        *contextSizeSpin;
    QSpinBox                        *threadCountSpin;
    QSpinBox                        *batchSizeSpin;
    QDoubleSpinBox                  *temperatureSpin;
    QDoubleSpinBox                  *topPSpin;
    QSpinBox                        *topKSpin;
    QSpinBox                        *pdfTruncationSpin;
     
    QCheckBox                       *whisperPrintRealtimeCheck;
    QCheckBox                       *whisperPrintProgressCheck;
    QCheckBox                       *whisperPrintTimestampsCheck;
    QCheckBox                       *whisperPrintSpecialCheck;
    QCheckBox                       *whisperTranslateCheck;
    QComboBox                       *whisperLanguageCombo;
    QSpinBox                        *whisperThreadsSpin;
    QSpinBox                        *whisperOffsetMsSpin;
    QSpinBox                        *whisperDurationMsSpin;
    QCheckBox                       *whisperTokenTimestampsCheck;
    QSpinBox                        *whisperMaxLenSpin;
    QCheckBox                       *whisperSplitOnWordCheck;
    QCheckBox                       *whisperSuppressBlankCheck;
    
    void setupUI();
    void loadDefaults();
};

#endif // SETTINGSDIALOG_H