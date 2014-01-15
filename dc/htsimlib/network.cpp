#include "network.h"

void
Packet::set(PacketFlow& flow, route_t &route, int pkt_size, packetid_t id)
{
    _flow = &flow;
    _size = pkt_size;
    _id = id;
    _nexthop = 0;
    _route = &route;
}

void
Packet::sendOn()
{
    assert(_nexthop<_route->size());
    PacketSink* nextsink = (*_route)[_nexthop];
    _nexthop++;
    nextsink->receivePacket(*this);
}

PacketFlow::PacketFlow(TrafficLogger* logger)
    : Logged("PacketFlow"),
      _logger(logger)
{}

void
PacketFlow::logTraffic(Packet& pkt, Logged& location, TrafficLogger::TrafficEvent ev) {
    if (_logger)
        _logger->logTraffic(pkt, location, ev);
};

Logged::id_t Logged::LASTIDNUM = 1;
