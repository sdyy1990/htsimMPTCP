#include "queue.h"
#include <math.h>

Queue::Queue(linkspeed_bps bitrate, mem_b maxsize, EventList& eventlist, QueueLogger* logger)
    : EventSource(eventlist,"queue"),
      _maxsize(maxsize), _logger(logger), _bitrate(bitrate)
{
    _queuesize = 0;
    _ps_per_byte = (simtime_picosec)((pow(10.0,12.0) * 8) / _bitrate);
}


void
Queue::beginService()
{
    assert(!_enqueued.empty());
    eventlist().sourceIsPendingRel(*this, drainTime(_enqueued.back()));
}

void
Queue::completeService()
{
    assert(!_enqueued.empty());
    Packet* pkt = _enqueued.back();
    _enqueued.pop_back();
    _queuesize -= pkt->size();
    pkt->flow().logTraffic(*pkt,*this,TrafficLogger::PKT_DEPART);
    if (_logger) _logger->logQueue(*this, QueueLogger::PKT_SERVICE, *pkt);
    pkt->sendOn();
    if (!_enqueued.empty())
        beginService();
}

void
Queue::doNextEvent()
{
    completeService();
}


void
Queue::receivePacket(Packet& pkt)
{
    if (_queuesize+pkt.size() > _maxsize) {
        if (_logger) _logger->logQueue(*this, QueueLogger::PKT_DROP, pkt);
        pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_DROP);
        pkt.free();
        return;
    }
    pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_ARRIVE);
    bool queueWasEmpty = _enqueued.empty();
    _enqueued.push_front(&pkt);
    _queuesize += pkt.size();
    if (_logger) _logger->logQueue(*this, QueueLogger::PKT_ENQUEUE, pkt);
    if (queueWasEmpty) {
        assert(_enqueued.size()==1);
        beginService();
    }
}
