#ifndef PLOT_H
#define PLOT_H

#include <QWidget>
#include <QtCharts>
#include <QChartView>
#include <QLineSeries>
#include <QVector>
#include <QMessageBox>
#include <algorithm>

#include "bench.h"
#include "qcustomplot.h"

namespace Ui {
class Plot;
}

class Plot : public QWidget
{
    Q_OBJECT

public:
    explicit Plot(QWidget *parent = nullptr);
    Bench *bench;
    ~Plot();
public slots:
    void plot_show();
protected:
   void closeEvent(QCloseEvent *event);
private:

    Ui::Plot *ui;
};

#endif // PLOT_H
