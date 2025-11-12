#ifndef ENERGYMONITOR_H
#define ENERGYMONITOR_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QChartView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QChart;
class QLineSeries;
class QValueAxis;
class QChartView;
QT_END_NAMESPACE

class EnergyMonitor : public QMainWindow
{
    Q_OBJECT

public:
    EnergyMonitor(QWidget *parent = nullptr);
    ~EnergyMonitor();

private slots:
    void updateData();

private:
    void setupUI();
    void setupChart();
    double getRealCPUEnergy();
    double getRealGPUEnergy();
    double readPowerFromFile(const QString& path);
    QString findPowercapPath();
    QString findGPUPowerPath();
    
    // UI Components
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QChartView *chartView;
    QHBoxLayout *infoLayout;
    QLabel *cpuLabel;
    QLabel *gpuLabel;
    
    // Chart components
    QChart *chart;
    QLineSeries *cpuSeries;
    QLineSeries *gpuSeries;
    QValueAxis *axisX;
    QValueAxis *axisY;
    
    // Data
    QVector<double> cpuEnergyData;
    QVector<double> gpuEnergyData;
    QTimer *dataTimer;
    int timeCounter;
    
    // Sensor paths
    QString cpuPowerPath;
    QString gpuPowerPath;
    
    // Constants
    static const int MAX_DATA_POINTS = 100;
};

#endif // ENERGYMONITOR_H