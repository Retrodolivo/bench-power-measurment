#include "bench.h"

Bench::Bench(QString ip, QString sn)
{
    this->ip = ip;
    this->ser_num = sn;
}

QString Bench::get_ip()
{
    return this->ip;
}

QString Bench::get_ser_num()
{
    return this->ser_num;
}

