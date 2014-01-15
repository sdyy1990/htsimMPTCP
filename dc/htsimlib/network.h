#ifndef NETWORK_H
#define NETWORK_H

#include <vector>
#include "config.h"
#include "loggertypes.h"

class Packet;
class PacketFlow;
class PacketSink;
typedef vector<PacketSink*> route_t;
typedef vector<route_t*> routes_t;
typedef uint32_t packetid_t;

class DataReceiver {
public:
    DataReceiver() {};
    virtual ~DataReceiver() {};
    virtual uint64_t cumulative_ack()=0;
    virtual uint32_t get_id()=0;
    virtual uint32_t drops()=0;
};

// See tcppacket.h to illustrate how Packet is typically used.
class Packet {
    friend class PacketFlow;
public:
    Packet() {}; // empty constructor; Packet::set must always be called as well. It's a separate method, for convenient reuse.
    virtual void free() =0; // say "this packet is no longer wanted". (doesn't necessarily destroy it, so it can be reused)
    virtual void sendOn(); // "go on to the next hop along your route"
    int size() const {
        return _size;
    }
    PacketFlow& flow() const {
        return *_flow;
    }
    virtual ~Packet() {};
    inline const packetid_t id() const {
        return _id;
    }
protected:
    void set(PacketFlow& flow, route_t &route, int pkt_size, packetid_t id);
    int _size;
    route_t* _route;
    unsigned int _nexthop;
    packetid_t _id;
    PacketFlow* _flow;
};

class PacketFlow : public Logged {
    friend class Packet;
public:
    PacketFlow(TrafficLogger* logger);
    virtual ~PacketFlow() {};
    void logTraffic(Packet& pkt, Logged& location, TrafficLogger::TrafficEvent ev);
protected:
    TrafficLogger* _logger;
};

class PacketSink {
public:
    PacketSink() { }
    virtual ~PacketSink() {}
    virtual void receivePacket(Packet& pkt) =0;
};


// For speed, it may be useful to keep a database of all packets that
// have been allocated -- that way we don't need a malloc for every
// new packet, we can just reuse old packets. Care, though -- the set()
// method will need to be invoked properly for each new/reused packet

template<class P>
class PacketDB {
public:
    P* allocPacket() {
        if (_freelist.empty())
            return new P();
        else {
            P* p = _freelist.back();
            _freelist.pop_back();
            return p;
        }
    };
    void freePacket(P* pkt) {
        _freelist.push_back(pkt);
    };
protected:
    vector<P*> _freelist; // Irek says it's faster with vector than with list
};


#endif
