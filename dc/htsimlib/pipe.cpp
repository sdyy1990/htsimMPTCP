#include "pipe.h"
#include <iostream>
Pipe::Pipe(simtime_picosec delay, EventList& eventlist)
    : EventSource(eventlist,"pipe"),
      _delay(delay)
{}

void
Pipe::receivePacket(Packet& pkt)
{
    pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_ARRIVE);
    if (_inflight.empty()) {
        // no packets currently inflight; need to notify the eventlist we've an event pending
        eventlist().sourceIsPendingRel(*this,_delay);
        //cout<<"+";
    }
    _inflight.push_front(make_pair(eventlist().now()+_delay,&pkt));
    //  cout<<"RCV:"<<str()<<":"<<_inflight.size()<<endl;
}

void
Pipe::doNextEvent() {
    if (_inflight.size()==0) return;
    Packet *pkt = _inflight.back().second;
    _inflight.pop_back();
    pkt->flow().logTraffic(*pkt,*this,TrafficLogger::PKT_DEPART);
    pkt->sendOn();
    if (!_inflight.empty()) {
        // notify the eventlist we've another event pending
        simtime_picosec nexteventtime = _inflight.back().first;
        _eventlist.sourceIsPending(*this, nexteventtime);
        //    cout <<"+";
    }
    //  cout<<"SND:"<<str()<<":"<<_inflight.size()<<endl;
}
