#include "client.h"
#include "ui_client.h"
#include "ui_plot.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);

    /*NETWORK setting*/
    tcpsocket = new QTcpSocket(this);
    /*Bench(IP, MERC_NET_ADDR)*/
    mazak = new Bench("169.254.28.12", "52");
    zak = new Bench("169.254.28.21", "42");
    graph = new Plot();

    bench_list.append(mazak);
    bench_list.append(zak);

    connect(ui->connect_btn, SIGNAL(clicked()), this, SLOT(do_connect()));
    connect(ui->disconnect_btn, SIGNAL(clicked()), this, SLOT(do_disconnect()));
    connect(ui->plot_btn, SIGNAL(clicked()), this, SLOT(plot()));
    connect(ui->time_btn, SIGNAL(clicked()), this, SLOT(set_sys_time()));
    connect(tcpsocket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytes_written(qint64)));
    connect(tcpsocket, SIGNAL(readyRead()), this, SLOT(ready_read()));
    connect(ui->status_clear_btn, SIGNAL(clicked()), ui->status_text, SLOT(clear()));
    connect(ui->com_box_bench, SIGNAL(activated(int)), this, SLOT(show_bench_info(int)));
    connect(ui->plot_btn, SIGNAL(clicked()), graph, SLOT(plot_show()));

    ui->com_box_bench->setCurrentIndex(-1);

    ui->connect_btn->setEnabled(false);
    ui->disconnect_btn->setEnabled(false);
    ui->plot_btn->setEnabled(false);
    ui->time_btn->setEnabled(false);
}

void Client::do_connect()
{
    qDebug() << "connecting...";
    tcpsocket->connectToHost(bench_list[current_bench]->get_ip(), 5000);
    if(tcpsocket->waitForConnected(2000))
    {
       ui->status_text->QTextEdit::append("Connected!");
       ui->connect_btn->setEnabled(false);
       ui->disconnect_btn->setEnabled(true);
       ui->plot_btn->setEnabled(true);
       ui->time_btn->setEnabled(true);
       ui->com_box_bench->setEnabled(false);
    }
    else
       ui->status_text->QTextEdit::append("Error: " + tcpsocket->errorString());

    /*ask data*/
    QByteArray cmd = "GET_ALL~";
    if(tcpsocket->write(cmd) == static_cast<qint64>(strlen(cmd)))
        ui->status_text->QTextEdit::append("OK");
    else
        ui->status_text->QTextEdit::append("WRITE_ERR");

}

void Client::do_disconnect()
{
     tcpsocket->disconnectFromHost();
     if(tcpsocket->state() == QAbstractSocket::UnconnectedState)
        ui->status_text->QTextEdit::append("Disconnected!");
     ui->connect_btn->setEnabled(true);
     ui->disconnect_btn->setEnabled(false);
     ui->com_box_bench->setEnabled(true);
     ui->plot_btn->setEnabled(false);
     ui->time_btn->setEnabled(false);
     rx_num = 0;
}

void Client::bytes_written(qint64 bytes)
{
    qDebug() << bytes << " bytes written";
}

void Client::ready_read()
{
    qDebug() << "reading...";
    switch(rx_num)
    {
//        case IP:
//        qDebug() << (bench_list[current_bench]->ip = tcpsocket->readAll()) << '\n';
//        break;

//        case SERNUM:
//        qDebug() << tcpsocket->readAll() << '\n';
//        break;

//        case BLOCKS_NUM:
//        qDebug() << tcpsocket->readAll() << '\n';
//        break;

        default:
        bench_list[current_bench]->raw_data.append(tcpsocket->readAll());
        qDebug() << bench_list[current_bench]->raw_data.size() << " bytes\n" << bench_list[current_bench]->raw_data.size() / 512 << " blocks\n";
    }
    rx_num++;
}

void Client::set_sys_time()
{
    qDebug() << "setting controller rtc...";
    QByteArray cmd = "SET_DT";
    qint32 timestamp = QDateTime::currentSecsSinceEpoch();
    for(int i = 3; i >= 0; i--)
        cmd.append((timestamp >> (8 * (3 - i))));
    cmd.append('~');
    if(tcpsocket->write(cmd) == static_cast<qint64>(strlen(cmd)))
        ui->status_text->QTextEdit::append("RTC_SET");
    else
        ui->status_text->QTextEdit::append("ERROR_RTC_SET");
}

void Client::plot()
{
    /*
        Total received sd card blocks of 512 bytes.
        Contains 56 points of power(4 bytes each), 56 points of timestamp(4 bytes each) and 56 points of Bench state(1 byte each).
        Total 56 * 4 + 56 * 4 + 56 * 1 = 504 bytes of data in 512 block.
    */
    qDebug() << bench_list[current_bench]->raw_data.size() / 512 << " blocks\n";


    /*parsing each block of 512 bytes*/
    for(int32_t ib = 0; ib < bench_list[current_bench]->raw_data.size() / 512; ib++)
    {
        for(uint32_t jp = 0; jp < POINTS_NUM; jp++)
        {
            for(int8_t k = 3; k >= 0; k--)
            {
                /*take power values*/
                temp = static_cast<uint8_t>(bench_list[current_bench]->raw_data[k + jp * 4 + ib * 512]) << 8 * k;
                qDebug() << k << Qt::hex << temp;
                bench_list[current_bench]->Spower_u32 += temp;
                bench_list[current_bench]->Spower_f = (static_cast<float>(bench_list[current_bench]->Spower_u32)) / 100;
                /*take date values*/
                temp = (static_cast<uint8_t>(bench_list[current_bench]->raw_data[224 + k + jp * 4 + ib * 512]) << 8 * k) & (0x000000FF << 8 * k);
                bench_list[current_bench]->timestamp += temp;
            }
            bench_list[current_bench]->Spower_vec.append(bench_list[current_bench]->Spower_f);
            bench_list[current_bench]->date_vec.append(QDateTime::fromSecsSinceEpoch(bench_list[current_bench]->timestamp, Qt::LocalTime));
            bench_list[current_bench]->timestamp_vec.append(bench_list[current_bench]->timestamp);
            /*take bench state values*/
            bench_list[current_bench]->status_vec.append(bench_list[current_bench]->raw_data[448 + jp]);
//            qDebug() << bench_list[current_bench]->Spower_u32;
            bench_list[current_bench]->Spower_u32 = 0;
            bench_list[current_bench]->timestamp = 0;
            qDebug() << bench_list[current_bench]->Spower_vec[jp + ib * POINTS_NUM] << "W\t" <<  bench_list[current_bench]->date_vec[jp + ib * POINTS_NUM].toString("yyyy-MM-dd HH:mm:ss") << "\t" << jp + ib * POINTS_NUM;
            switch(bench_list[current_bench]->status_vec[jp + ib * POINTS_NUM])
            {
                case Bench::RED:
                qDebug() << "RED";
                break;

                case Bench::YELLOW:
                qDebug() << "YELLOW";
                break;

                case Bench::GREEN:
                qDebug() << "GREEN";
                break;
            }
//        lseries->append(bench_list[current_bench]->timestamp, bench_list[current_bench]->Spower_vec[j + i * POINTS_NUM]);
        }
    }
    graph->bench = bench_list[current_bench];

}

void Client::show_bench_info(int index)
{
    show_bench(bench_list[index]);
    this->current_bench = index;
    if(this->current_bench >= 0)
        ui->connect_btn->setEnabled(true);
}

void Client::show_bench(Bench *b)
{
    ui->lbl_merc_sn->setText(b->get_ser_num());
    ui->lbl_merc_ip->setText(b->get_ip());
}


Client::~Client()
{
    delete ui;
}
