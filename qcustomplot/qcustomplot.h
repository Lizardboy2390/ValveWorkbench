#ifndef QCUSTOMPLOT_H
#define QCUSTOMPLOT_H

#include <QWidget>
#include <QVector>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QSharedPointer>
#include <QLayout>
#include <QVBoxLayout>

// Forward declarations
class QCustomPlot;
class QCPAxisRect;
class QCPLegend;
class QCPAxis;
class QCPGraph;
class QCPItemLine;
class QCPItemText;
class QCPItemTracer;
class QCPAxisTicker;

// Namespace for QCustomPlot enums
namespace QCP {
  enum InteractionFlag { iRangeDrag = 0x001, iRangeZoom = 0x002 };
  Q_DECLARE_FLAGS(Interactions, InteractionFlag)
}
Q_DECLARE_OPERATORS_FOR_FLAGS(QCP::Interactions)

// QCPAxisTickerFixed class for fixed tick intervals
class QCPAxisTickerFixed : public QObject
{
  Q_OBJECT
public:
  enum TickStepStrategy { tssReadability, tssMeetTickCount };
  
  QCPAxisTickerFixed() : mTickStep(1.0), mTickCount(5), mStrategy(tssReadability) {}
  
  void setTickStep(double step) { mTickStep = step; }
  void setTickCount(int count) { mTickCount = count; }
  void setTickStepStrategy(TickStepStrategy strategy) { mStrategy = strategy; }
  
private:
  double mTickStep;
  int mTickCount;
  TickStepStrategy mStrategy;
};

// QCPAxis class for axis handling
class QCPAxis : public QObject
{
  Q_OBJECT
public:
  QCPAxis() : mLabel(""), mRange(0, 10) {}
  
  void setLabel(const QString &label) { mLabel = label; }
  void setRange(double lower, double upper) { mRange = qMakePair(lower, upper); }
  
  QSharedPointer<QCPAxisTickerFixed> ticker() const { return mTicker; }
  void setTicker(QSharedPointer<QCPAxisTickerFixed> ticker) { mTicker = ticker; }
  
private:
  QString mLabel;
  QPair<double, double> mRange;
  QSharedPointer<QCPAxisTickerFixed> mTicker;
};

// QCPAxisRect class for axis rectangle handling
class QCPAxisRect : public QObject
{
  Q_OBJECT
public:
  void setupFullAxesBox(bool setup) { Q_UNUSED(setup); }
};

// QCPLegend class for legend handling
class QCPLegend : public QObject
{
  Q_OBJECT
public:
  void setVisible(bool visible) { mVisible = visible; }
  
private:
  bool mVisible;
};

// QCPGraph class for graph handling
class QCPGraph : public QObject
{
  Q_OBJECT
public:
  void setData(const QVector<double> &keys, const QVector<double> &values) {
    mKeys = keys;
    mValues = values;
  }
  
  void setPen(const QPen &pen) { mPen = pen; }
  void setName(const QString &name) { mName = name; }
  
private:
  QVector<double> mKeys;
  QVector<double> mValues;
  QPen mPen;
  QString mName;
};

// QCPItemPosition class for item positioning
class QCPItemPosition : public QObject
{
  Q_OBJECT
public:
  void setCoords(double key, double value) {
    mKey = key;
    mValue = value;
  }
  
private:
  double mKey;
  double mValue;
};

// Forward declaration for QCPItemLine constructor
class QCustomPlot;

// QCPItemLine class for line items
class QCPItemLine : public QObject
{
  Q_OBJECT
public:
  QCPItemLine(QCustomPlot *parentPlot) {
    start = new QCPItemPosition();
    end = new QCPItemPosition();
  }
  
  ~QCPItemLine() {
    delete start;
    delete end;
  }
  
  void setPen(const QPen &pen) { mPen = pen; }
  
  QCPItemPosition *start;
  QCPItemPosition *end;
  
private:
  QPen mPen;
};

// QCPItemText class for text items
class QCPItemText : public QObject
{
  Q_OBJECT
public:
  QCPItemText(QCustomPlot *parentPlot) {
    position = new QCPItemPosition();
  }
  
  ~QCPItemText() {
    delete position;
  }
  
  void setText(const QString &text) { mText = text; }
  void setFont(const QFont &font) { mFont = font; }
  
  QCPItemPosition *position;
  
private:
  QString mText;
  QFont mFont;
};

// QCPItemTracer class for point tracers
class QCPItemTracer : public QObject
{
  Q_OBJECT
public:
  enum TracerStyle { tsNone, tsPlus, tsCrosshair, tsCircle, tsSquare };
  
  QCPItemTracer(QCustomPlot *parentPlot) : mStyle(tsCircle), mSize(6.0) {
    position = new QCPItemPosition();
  }
  
  ~QCPItemTracer() {
    delete position;
  }
  
  void setStyle(TracerStyle style) { mStyle = style; }
  void setSize(double size) { mSize = size; }
  void setPen(const QPen &pen) { mPen = pen; }
  void setBrush(const QBrush &brush) { mBrush = brush; }
  
  QCPItemPosition *position;
  
private:
  TracerStyle mStyle;
  double mSize;
  QPen mPen;
  QBrush mBrush;
};

// Main QCustomPlot class
class QCustomPlot : public QWidget
{
  Q_OBJECT
  
public:
  explicit QCustomPlot(QWidget *parent = nullptr) : QWidget(parent) {
    mAxisRect = new QCPAxisRect();
    mLegend = new QCPLegend();
    mXAxis = new QCPAxis();
    mYAxis = new QCPAxis();
  }
  
  ~QCustomPlot() {
    clearGraphs();
    clearItems();
    delete mAxisRect;
    delete mLegend;
    delete mXAxis;
    delete mYAxis;
  }
  
  void setInteractions(QCP::Interactions interactions) { mInteractions = interactions; }
  
  QCPAxisRect *axisRect() const { return mAxisRect; }
  QCPLegend *legend;
  QCPAxis *xAxis;
  QCPAxis *yAxis;
  
  void clearGraphs() {
    qDeleteAll(mGraphs);
    mGraphs.clear();
  }
  
  void clearItems() {
    qDeleteAll(mItems);
    mItems.clear();
  }
  
  void addGraph() {
    QCPGraph *newGraph = new QCPGraph();
    mGraphs.append(newGraph);
  }
  
  QCPGraph *graph(int index) const {
    if (index >= 0 && index < mGraphs.size())
      return mGraphs.at(index);
    return nullptr;
  }
  
  int graphCount() const { return mGraphs.size(); }
  
  void replot() {
    // In a real implementation, this would trigger a repaint
    update();
  }
  
private:
  QCPAxisRect *mAxisRect;
  QCPLegend *mLegend;
  QCPAxis *mXAxis;
  QCPAxis *mYAxis;
  QCP::Interactions mInteractions;
  QList<QCPGraph*> mGraphs;
  QList<QObject*> mItems;
  
  // Make these accessible to item classes
  friend class QCPItemLine;
  friend class QCPItemText;
  friend class QCPItemTracer;
};

#endif // QCUSTOMPLOT_H
