#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QDebug>
#include <QDateTime>
#include <QString>
#include <QList>

#include "bench.h"
#include "plot.h"


#define POINTS_NUM  56

QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();

    QList<Bench *> bench_list;
    int current_bench;
    void show_bench(Bench *b);

public slots:
    void do_connect();
    void do_disconnect();
    void bytes_written(qint64 bytes);
    void ready_read();
    void set_sys_time();
    void plot();
    void show_bench_info(int index);

private:
    QTcpSocket *tcpsocket;
    Bench *mazak;
    Bench *zak;
    Plot *graph;
    enum rx_packet { IP, SERNUM, BLOCKS_NUM};
    /*current packet number*/
    uint32_t rx_num = 0;
    uint32_t temp = 0;
    QDateTime dt;

    Ui::Client *ui;
};
#endif // CLIENT_H
