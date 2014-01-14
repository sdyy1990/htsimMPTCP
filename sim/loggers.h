#ifndef LOGGERS_H
#define LOGGERS_H

#include "logfile.h"
#include "network.h"
#include "eventlist.h"
#include "loggertypes.h"
#include "queue.h"
#include "tcp.h"
#include "mtcp.h"
#include "qcn.h"
#include <list>
#include <map>

class QueueLoggerSimple : public Logger, public QueueLogger {
public:
    void logQueue(Queue& queue, QueueLogger::QueueEvent ev, Packet& pkt) {
        _logfile->writeRecord(QueueLogger::QUEUE_EVENT,queue.id,ev,(double)queue._queuesize,pkt.flow().id,pkt.id());
    }
};

class TrafficLoggerSimple : public Logger, public TrafficLogger {
public:
    void logTraffic(Packet& pkt, Logged& location, TrafficEvent ev) {
        _logfile->writeRecord(TrafficLogger::TRAFFIC_EVENT,location.id,ev,pkt.flow().id,pkt.id(),0);
    }
};

class TcpLoggerSimple : public Logger, public TcpLogger {
public:
    void logTcp(TcpSrc &tcp, TcpEvent ev) {
        _logfile->writeRecord(TcpLogger::TCP_EVENT,tcp.id,ev,tcp._cwnd,tcp._unacked,tcp._in_fast_recovery?tcp._ssthresh:tcp._cwnd);//_effcwnd);
        _logfile->writeRecord(TcpLogger::TCP_STATE,tcp.id,TcpLogger::TCPSTATE_CNTRL,tcp._cwnd,tcp._ssthresh,tcp._recoverq);

        _logfile->writeRecord(TcpLogger::TCP_STATE,tcp.id,TcpLogger::TCPSTATE_SEQ,tcp._last_acked,tcp._highest_sent,tcp._RFC2988_RTO_timeout);
        //tcp._dupacks);
    }
};

class MultipathTcpLoggerSimple: public Logger, public MultipathTcpLogger {
public:
    void logMultipathTcp(MultipathTcpSrc& mtcp, MultipathTcpEvent ev) {
        if (ev==MultipathTcpLogger::CHANGE_A)
            _logfile->writeRecord(MultipathTcpLogger::MTCP,mtcp.id,ev,mtcp.a,mtcp._alfa,0);
        else if (ev==MultipathTcpLogger::RTT_UPDATE)
            _logfile->writeRecord(MultipathTcpLogger::MTCP,mtcp.id,ev,mtcp._subflows.front()->_rtt/1000000000,mtcp._subflows.back()->_rtt/1000000000,mtcp._subflows.front()->_mdev/1000000000);
        else if (ev==MultipathTcpLogger::WINDOW_UPDATE)
            _logfile->writeRecord(MultipathTcpLogger::MTCP,mtcp.id,ev,mtcp._subflows.front()->effective_window(),mtcp._subflows.back()->effective_window(),0);
    }
};

class QueueLoggerSampling : public Logger, public QueueLogger, public EventSource {
public:
    QueueLoggerSampling(simtime_picosec period, EventList& eventlist);
    void logQueue(Queue& queue, QueueEvent ev, Packet& pkt);
    void doNextEvent();
private:
    Queue* _queue;
    simtime_picosec _lastlook;
    simtime_picosec _period;
    mem_b _lastq;
    bool _seenQueueInD;
    mem_b _minQueueInD;
    mem_b _maxQueueInD;
    mem_b _lastDroppedInD;
    mem_b _lastIdledInD;
    int _numIdledInD;
    int _numDropsInD;
    double _cumidle;
    double _cumarr;
    double _cumdrop;
};

class SinkLoggerSampling : public Logger, public EventSource {
public:
    SinkLoggerSampling(simtime_picosec period, EventList& eventlist);
    void doNextEvent();
    void monitorSink(DataReceiver* sink);
private:
    vector<DataReceiver*> _sinks;
    vector<uint64_t> _last_seq;
    vector<double> _last_rate;

    struct lttime
    {
        bool operator()(const MultipathTcpSrc* i1, const MultipathTcpSrc* i2) const
        {
            return i1->id<i2->id;
        }
    };
    typedef map<MultipathTcpSrc*,double,lttime> multipath_map;
    multipath_map _multipath_src;

    simtime_picosec _last_time;
    simtime_picosec _period;
};

class AggregateTcpLogger : public Logger, public EventSource {
public:
    AggregateTcpLogger(simtime_picosec period, EventList& eventlist);
    void doNextEvent();
    void monitorTcp(TcpSrc& tcp);
private:
    simtime_picosec _period;
    typedef vector<TcpSrc*> tcplist_t;
    tcplist_t _monitoredTcps;
};

class QcnLoggerSimple : public Logger, public QcnLogger {
public:
    void logQcn(QcnReactor &src, QcnEvent ev, double var3) {
        if (ev!=QcnLogger::QCN_SEND)
            _logfile->writeRecord(QcnLogger::QCN_EVENT,src.id,ev,src._currentRate,src._packetCycles,var3);
    };
    void logQcnQueue(QcnQueue &src, QcnQueueEvent ev, double var1, double var2, double var3) {
        _logfile->writeRecord(QcnLogger::QCNQUEUE_EVENT,src.id,ev,var1,var2,var3);
    };
};



#endif
