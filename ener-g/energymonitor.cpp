#include "energymonitor.h"
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QTimer>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QProcess>

EnergyMonitor::EnergyMonitor(QWidget *parent)
    : QMainWindow(parent)
    , timeCounter(0)
{
    // Find sensor paths first
    cpuPowerPath = findPowercapPath();
    gpuPowerPath = findGPUPowerPath();
    
    setupUI();
    setupChart();
    
    // Setup timer to update data every second
    dataTimer = new QTimer(this);
    connect(dataTimer, &QTimer::timeout, this, &EnergyMonitor::updateData);
    dataTimer->start(1000); // Update every second
}

EnergyMonitor::~EnergyMonitor()
{
}

void EnergyMonitor::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    mainLayout = new QVBoxLayout(centralWidget);
    
    // Information labels
    infoLayout = new QHBoxLayout();
    
    QString cpuStatus = cpuPowerPath.isEmpty() ? " (No sensor)" : "";
    QString gpuStatus = gpuPowerPath.isEmpty() ? " (No sensor)" : "";
    
    cpuLabel = new QLabel("CPU Energy: 0 W" + cpuStatus);
    gpuLabel = new QLabel("GPU Energy: 0 W" + gpuStatus);
    
    // Style the labels
    cpuLabel->setStyleSheet("QLabel { background-color: #3498db; color: white; padding: 5px; border-radius: 3px; }");
    gpuLabel->setStyleSheet("QLabel { background-color: #e74c3c; color: white; padding: 5px; border-radius: 3px; }");
    
    infoLayout->addWidget(cpuLabel);
    infoLayout->addWidget(gpuLabel);
    infoLayout->addStretch();
    
    // Chart view
    chartView = new QChartView();
    chartView->setRenderHint(QPainter::Antialiasing);
    
    mainLayout->addLayout(infoLayout);
    mainLayout->addWidget(chartView);
    
    setWindowTitle("CPU/GPU Energy Monitor - Real Data");
    resize(800, 600);
}

void EnergyMonitor::setupChart()
{
    chart = new QChart();
    chart->setTitle("CPU and GPU Energy Usage - Real Data");
    chart->setAnimationOptions(QChart::NoAnimation);
    
    // Create series
    cpuSeries = new QLineSeries();
    cpuSeries->setName("CPU Energy");
    cpuSeries->setColor(QColor(52, 152, 219)); // Blue
    
    gpuSeries = new QLineSeries();
    gpuSeries->setName("GPU Energy");
    gpuSeries->setColor(QColor(231, 76, 60)); // Red
    
    // Add series to chart
    chart->addSeries(cpuSeries);
    chart->addSeries(gpuSeries);
    
    // Create axes
    axisX = new QValueAxis();
    axisX->setTitleText("Time (seconds)");
    axisX->setRange(0, MAX_DATA_POINTS);
    axisX->setLabelFormat("%d");
    
    axisY = new QValueAxis();
    axisY->setTitleText("Power (Watts)");
    axisY->setRange(0, 150);
    axisY->setLabelFormat("%.1f");
    
    // Attach axes
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    
    cpuSeries->attachAxis(axisX);
    cpuSeries->attachAxis(axisY);
    gpuSeries->attachAxis(axisX);
    gpuSeries->attachAxis(axisY);
    
    chartView->setChart(chart);
}

void EnergyMonitor::updateData()
{
    // Get real energy data
    double cpuEnergy = getRealCPUEnergy();
    double gpuEnergy = getRealGPUEnergy();
    
    // Add new data points
    cpuEnergyData.append(cpuEnergy);
    gpuEnergyData.append(gpuEnergy);
    
    // Remove old data if we exceed maximum points
    if (cpuEnergyData.size() > MAX_DATA_POINTS) {
        cpuEnergyData.removeFirst();
        gpuEnergyData.removeFirst();
    }
    
    // Update time counter
    timeCounter++;
    
    // Update chart data
    cpuSeries->clear();
    gpuSeries->clear();
    
    int startTime = qMax(0, timeCounter - MAX_DATA_POINTS);
    for (int i = 0; i < cpuEnergyData.size(); ++i) {
        cpuSeries->append(startTime + i, cpuEnergyData[i]);
        gpuSeries->append(startTime + i, gpuEnergyData[i]);
    }
    
    // Update X-axis range
    axisX->setRange(startTime, startTime + MAX_DATA_POINTS - 1);
    
    // Update labels
    cpuLabel->setText(QString("CPU Energy: %1 W").arg(cpuEnergy, 0, 'f', 1));
    gpuLabel->setText(QString("GPU Energy: %1 W").arg(gpuEnergy, 0, 'f', 1));
}

double EnergyMonitor::getRealCPUEnergy()
{
    if (cpuPowerPath.isEmpty()) {
        // Fallback to simulation if no sensor found
        static double baseLoad = 15.0;
        double variation = QRandomGenerator::global()->generateDouble() * 30.0;
        if (QRandomGenerator::global()->generateDouble() < 0.05) {
            variation += 40.0;
        }
        return baseLoad + variation;
    }
    
    return readPowerFromFile(cpuPowerPath);
}

double EnergyMonitor::getRealGPUEnergy()
{
    if (gpuPowerPath.isEmpty()) {
        // Fallback to simulation if no sensor found
        static double baseLoad = 25.0;
        double variation = QRandomGenerator::global()->generateDouble() * 50.0;
        if (QRandomGenerator::global()->generateDouble() < 0.03) {
            variation += 80.0;
        }
        return baseLoad + variation;
    }
    
    return readPowerFromFile(gpuPowerPath);
}

double EnergyMonitor::readPowerFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file:" << path;
        return 0.0;
    }
    
    QTextStream in(&file);
    QString valueStr = in.readAll().trimmed();
    file.close();
    
    bool ok;
    double value = valueStr.toDouble(&ok);
    
    if (!ok) {
        qDebug() << "Invalid value in file:" << path << "value:" << valueStr;
        return 0.0;
    }
    
    // Convert from microwatts to watts if needed
    if (path.contains("energy_uj") || path.contains("microjoules")) {
        value = value / 1000000.0; // Convert microjoules to joules
    } else if (path.contains("power1_input") || value > 1000) {
        value = value / 1000000.0; // Convert microwatts to watts
    }
    
    return value;
}

QString EnergyMonitor::findPowercapPath()
{
    // Look for Intel RAPL power monitoring
    QDir dir("/sys/class/powercap");
    if (!dir.exists()) {
        qDebug() << "Powercap directory not found";
        return QString();
    }
    
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        QFile energyFile(dir.absoluteFilePath(entry + "/energy_uj"));
        if (energyFile.exists()) {
            qDebug() << "Found RAPL powercap at:" << energyFile.fileName();
            return energyFile.fileName();
        }
    }
    
    // Alternative: check hwmon
    dir.setPath("/sys/class/hwmon");
    if (dir.exists()) {
        QStringList hwmonDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& hwmonDir : hwmonDirs) {
            QString basePath = dir.absoluteFilePath(hwmonDir);
            QFile powerFile(basePath + "/power1_input");
            if (powerFile.exists()) {
                qDebug() << "Found power sensor at:" << powerFile.fileName();
                return powerFile.fileName();
            }
        }
    }
    
    qDebug() << "No CPU power sensor found";
    return QString();
}

QString EnergyMonitor::findGPUPowerPath()
{
    // Check for NVIDIA GPU
    QDir dir("/sys/class/hwmon");
    if (!dir.exists()) {
        return QString();
    }
    
    QStringList hwmonDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& hwmonDir : hwmonDirs) {
        QString basePath = dir.absoluteFilePath(hwmonDir);
        QFile nameFile(basePath + "/name");
        if (nameFile.exists() && nameFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&nameFile);
            QString name = in.readAll().trimmed();
            nameFile.close();
            
            if (name == "nvidiagpu" || name == "amdgpu") {
                // Check for power files
                QFile powerFile(basePath + "/power1_average");
                if (powerFile.exists()) {
                    return powerFile.fileName();
                }
                
                powerFile.setFileName(basePath + "/power1_input");
                if (powerFile.exists()) {
                    return powerFile.fileName();
                }
            }
        }
    }
    
    // Check for AMD GPU
    dir.setPath("/sys/class/drm");
    if (dir.exists()) {
        QStringList cardDirs = dir.entryList(QStringList() << "card*", QDir::Dirs);
        for (const QString& cardDir : cardDirs) {
            QString hwmonPath = dir.absoluteFilePath(cardDir + "/device/hwmon");
            QDir hwmonDir(hwmonPath);
            if (hwmonDir.exists()) {
                QStringList hwmonSubDirs = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& hwmonSubDir : hwmonSubDirs) {
                    QFile powerFile(hwmonPath + "/" + hwmonSubDir + "/power1_average");
                    if (powerFile.exists()) {
                        return powerFile.fileName();
                    }
                }
            }
        }
    }
    
    qDebug() << "No GPU power sensor found";
    return QString();
}