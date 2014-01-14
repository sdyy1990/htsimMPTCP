#ifndef LOGGERTYPES_H
#define LOGGERTYPES_H

class Packet;
class TcpSrc;
class Queue;
class QcnReactor;
class QcnQueue;
class MultipathTcpSrc;

class Logged {
public:
    typedef uint32_t id_t;
    Logged(const string& name) {
        _name=name;
        id=LASTIDNUM;
        Logged::LASTIDNUM++;
    }
    virtual ~Logged() {}
    void setName(const string& name) {
        _name=name;
    }
    virtual const string& str() {
        return _name;
    };
    id_t id;
private:
    static id_t LASTIDNUM;
    string _name;
};

class TrafficLogger {
public:
    enum EventType { TRAFFIC_EVENT=3 };
    enum TrafficEvent { PKT_ARRIVE=0, PKT_DEPART=1, PKT_CREATESEND=2, PKT_DROP=3, PKT_RCVDESTROY=4 };
    virtual void logTraffic(Packet& pkt, Logged& location, TrafficEvent ev) =0;
    virtual ~TrafficLogger() {};
};

class QueueLogger {
public:
    enum QueueEvent { PKT_ENQUEUE=0, PKT_DROP=1, PKT_SERVICE=2 };
    enum QueueRecord { CUM_TRAFFIC=0 };
    enum QueueApprox { QUEUE_RANGE=0, QUEUE_OVERFLOW=1 };
    enum EventType { QUEUE_EVENT=0, QUEUE_RECORD=4, QUEUE_APPROX=5 };
    virtual void logQueue(Queue& queue, QueueEvent ev, Packet& pkt) =0;
    virtual ~QueueLogger() {};
};

class MultipathTcpLogger {
public:
    enum EventType {MTCP = 12};
    enum MultipathTcpEvent { CHANGE_A=0, RTT_UPDATE=1, WINDOW_UPDATE=2, RATE=3 };

    virtual void logMultipathTcp(MultipathTcpSrc &src, MultipathTcpEvent ev) =0;
    virtual ~MultipathTcpLogger() {};
};

class EnergyLogger {
public:
    enum EventType {ENERGY = 13};
    enum EnergyEvent { DRAW=0 };

    virtual ~EnergyLogger() {};
};

class TcpLogger {
public:
    enum EventType { TCP_EVENT=1, TCP_STATE=2, TCP_RECORD=6 , TCP_SINK = 11};
    enum TcpEvent { TCP_RCV=0, TCP_RCV_FR_END=1, TCP_RCV_FR=2, TCP_RCV_DUP_FR=3,
                    TCP_RCV_DUP=4, TCP_RCV_3DUPNOFR=5,
                    TCP_RCV_DUP_FASTXMIT=6, TCP_TIMEOUT=7
                  };
    enum TcpState { TCPSTATE_CNTRL=0, TCPSTATE_SEQ=1 };
    enum TcpRecord { AVE_CWND=0 };
    enum TcpSinkRecord { RATE = 0 };

    virtual void logTcp(TcpSrc &src, TcpEvent ev) =0;
    virtual ~TcpLogger() {};
};

class QcnLogger {
public:
    enum EventType { QCN_EVENT=7, QCNQUEUE_EVENT=8 };
    enum QcnEvent { QCN_SEND=0, QCN_INC=1, QCN_DEC=2, QCN_INCD=3, QCN_DECD=4 };
    enum QcnQueueEvent { QCN_FB=0, QCN_NOFB=1 };
    virtual void logQcn(QcnReactor &src, QcnEvent ev, double var3) =0;
    virtual void logQcnQueue(QcnQueue &src, QcnQueueEvent ev, double var1, double var2, double var3) =0;
    virtual ~QcnLogger() {};
};

#endif
