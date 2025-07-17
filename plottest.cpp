#include "valvemodel/ui/plotqcp.h"
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QRandomGenerator>

// Simple test application for PlotQCP
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("PlotQCP Test");
    mainWindow.resize(800, 600);
    
    // Create central widget
    QWidget *centralWidget = new QWidget(&mainWindow);
    mainWindow.setCentralWidget(centralWidget);
    
    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    // Create plot widget
    PlotQCP *plotWidget = new PlotQCP(centralWidget);
    layout->addWidget(plotWidget);
    
    // Create test buttons
    QPushButton *btnTriodeCurves = new QPushButton("Show Triode Curves", centralWidget);
    QPushButton *btnLoadLine = new QPushButton("Add Load Line", centralWidget);
    QPushButton *btnOperatingPoint = new QPushButton("Mark Operating Point", centralWidget);
    QPushButton *btnClear = new QPushButton("Clear Plot", centralWidget);
    
    layout->addWidget(btnTriodeCurves);
    layout->addWidget(btnLoadLine);
    layout->addWidget(btnOperatingPoint);
    layout->addWidget(btnClear);
    
    // Set up axes
    plotWidget->setAxes(0, 400, 50, 0, 50, 5);
    
    // Connect buttons
    QObject::connect(btnTriodeCurves, &QPushButton::clicked, [plotWidget]() {
        // Generate some triode-like curves
        QVector<QColor> colors = {Qt::blue, Qt::red, Qt::green, Qt::magenta, Qt::cyan};
        
        for (int vg = 0; vg > -25; vg -= 5) {
            QVector<double> xData, yData;
            
            // Generate plate curve data points
            for (double va = 0; va <= 400; va += 5) {
                double ia = 10 * (1 - exp((vg - 0) / 10)) * (1 - exp(-va / 250));
                if (ia < 0) ia = 0;
                
                xData.append(va);
                yData.append(ia);
            }
            
            // Add curve to plot
            QString name = QString("Vg = %1V").arg(vg);
            int colorIndex = abs(vg / 5) % colors.size();
            plotWidget->addCurve(xData, yData, name, colors[colorIndex]);
        }
    });
    
    QObject::connect(btnLoadLine, &QPushButton::clicked, [plotWidget]() {
        // Add a load line
        plotWidget->addLoadLine(0, 40, 400, 0, "Load Line (10k)", Qt::black);
    });
    
    QObject::connect(btnOperatingPoint, &QPushButton::clicked, [plotWidget]() {
        // Mark an operating point
        plotWidget->markOperatingPoint(250, 15, "Q Point (250V, 15mA)");
    });
    
    QObject::connect(btnClear, &QPushButton::clicked, [plotWidget]() {
        // Clear the plot
        plotWidget->clear();
        plotWidget->setAxes(0, 400, 50, 0, 50, 5);
    });
    
    // Show window
    mainWindow.show();
    
    return app.exec();
}
