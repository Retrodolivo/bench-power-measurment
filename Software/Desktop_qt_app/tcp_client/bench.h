#ifndef BENCH_H
#define BENCH_H

#include <QObject>
#include <QDateTime>
#include <QString>

class Bench : public QObject{

    Q_OBJECT

public:
    QString ser_num;
    QString ip;

    QByteArray raw_data = 0;
    uint32_t blocks_num = 0;
    uint32_t Spower_u32 = 0;
    float Spower_f;
    qreal timestamp = 0;
    QVector<qreal> Spower_vec;
    QVector<qreal>timestamp_vec;
    QVector<QDateTime> date_vec;
    QVector<uint8_t> status_vec;


    enum state { RED, YELLOW, GREEN};

    Bench(QString ip, QString sn);
    QString get_ip();
    QString get_ser_num();


};

#endif // BENCH_H

