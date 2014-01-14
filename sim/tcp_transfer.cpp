#include "tcp_transfer.h"
#include "mtcp.h"
#include "math.h"
#include <iostream>
#include "config.h"

////////////////////////////////////////////////////////////////
//  TCP PERIODIC SOURCE
////////////////////////////////////////////////////////////////

uint64_t generateFlowSize() {
    //  if (drand()>0.99)
    //return 8000000;// * (0.75 + drand()/2);
    //else
    return 70000;// * (0.5 + drand());
}

TcpSrcTransfer::TcpSrcTransfer(TcpLogger* logger, TrafficLogger* pktLogger, EventList &eventlist,uint64_t bytes_to_send, vector<route_t*>* p) : TcpSrc(logger,pktLogger,eventlist)
{
    _is_active = false;
    _ssthresh = 0xffffffff;
    _bytes_to_send = bytes_to_send;
    _paths = p;
}

void TcpSrcTransfer::reset(uint64_t bb, int shouldRestart) {
    _sawtooth = 0;
    _rtt_avg = timeFromMs(0);
    _rtt_cum = timeFromMs(0);
    _highest_sent = 0;
    _effcwnd = 0;
    _ssthresh = 0xffffffff;
    _last_acked = 0;
    _dupacks = 0;
    _mdev = 0;
    _recoverq = 0;
    _in_fast_recovery = false;

    _rtx_timeout_pending = false;
    _RFC2988_RTO_timeout = timeInf;

    _bytes_to_send = bb;

    if (shouldRestart)
        eventlist().sourceIsPendingRel(*this,_rtt*(1+drand()));
}



void
TcpSrcTransfer::connect(route_t& routeout, route_t& routeback, TcpSink& sink, simtime_picosec starttime)
{
    _is_active = false;

    TcpSrc::connect(routeout,routeback,sink,starttime);
}

void
TcpSrcTransfer::doNextEvent() {
    if (!_is_active) {
        _is_active = true;

        //delete _route;
        if (_paths!=NULL) {
            _route = new route_t(*(_paths->at(rand()%_paths->size())));
            _route->push_back(_sink);
        }

        //should reset route here!
        //how?
        ((TcpSinkTransfer*)_sink)->reset();

        _started = eventlist().now();
        startflow();
    }
    else TcpSrc::doNextEvent();
}

void
TcpSrcTransfer::receivePacket(Packet& pkt) {
    if (_is_active) {
        TcpSrc::receivePacket(pkt);

        if (_bytes_to_send>0) {
            if (!_mSrc && _last_acked>=_bytes_to_send) {
                _is_active = false;
                cout << endl << "Flow " << _bytes_to_send << " finished after " << timeAsMs(eventlist().now()-_started) << endl;
                reset(generateFlowSize(),0);
            }
            else if (_mSrc) {
                if (_last_acked >= _bytes_to_send/_mSrc->_subflows.size() && _mSrc->compute_total_bytes()>=_bytes_to_send) {
                    //log finish time

                    cout << endl << "Flow " << _bytes_to_send << " finished after " << timeAsMs(eventlist().now()-_started) << endl;

                    //reset all the subflows, including this one.
                    int bb = generateFlowSize();

                    list<TcpSrc*>::iterator it;
                    int subflows_to_activate = bb >= 1000000 ? 8:1;
                    int crt_subflow = 0;

                    for (it = _mSrc->_subflows.begin(); it!=_mSrc->_subflows.end(); it++) {
                        TcpSrc* t = (*it);
                        TcpSrcTransfer* crt = (TcpSrcTransfer*)t;
                        crt->_is_active = false;
                        crt->reset(bb,crt_subflow<subflows_to_activate);
                        crt_subflow++;
                    }
                }
            }
        }
    }
    else {
        pkt.flow().logTraffic(pkt,*this,TrafficLogger::PKT_RCVDESTROY);
        pkt.free();
    }
}

void TcpSrcTransfer::rtx_timer_hook(simtime_picosec now, simtime_picosec period) {
    if (!_is_active) return;

    if (now <= _RFC2988_RTO_timeout || _RFC2988_RTO_timeout==timeInf) return;

    if (_highest_sent == 0) return;

    cout << "Transfer timeout: active " << _is_active << " bytes to send " << _bytes_to_send << " sent " << _last_acked << endl;

    TcpSrc::rtx_timer_hook(now,period);
}

////////////////////////////////////////////////////////////////
//  Tcp Transfer SINK
////////////////////////////////////////////////////////////////

TcpSinkTransfer::TcpSinkTransfer() : TcpSink()
{
}

void TcpSinkTransfer::reset() {
    _cumulative_ack = 0;
    _received.clear();

    //queue logger sampling?
}
