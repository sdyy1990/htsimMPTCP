#ifndef TCP_H
#define TCP_H

/*
 * A TCP source and sink
 */

#include <list>
#include "config.h"
#include "network.h"
#include "tcppacket.h"
#include "eventlist.h"

#define timeInf 0
//#define PACKET_SCATTER 1

//#define MAX_SENT 10000

class TcpSink;
class MultipathTcpSrc;
class MultipathTcpSink;
class MultipathTcpSrcListener;
class TcpSrc : public PacketSink, public EventSource {
    friend class TcpSink;
public:
    TcpSrc(TcpLogger* logger, TrafficLogger* pktlogger, EventList &eventlist);
    virtual void connect(route_t& routeout, route_t& routeback, TcpSink& sink, simtime_picosec startTime);
    void startflow();
    void joinMultipathConnection(MultipathTcpSrc* multipathSrc);

    virtual void doNextEvent();
    virtual void receivePacket(Packet& pkt);

    void replace_route(route_t* newroute);

    uint32_t effective_window();
    virtual void rtx_timer_hook(simtime_picosec now,simtime_picosec period);
// should really be private, but loggers want to see:
    uint64_t _highest_sent;  //seqno is in bytes
    uint64_t _packets_sent;
    uint32_t _cwnd;
    uint32_t _maxcwnd;
    uint64_t _last_acked;
    uint32_t _ssthresh;
    uint16_t _dupacks;
#ifdef PACKET_SCATTER
    uint16_t DUPACK_TH;
    uint16_t _crt_path;
#endif

    int32_t _app_limited;

    //round trip time estimate, needed for coupled congestion control
    simtime_picosec _rtt, _rto, _mdev;
    simtime_picosec _rtt_avg, _rtt_cum;
    //simtime_picosec when[MAX_SENT];
    int _sawtooth;

    uint16_t _mss;
    uint32_t _unacked; // an estimate of the amount of unacked data WE WANT TO HAVE in the network
    uint32_t _effcwnd; // an estimate of our current transmission rate, expressed as a cwnd
    uint64_t _recoverq;
    bool _in_fast_recovery;

    uint32_t _drops;

    TcpSink* _sink;
    MultipathTcpSrc* _mSrc;
    MultipathTcpSrcListener* _mListener;
    simtime_picosec _RFC2988_RTO_timeout;
    bool _rtx_timeout_pending;

    void set_app_limit(int pktps);

    route_t* _route;
    simtime_picosec _last_ping;
    void set_max_packets(uint64_t mx);
    
#ifdef PACKET_SCATTER
    vector<route_t*>* _paths;

    void set_paths(vector<route_t*>* rt);
#endif
private:
    route_t* _old_route;
    uint64_t _last_packet_with_old_route;

    // Housekeeping
    TcpLogger* _logger;
    TrafficLogger* _pktlogger;
    // Connectivity
    PacketFlow _flow;

    // Mechanism
    void clear_timer(uint64_t start,uint64_t end);
    void inflate_window();
    void send_packets();
    void retransmit_packet();
    //simtime_picosec _last_sent_time;

    //	void clearWhen(TcpAck::seq_t from, TcpAck::seq_t to);
    //void showWhen (int from, int to);
    uint64_t _max_packets;
    bool _terminated;
};

class TcpSink : public PacketSink, public DataReceiver, public Logged {
    friend class TcpSrc;
public:
    TcpSink();
    void receivePacket(Packet& pkt);
    inline void joinMultipathConnection(MultipathTcpSink* sink) {
        _mSink = sink;
    };
    TcpAck::seq_t _cumulative_ack; // the packet we have cumulatively acked
    uint64_t _packets;
    uint32_t _drops;
    uint64_t cumulative_ack() {
        return _cumulative_ack + _received.size()*1000;
    }
    uint32_t drops() {
        return _src->_drops;
    }
    uint32_t get_id() {
        return id;
    }

    MultipathTcpSink* _mSink;
    list<TcpAck::seq_t> _received; // list of packets above a hole, that we've received

    TcpSrc* _src;
private:
    // Connectivity
    void connect(TcpSrc& src, route_t& route);
    route_t* _route;

    // Mechanism
    void send_ack(simtime_picosec ts);
};

class TcpRtxTimerScanner : public EventSource {
public:
    TcpRtxTimerScanner(simtime_picosec scanPeriod, EventList& eventlist);
    void doNextEvent();
    void registerTcp(TcpSrc &tcpsrc);
private:
    simtime_picosec _scanPeriod;
    typedef list<TcpSrc*> tcps_t;
    tcps_t _tcps;
};

#endif
