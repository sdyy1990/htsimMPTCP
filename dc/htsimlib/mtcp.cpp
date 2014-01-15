#include "mtcp.h"
#include <math.h>
#include <iostream>
////////////////////////////////////////////////////////////////
//  Multipath TCP SOURCE
////////////////////////////////////////////////////////////////

#define ABS(x) ((x)>=0?(x):-(x))
#define A_SCALE 512

MultipathTcpSrc::MultipathTcpSrc(char cc_type,EventList& ev,MultipathTcpLogger* logger, double epsilon):
    EventSource(ev,"MTCP"),_alfa(1),_logger(logger), _e(epsilon)
{
    _cc_type = cc_type;
    a = A_SCALE;
    _subflowcnt = 0;
}

void MultipathTcpSrc::addSubflow(TcpSrc* subflow) {
    _subflows.push_back(subflow);
    subflow->joinMultipathConnection(this);
    _subflowcnt += 1;
}

void
MultipathTcpSrc::receivePacket(Packet& pkt) {
}

uint32_t
MultipathTcpSrc::inflate_window(uint32_t cwnd, int newly_acked,uint32_t mss) {
    int tcp_inc = (newly_acked * mss)/cwnd;
    int tt = (newly_acked * mss) % cwnd;
    int tmp, total_cwnd, tmp2,subs;
    double tmp_float;

    if (tcp_inc==0)
        return cwnd;

    switch(_cc_type) {

    case UNCOUPLED:
        if ((cwnd + tcp_inc)/mss != cwnd/mss) {
            if (_logger) {
                _logger->logMultipathTcp(*this,MultipathTcpLogger::WINDOW_UPDATE);
                _logger->logMultipathTcp(*this,MultipathTcpLogger::RTT_UPDATE);
            }
        }
        return cwnd + tcp_inc;

    case COUPLED_SCALABLE_TCP:
        return cwnd + newly_acked*0.01;

    case FULLY_COUPLED:
        total_cwnd = compute_total_window();
        tt = (int)(newly_acked * mss * A);
        tmp = tt/total_cwnd;
        if (tmp>tcp_inc)
            tmp = tcp_inc;

        //if ((rand()%total_cwnd) < tt % total_cwnd)
        //tmp ++;

        if ((cwnd + tmp)/mss != cwnd/mss) {
            if (_logger) {
                _logger->logMultipathTcp(*this,MultipathTcpLogger::WINDOW_UPDATE);
                _logger->logMultipathTcp(*this,MultipathTcpLogger::RTT_UPDATE);
            }
        }

        return cwnd + tmp;

    case COUPLED_INC:
        total_cwnd = compute_total_window();
        tmp2 = (newly_acked * mss * a) / total_cwnd;

        tmp = tmp2 / A_SCALE;

        if (tmp < 0) {
            printf("Negative increase!");
            tmp = 0;
        }

        if (rand() % A_SCALE < tmp2 % A_SCALE)
            tmp++;

        if (tmp>tcp_inc)//capping
            tmp = tcp_inc;

        if ((cwnd + tmp)/mss != cwnd/mss) {
            a = compute_a_scaled();
            if (_logger) {
                _logger->logMultipathTcp(*this,MultipathTcpLogger::WINDOW_UPDATE);
                _logger->logMultipathTcp(*this,MultipathTcpLogger::RTT_UPDATE);
                _logger->logMultipathTcp(*this,MultipathTcpLogger::CHANGE_A);
            }
        }

        return cwnd + tmp;

    case COUPLED_TCP:
        //need to update this to the number of subflows (wtf man!)
        subs = _subflows.size();
        subs *= subs;
        tmp = tcp_inc/subs;
        if (tcp_inc%subs>=subs/2)
            tmp++;

        if ((cwnd + tmp)/mss != cwnd/mss) {
            //a = compute_a_tcp();
            if (_logger) {
                _logger->logMultipathTcp(*this,MultipathTcpLogger::WINDOW_UPDATE);
                _logger->logMultipathTcp(*this,MultipathTcpLogger::RTT_UPDATE);
                //_logger->logMultipathTcp(*this,MultipathTcpLogger::CHANGE_A);
            }
        }
        return cwnd + tmp;

    case COUPLED_EPSILON:
        total_cwnd = compute_total_window();
        tmp_float = ((double)newly_acked * mss * _alfa * pow(_alfa*cwnd,1-_e))/pow(total_cwnd,2-_e);

        tmp = (int)floor(tmp_float);
        if (drand()< tmp_float-tmp)
            tmp++;

        if (tmp>tcp_inc)//capping
            tmp = tcp_inc;

        if ((cwnd + tmp)/mss != cwnd/mss) {
            if (_e>0&&_e<2)
                _alfa = compute_alfa();

            if (_logger) {
                _logger->logMultipathTcp(*this,MultipathTcpLogger::WINDOW_UPDATE);
                _logger->logMultipathTcp(*this,MultipathTcpLogger::RTT_UPDATE);
                _logger->logMultipathTcp(*this,MultipathTcpLogger::CHANGE_A);
            }
        }
        return cwnd + tmp;


    default:
        printf("Unknown cc type %d\n",_cc_type);
        exit(1);
    }
}

void MultipathTcpSrc::window_changed() {

    switch(_cc_type) {
    case COUPLED_EPSILON:
        if (_e>0&&_e<2)
            _alfa = compute_alfa();

        if (_logger) {
            _logger->logMultipathTcp(*this,MultipathTcpLogger::WINDOW_UPDATE);
            _logger->logMultipathTcp(*this,MultipathTcpLogger::RTT_UPDATE);
            _logger->logMultipathTcp(*this,MultipathTcpLogger::CHANGE_A);
        }
        return;
    case COUPLED_INC:
        a = compute_a_scaled();
        if (_logger) {
            _logger->logMultipathTcp(*this,MultipathTcpLogger::WINDOW_UPDATE);
            _logger->logMultipathTcp(*this,MultipathTcpLogger::RTT_UPDATE);
            _logger->logMultipathTcp(*this,MultipathTcpLogger::CHANGE_A);
        }
        return;
    }
}

uint32_t
MultipathTcpSrc::deflate_window(uint32_t cwnd, uint32_t mss) {
    int d;
    uint32_t decrease_tcp = max(cwnd/2,(uint32_t)(mss));
    switch(_cc_type) {
    case UNCOUPLED:
    case COUPLED_INC:
    case COUPLED_TCP:
    case COUPLED_EPSILON:
        return decrease_tcp;

    case COUPLED_SCALABLE_TCP:
        d = (int)cwnd - (compute_total_window()>>3);
        if (d<0) d = 0;

        //cout << "Dropping to " << max(mss,(uint32_t)d) << endl;
        return max(mss, (uint32_t)d);

    case FULLY_COUPLED:
        d = (int)cwnd - compute_total_window()/B;
        if (d<0) d = 0;

        //cout << "Dropping to " << max(mss,(uint32_t)d) << endl;
        return max(mss, (uint32_t)d);

    default:
        printf("Unknown cc type %d\n",_cc_type);
        exit(1);
    }
}

uint32_t
MultipathTcpSrc::compute_total_window() {
    list<TcpSrc*>::iterator it;
    uint32_t crt_wnd = 0;

    //  printf ("Tot wnd ");
    for (it = _subflows.begin(); it!=_subflows.end(); it++) {
        TcpSrc& crt = *(*it);
        crt_wnd += crt._in_fast_recovery?crt._ssthresh:crt._cwnd;
    }

    return crt_wnd;
}


uint64_t
MultipathTcpSrc::compute_total_bytes() {
    list<TcpSrc*>::iterator it;
    uint64_t b = 0;

    for (it = _subflows.begin(); it!=_subflows.end(); it++) {
        TcpSrc& crt = *(*it);
        b += crt._last_acked;
    }

    return b;
}


uint32_t
MultipathTcpSrc::compute_a_tcp() {
    if (_cc_type!=COUPLED_TCP)
        return 0;

    if (_subflows.size()!=2) {
        cout << "Expecting 2 subflows, found" << _subflows.size() << endl;
        exit(1);
    }

    TcpSrc* flow0 = _subflows.front();
    TcpSrc* flow1 = _subflows.back();

    uint32_t cwnd0 = flow0->_in_fast_recovery?flow0->_ssthresh:flow0->_cwnd;
    uint32_t cwnd1 = flow1->_in_fast_recovery?flow1->_ssthresh:flow1->_cwnd;

    double pdelta = (double)cwnd1/cwnd0;
    double rdelta;

#if USE_AVG_RTT
    if (flow0->_rtt_avg>timeFromMs(0)&&flow1->_rtt_avg>timeFromMs(1))
        rdelta = (double)flow0->_rtt_avg / flow1->_rtt_avg;
    else
        rdelta = 1;

#else
    rdelta = (double)flow0->_rtt / flow1->_rtt;

#endif


    if (1 < pdelta * rdelta * rdelta) {
#if USE_AVG_RTT
        if (flow0->_rtt_avg>timeFromMs(0)&&flow1->_rtt_avg>timeFromMs(1))
            rdelta = (double)flow1->_rtt_avg / flow0->_rtt_avg;
        else
            rdelta = 1;
#else
        rdelta = (double)flow1->_rtt / flow0->_rtt;
#endif

        pdelta = (double)cwnd0/cwnd1;
    }

    double t = 1.0+rdelta*sqrt(pdelta);

    return (uint32_t)(A_SCALE/t/t);
}


uint32_t MultipathTcpSrc::compute_a_scaled() {
    if (_cc_type!=COUPLED_INC && _cc_type!=COUPLED_TCP)
        return 0;

    uint32_t sum_denominator=0;
    uint64_t t = 0;
    uint64_t cwndSum = 0;

    list<TcpSrc*>::iterator it;
    for(it=_subflows.begin(); it!=_subflows.end(); ++it) {
        TcpSrc& flow = *(*it);
        uint32_t cwnd = flow._in_fast_recovery?flow._ssthresh:flow._cwnd;
        uint32_t rtt = timeAsUs(flow._rtt)/10;
        if(rtt==0) rtt=1;

        t = max(t,(uint64_t)cwnd * flow._mss * flow._mss / rtt / rtt);
        sum_denominator += cwnd * flow._mss / rtt;
        cwndSum += cwnd;
    }
    return (uint32_t)( A_SCALE * (uint64_t)cwndSum * t / sum_denominator / sum_denominator);
}


double MultipathTcpSrc::compute_alfa() {
    if (_cc_type!=COUPLED_EPSILON)
        return 0;

    if (_subflows.size()==1) {
        return 1;
    }
    else {
        double maxt = 0,sum_denominator = 0;

        list<TcpSrc*>::iterator it;
        for(it=_subflows.begin(); it!=_subflows.end(); ++it) {
            TcpSrc& flow = *(*it);
            uint32_t cwnd = flow._in_fast_recovery?flow._ssthresh:flow._cwnd;
            uint32_t rtt = timeAsMs(flow._rtt);

            if (rtt==0)
                rtt = 1;

            double t = pow(cwnd,_e/2)/rtt;
            if (t>maxt)
                maxt = t;

            sum_denominator += ((double)cwnd/rtt);
        }

        return (double)compute_total_window() * pow(maxt, 1/(1-_e/2)) / pow(sum_denominator, 1/(1-_e/2));
    }
}


double
MultipathTcpSrc::compute_a() {
    if (_cc_type!=COUPLED_INC && _cc_type!=COUPLED_TCP)
        return -1;

    if (_subflows.size()!=2) {
        cout << "Expecting 2 subflows, found" << _subflows.size() << endl;
        exit(1);
    }

    double a;

    TcpSrc* flow0 = _subflows.front();
    TcpSrc* flow1 = _subflows.back();

    uint32_t cwnd0 = flow0->_in_fast_recovery?flow0->_ssthresh:flow0->_cwnd;
    uint32_t cwnd1 = flow1->_in_fast_recovery?flow1->_ssthresh:flow1->_cwnd;

    double pdelta = (double)cwnd1/cwnd0;
    double rdelta;

#if USE_AVG_RTT
    if (flow0->_rtt_avg>timeFromMs(0)&&flow1->_rtt_avg>timeFromMs(1))
        rdelta = (double)flow0->_rtt_avg / flow1->_rtt_avg;
    else
        rdelta = 1;

    //cout << "R1 " << flow0->_rtt_avg/1000000000 << " R2 " << flow1->_rtt_avg/1000000000 << endl;
#else
    rdelta = (double)flow0->_rtt / flow1->_rtt;

    //cout << "R1 " << flow0->_rtt/1000000000 << " R2 " << flow1->_rtt/1000000000 << endl;
#endif


    //cout << "R1 " << flow0->_rtt_avg/1000000000 << " R2 " << flow1->_rtt_avg/1000000000 << endl;

    //second_better
    if (1 < pdelta * rdelta * rdelta) {
#if USE_AVG_RTT
        if (flow0->_rtt_avg>timeFromMs(0)&&flow1->_rtt_avg>timeFromMs(1))
            rdelta = (double)flow1->_rtt_avg / flow0->_rtt_avg;
        else
            rdelta = 1;
#else
        rdelta = (double)flow1->_rtt / flow0->_rtt;
#endif

        pdelta = (double)cwnd0/cwnd1;
    }

    if (_cc_type==COUPLED_INC) {
        a = (1+pdelta)/(1+pdelta*rdelta)/(1+pdelta*rdelta);
        //cout << "A\t" << a << endl;


        if (a<0.5) {
            cout << " a comp error " << a << ";resetting to 0.5" <<endl;
            a = 0.5;
        }

        //a = compute_a2();
        ///if (ABS(a-compute_a2())>0.01)
        //cout << "Compute a error! " <<a << " vs " << compute_a2()<<endl;
    }
    else {
        double t = 1.0+rdelta*sqrt(pdelta);
        a = 1.0/t/t;
    }

    return a;
}

void MultipathTcpSrc::doNextEvent() {}


////////////////////////////////////////////////////////////////
//  MTCP SINK
////////////////////////////////////////////////////////////////

MultipathTcpSink::MultipathTcpSink()
{}

void MultipathTcpSink::addSubflow(TcpSink* sink) {
    _subflows.push_back(sink);
    sink->joinMultipathConnection(this);
}

void
MultipathTcpSink::receivePacket(Packet& pkt) {
}


MultipathTcpSrcListener::MultipathTcpSrcListener() {
	finished.clear();
	_total_finished = 0;
}
void MultipathTcpSrcListener::notify_finished(TcpSrc *tcp) {
	MultipathTcpSrc * mtcp;
	mtcp = tcp-> _mSrc;
	finished[mtcp] ++;
	if (finished[mtcp] == mtcp->_subflowcnt)
		_total_finished++;
}
