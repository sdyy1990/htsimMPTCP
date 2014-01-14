#ifndef QUEUE_H
#define QUEUE_H

/*
 * A simple FIFO queue
 */

#include <list>
#include "config.h"
#include "eventlist.h"
#include "network.h"
#include "loggertypes.h"

class Queue : public EventSource, public PacketSink {
public:
    Queue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist, QueueLogger* logger);
    virtual void receivePacket(Packet& pkt);
    void doNextEvent();
// should really be private, but loggers want to see
    mem_b _maxsize;
    mem_b _queuesize;
    inline simtime_picosec drainTime(Packet *pkt) {
        return (simtime_picosec)(pkt->size()) * _ps_per_byte;
    }
    inline mem_b serviceCapacity(simtime_picosec t) {
        return (mem_b)(timeAsSec(t) * (double)_bitrate);
    }
protected:
    // Housekeeping
    QueueLogger* _logger;
    // Mechanism
    void beginService(); // start serving the item at the head of the queue
    void completeService(); // wrap up serving the item at the head of the queue
    linkspeed_bps _bitrate;
    simtime_picosec _ps_per_byte;  // service time, in picoseconds per byte
    list<Packet*> _enqueued;
};

#endif
