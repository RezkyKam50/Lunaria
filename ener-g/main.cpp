#include "energymonitor.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Energy Monitor");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("YourCompany");
    
    // Use Fusion style for consistent cross-platform appearance
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Create and show main window
    EnergyMonitor monitor;
    monitor.show();
    
    return app.exec();
}