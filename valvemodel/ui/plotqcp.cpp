#include "plotqcp.h"

// Include QCustomPlot header
#include "qcustomplot/qcustomplot.h"

#include <QDebug>

PlotQCP::PlotQCP(QWidget *parent) : QWidget(parent)
{
    // Create QCustomPlot instance
    customPlot = new QCustomPlot(this);
    
    // Set up layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(customPlot);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    
    // Configure plot
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    customPlot->axisRect()->setupFullAxesBox(true);
    customPlot->legend->setVisible(true);
    
    // Initialize variables
    xStart = 0.0;
    xStop = 0.0;
    yStart = 0.0;
    yStop = 0.0;
    xScale = 1.0;
    yScale = 1.0;
}

PlotQCP::~PlotQCP()
{
    // QCustomPlot will be deleted by Qt's parent-child mechanism
}

void PlotQCP::setAxes(double _xStart, double _xStop, double xMajorDivision, 
                     double _yStart, double _yStop, double yMajorDivision, 
                     int xLabelEvery, int yLabelEvery)
{
    // Store axis limits
    xStart = _xStart;
    xStop = _xStop;
    yStart = _yStart;
    yStop = _yStop;
    
    // Calculate scales
    xScale = 1.0; // QCustomPlot handles scaling internally
    yScale = 1.0;
    
    // Set up new tickers (no need to clear as we're replacing them)
    
    // Set axis ranges
    customPlot->xAxis->setRange(xStart, xStop);
    customPlot->yAxis->setRange(yStart, yStop);
    
    // Create ticker with specified divisions
    QSharedPointer<QCPAxisTickerFixed> xTicker(new QCPAxisTickerFixed);
    xTicker->setTickStep(xMajorDivision);
    customPlot->xAxis->setTicker(xTicker);
    
    QSharedPointer<QCPAxisTickerFixed> yTicker(new QCPAxisTickerFixed);
    yTicker->setTickStep(yMajorDivision);
    customPlot->yAxis->setTicker(yTicker);
    
    // Set up labels
    if (xLabelEvery > 0) {
        xTicker->setTickStepStrategy(QCPAxisTickerFixed::tssMeetTickCount);
        xTicker->setTickCount(xLabelEvery);
    }
    
    if (yLabelEvery > 0) {
        yTicker->setTickStepStrategy(QCPAxisTickerFixed::tssMeetTickCount);
        yTicker->setTickCount(yLabelEvery);
    }
    
    // Set axis labels
    customPlot->xAxis->setLabel("Anode Voltage (V)");
    customPlot->yAxis->setLabel("Anode Current (mA)");
    
    // Replot
    customPlot->replot();
}

QCustomPlot* PlotQCP::getPlot()
{
    return customPlot;
}

QGraphicsScene* PlotQCP::getScene()
{
    // This is a compatibility method for the old Plot class
    // QCustomPlot doesn't use QGraphicsScene, so return nullptr
    qDebug() << "Warning: getScene() called on PlotQCP, which doesn't use QGraphicsScene";
    return nullptr;
}

QGraphicsLineItem* PlotQCP::createSegment(double x1, double y1, double x2, double y2, QPen pen)
{
    // Create a line using QCustomPlot's QCPItemLine
    QCPItemLine* line = new QCPItemLine(customPlot);
    line->start->setCoords(x1, y1);
    line->end->setCoords(x2, y2);
    line->setPen(pen);
    // Register with parent plot
    line->registerWithParentPlot();
    
    // Replot
    customPlot->replot();
    
    // Return nullptr for compatibility
    return nullptr;
}

QGraphicsTextItem* PlotQCP::createLabel(double x, double y, double value)
{
    // Create a text label using QCustomPlot's QCPItemText
    QCPItemText* textItem = new QCPItemText(customPlot);
    textItem->position->setCoords(x, y);
    textItem->setText(QString::number(value));
    textItem->setFont(QFont("Arial", 9));
    // Register with parent plot
    textItem->registerWithParentPlot();
    
    // Replot
    customPlot->replot();
    
    // Return nullptr for compatibility
    return nullptr;
}

void PlotQCP::clear()
{
    // Clear all graphs and items
    customPlot->clearGraphs();
    customPlot->clearItems();
    
    // Replot
    customPlot->replot();
}

void PlotQCP::add(QGraphicsItem* item)
{
    // Compatibility method - does nothing in QCustomPlot
    Q_UNUSED(item);
    qDebug() << "Warning: add() called on PlotQCP, which doesn't use QGraphicsItems";
}

void PlotQCP::remove(QGraphicsItem* item)
{
    // Compatibility method - does nothing in QCustomPlot
    Q_UNUSED(item);
    qDebug() << "Warning: remove() called on PlotQCP, which doesn't use QGraphicsItems";
}

int PlotQCP::addCurve(const QVector<double>& xData, const QVector<double>& yData, 
                     const QString& name, const QColor& color)
{
    // Create a new graph
    int graphIndex = customPlot->graphCount();
    customPlot->addGraph();
    
    // Set data
    customPlot->graph(graphIndex)->setData(xData, yData);
    
    // Set appearance
    customPlot->graph(graphIndex)->setPen(QPen(color));
    customPlot->graph(graphIndex)->setName(name);
    
    // Replot
    customPlot->replot();
    
    return graphIndex;
}

int PlotQCP::addLoadLine(double startX, double startY, double endX, double endY,
                        const QString& name, const QColor& color)
{
    // Create data vectors for the load line
    QVector<double> xData, yData;
    xData << startX << endX;
    yData << startY << endY;
    
    // Create a new graph
    int graphIndex = customPlot->graphCount();
    customPlot->addGraph();
    
    // Set data
    customPlot->graph(graphIndex)->setData(xData, yData);
    
    // Set appearance
    QPen pen(color);
    pen.setStyle(Qt::DashLine);
    pen.setWidth(2);
    customPlot->graph(graphIndex)->setPen(pen);
    customPlot->graph(graphIndex)->setName(name);
    
    // Replot
    customPlot->replot();
    
    return graphIndex;
}

void PlotQCP::markOperatingPoint(double x, double y, const QString& name)
{
    // Create a point marker using QCPItemTracer
    QCPItemTracer* tracer = new QCPItemTracer(customPlot);
    tracer->setStyle(QCPItemTracer::tsCircle);
    tracer->setSize(10);
    tracer->setPen(QPen(Qt::red));
    tracer->setBrush(QBrush(Qt::red));
    tracer->position->setCoords(x, y);
    // Register with parent plot
    tracer->registerWithParentPlot();
    
    // Add a text label
    QCPItemText* textItem = new QCPItemText(customPlot);
    textItem->position->setCoords(x + 5, y + 5);
    textItem->setText(name);
    textItem->setFont(QFont("Arial", 9));
    // Register with parent plot
    textItem->registerWithParentPlot();
    
    // Replot
    customPlot->replot();
}

void PlotQCP::replot()
{
    customPlot->replot();
}
