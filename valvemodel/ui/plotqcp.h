#pragma once

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QVBoxLayout>
#include <QColor>
#include <QVector>

// Forward declarations
class QCustomPlot;
class QCPGraph;

/**
 * @brief The PlotQCP class provides a wrapper around QCustomPlot with an interface compatible with the original Plot class
 */
class PlotQCP : public QWidget
{
    Q_OBJECT

private:
    QCustomPlot* customPlot;
    double xStart, xStop, yStart, yStop;
    double xScale, yScale;

public:
    explicit PlotQCP(QWidget *parent = nullptr);
    ~PlotQCP();

    /**
     * @brief Sets up the axes with the specified parameters
     * @param xStart Starting value for x-axis
     * @param xStop Ending value for x-axis
     * @param xMajorDivision Major division size for x-axis
     * @param yStart Starting value for y-axis
     * @param yStop Ending value for y-axis
     * @param yMajorDivision Major division size for y-axis
     * @param xLabelEvery Label every nth x tick
     * @param yLabelEvery Label every nth y tick
     */
    void setAxes(double xStart, double xStop, double xMajorDivision, 
                double yStart, double yStop, double yMajorDivision, 
                int xLabelEvery = 0, int yLabelEvery = 0);
    
    /**
     * @brief Returns the QCustomPlot widget
     * @return Pointer to the QCustomPlot widget
     */
    QCustomPlot* getPlot();
    
    /**
     * @brief Compatibility method to get the scene (returns nullptr)
     * @return nullptr (QCustomPlot doesn't use QGraphicsScene)
     */
    QGraphicsScene* getScene();
    
    /**
     * @brief Creates a line segment on the plot
     * @param x1 Starting x coordinate
     * @param y1 Starting y coordinate
     * @param x2 Ending x coordinate
     * @param y2 Ending y coordinate
     * @param pen Pen to use for drawing
     * @return nullptr (QCustomPlot handles this differently)
     */
    QGraphicsLineItem* createSegment(double x1, double y1, double x2, double y2, QPen pen);
    
    /**
     * @brief Creates a text label on the plot
     * @param x X coordinate
     * @param y Y coordinate
     * @param value Value to display
     * @return nullptr (QCustomPlot handles this differently)
     */
    QGraphicsTextItem* createLabel(double x, double y, double value);
    
    /**
     * @brief Clears all graphs and items from the plot
     */
    void clear();
    
    /**
     * @brief Compatibility method (does nothing in QCustomPlot)
     * @param item Item to add
     */
    void add(QGraphicsItem* item);
    
    /**
     * @brief Compatibility method (does nothing in QCustomPlot)
     * @param item Item to remove
     */
    void remove(QGraphicsItem* item);
    
    /**
     * @brief Adds a curve to the plot
     * @param xData X data points
     * @param yData Y data points
     * @param name Curve name
     * @param color Curve color
     * @return Graph index
     */
    int addCurve(const QVector<double>& xData, const QVector<double>& yData, 
                const QString& name, const QColor& color);
    
    /**
     * @brief Adds a load line to the plot
     * @param startX Starting x coordinate
     * @param startY Starting y coordinate
     * @param endX Ending x coordinate
     * @param endY Ending y coordinate
     * @param name Line name
     * @param color Line color
     * @return Graph index
     */
    int addLoadLine(double startX, double startY, double endX, double endY,
                   const QString& name, const QColor& color);
    
    /**
     * @brief Marks an operating point on the plot
     * @param x X coordinate
     * @param y Y coordinate
     * @param name Point name
     */
    void markOperatingPoint(double x, double y, const QString& name);
    
    /**
     * @brief Redraws the plot
     */
    void replot();

};
