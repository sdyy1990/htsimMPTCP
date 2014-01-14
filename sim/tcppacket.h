#ifndef TCPPACKET_H
#define TCPPACKET_H

#include <list>
#include "network.h"

// TcpPacket and TcpAck are subclasses of Packet.
// They incorporate a packet database, to reuse packet objects that are no longer needed.
// Note: you never construct a new TcpPacket or TcpAck directly;
// rather you use the static method newpkt() which knows to reuse old packets from the database.

class TcpPacket : public Packet {
public:
    typedef uint64_t seq_t;

    inline static TcpPacket* newpkt(PacketFlow &flow, route_t &route,
                                    seq_t seqno, int size) {
        TcpPacket* p = _packetdb.allocPacket();
        p->set(flow,route,size,seqno+size-1); // The TCP sequence number is the first byte of the packet; I will ID the packet by its last byte.
        p->_seqno = seqno;
        return p;
    }

    void free() {
        _packetdb.freePacket(this);
    }
    virtual ~TcpPacket() {}
    inline seq_t seqno() const {
        return _seqno;
    }
    inline simtime_picosec ts() const {
        return _ts;
    }
    inline void set_ts(simtime_picosec ts) {
        _ts = ts;
    }
    const static uint64_t DEFAULTDATASIZE=1000; // size of a data packet, measured in bytes; used by TcpSrc
protected:
    seq_t _seqno;
    simtime_picosec _ts;
    static PacketDB<TcpPacket> _packetdb;
};

class TcpAck : public Packet {
public:
    typedef TcpPacket::seq_t seq_t;

    inline static TcpAck* newpkt(PacketFlow &flow, route_t &route,
                                 seq_t seqno, seq_t ackno) {
        TcpAck* p = _packetdb.allocPacket();
        p->set(flow,route,ACKSIZE,ackno);
        p->_seqno = seqno;
        p->_ackno = ackno;
        return p;
    }

    void free() {
        _packetdb.freePacket(this);
    }
    inline seq_t seqno() const {
        return _seqno;
    }
    inline seq_t ackno() const {
        return _ackno;
    }
    inline simtime_picosec ts() const {
        return _ts;
    }
    inline void set_ts(simtime_picosec ts) {
        _ts = ts;
    }

    virtual ~TcpAck() {}
    const static int ACKSIZE=40;
protected:
    seq_t _seqno;
    seq_t _ackno;
    simtime_picosec _ts;
    static PacketDB<TcpAck> _packetdb;
};

#endif
