#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    
    // Getters  
    QString getSystemPrompt () const;
    int getMaxTokens        () const;
    int getContextSize      () const;
    int getThreadCount      () const;
    int getBatchSize        () const;
    double getTemperature   () const;
    double getTopP          () const;
    int getTopK             () const;
    
    // Setters 
    void setSystemPrompt    (const QString &prompt);
    void setMaxTokens       (int tokens);
    void setContextSize     (int size);
    void setThreadCount     (int threads);
    void setBatchSize       (int size);
    void setTemperature     (double temp);
    void setTopP            (double p);
    void setTopK            (int k);

private:
    QTextEdit       *systemPromptEdit;
    QSpinBox        *maxTokensSpin;
    QSpinBox        *contextSizeSpin;
    QSpinBox        *threadCountSpin;
    QSpinBox        *batchSizeSpin;
    QDoubleSpinBox  *temperatureSpin;
    QDoubleSpinBox  *topPSpin;
    QSpinBox        *topKSpin;
    
    void setupUI();
    void loadDefaults();
};

#endif // SETTINGSDIALOG_H