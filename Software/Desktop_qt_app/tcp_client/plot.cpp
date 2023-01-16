#include "plot.h"
#include "ui_plot.h"

Plot::Plot(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Plot)
{
    ui->setupUi(this);
    bench = new Bench("","");
    connect(ui->close_btn, SIGNAL(clicked()), this, SLOT(close()));
}

void Plot::plot_show()
{

    if(bench->Spower_vec.size() > 0)
    {
        ui->custom_plot->setInteraction(QCP::iRangeDrag, true);
        ui->custom_plot->setInteraction(QCP::iRangeZoom, true);
        ui->custom_plot->addGraph();
        ui->custom_plot->graph(0)->setData(bench->timestamp_vec, bench->Spower_vec);
//        ui->custom_plot->axisRect()->setRangeDrag(Qt::Horizontal);   // Enable only drag along the horizontal axis
//        ui->custom_plot->axisRect()->setRangeZoom(Qt::Horizontal);   // Enable zoom only on the horizontal axis
        ui->custom_plot->xAxis->setLabel("Время");
        ui->custom_plot->xAxis->setRange(bench->timestamp_vec.first(), bench->timestamp_vec.last());
        QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
        dateTicker->setDateTimeFormat("hh:mm:ss");
        QDateTime dt_f = QDateTime::fromSecsSinceEpoch(bench->timestamp_vec.first());
        QDateTime dt_l = QDateTime::fromSecsSinceEpoch(bench->timestamp_vec.last());
        qint64 dt = dt_f.secsTo(dt_l);
//        qDebug() << dt;
        ui->custom_plot->xAxis->setTicker(dateTicker);

        ui->custom_plot->yAxis->setLabel("Полная мощность");
        ui->custom_plot->yAxis->setRange(*std::min_element(bench->Spower_vec.constBegin(), bench->Spower_vec.constEnd()),
                                         *std::max_element(bench->Spower_vec.constBegin(), bench->Spower_vec.constEnd()));

        ui->custom_plot->replot();

        this->show();

    }
    else
        QMessageBox::warning(this, "", "Пока данных нет. Попробуйте переподключиться позже.");

}

void Plot::closeEvent(QCloseEvent *event)
{
    if (bench->Spower_vec.size() > 0)
    {
        ui->custom_plot->graph(0)->data()->clear();
        event->accept();
        qDebug() << "Graph was closed";
    }
    else
    {
        event->ignore();
    }
}

Plot::~Plot()
{
    delete ui;
}
