#include "config.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include <math.h>
#include "network.h"
#include "randomqueue.h"
#include "pipe.h"
#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "clock.h"
#include "mtcp.h"
#include "exoqueue.h"

string ntoa(double n);
string itoa(uint64_t n);

// Simulation params
simtime_picosec RTT1=timeFromMs(50);
simtime_picosec RTT2=timeFromMs(250);
double targetwnd = 30;
int NUMFLOWS = 2;

#define TCP 1

linkspeed_bps SERVICE = speedFromPktps(500);//NUMFLOWS * targetwnd/timeAsSec(RTT1));

#define RANDOM_BUFFER 3

#define FEEDER_BUFFER 2000

mem_b BUFFER=memFromPkt(RANDOM_BUFFER+100);//NUMFLOWS * targetwnd);

void exit_error(char* progr) {
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED]" << endl;
    exit(1);
}

int main(int argc, char **argv) {
    EventList eventlist;
    eventlist.setEndtime(timeFromSec(1000));
    Clock c(timeFromSec(50/100.), eventlist);
    int algo = UNCOUPLED;
    int length = 3000;

    if (argc>1) {
        if (!strcmp(argv[1],"UNCOUPLED"))
            algo = UNCOUPLED;
        else if (!strcmp(argv[1],"COUPLED_INC"))
            algo = COUPLED_INC;
        else if (!strcmp(argv[1],"FULLY_COUPLED"))
            algo = FULLY_COUPLED;
        else if (!strcmp(argv[1],"COUPLED_TCP"))
            algo = COUPLED_TCP;
        else
            exit_error(argv[0]);
    }

    srand(time(NULL));

    // prepare the loggers
    stringstream filename(ios_base::out);
    filename << "../data/logout.dat";
    cout << "Outputting to " << filename.str() << endl;
    Logfile logfile(filename.str(),eventlist);

    logfile.setStartTime(timeFromSec(0.5));
    QueueLoggerSimple logQueue = QueueLoggerSimple();
    logfile.addLogger(logQueue);
    QueueLoggerSimple logPQueue = QueueLoggerSimple();
    logfile.addLogger(logPQueue);
    MultipathTcpLoggerSimple mlogger = MultipathTcpLoggerSimple();
    logfile.addLogger(mlogger);

    SinkLoggerSampling sinkLogger = SinkLoggerSampling(timeFromMs(1000),eventlist);

    logfile.addLogger(sinkLogger);


    QueueLoggerSampling qs = QueueLoggerSampling(timeFromMs(1000),eventlist);
    logfile.addLogger(qs);

    TcpLoggerSimple logTcp;
    logfile.addLogger(logTcp);

    // Build the network
    Pipe pipe1(RTT1, eventlist);
    pipe1.setName("pipe1");
    logfile.writeName(pipe1);
    Pipe pipe2(RTT2, eventlist);
    pipe2.setName("pipe2");
    logfile.writeName(pipe2);

    RandomQueue queue(SERVICE, BUFFER, eventlist,&qs,memFromPkt(RANDOM_BUFFER));
    queue.setName("Queue");
    logfile.writeName(queue);

    Queue pqueue3(SERVICE*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
    pqueue3.setName("PQueue3");
    logfile.writeName(pqueue3);
    Queue pqueue4(SERVICE*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
    pqueue4.setName("PQueue4");
    logfile.writeName(pqueue4);

    Queue* pqueue;

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(10), eventlist);

    //TCP flows on path 1
    TcpSrc* tcpSrc;
    TcpSink* tcpSnk;
    route_t* routeout;
    route_t* routein;
    double extrastarttime;

    for (int i=0; i<TCP; i++) {
        tcpSrc = new TcpSrc(&logTcp,NULL,eventlist);
        tcpSrc->setName("Tcp1");
        logfile.writeName(*tcpSrc);
        tcpSnk = new TcpSink();
        tcpSnk->setName("Tcp1");
        logfile.writeName(*tcpSnk);
        sinkLogger.monitorSink(tcpSnk);

        tcpRtxScanner.registerTcp(*tcpSrc);

        // tell it the route
        pqueue = new Queue(SERVICE*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
        pqueue->setName("PQueue1_"+ntoa(i));
        logfile.writeName(*pqueue);

        routeout = new route_t();
        routeout->push_back(pqueue);
        routeout->push_back(&queue);
        routeout->push_back(&pipe1);
        routeout->push_back(tcpSnk);

        routein  = new route_t();
        routein->push_back(tcpSrc);

        extrastarttime = drand()*50;
        tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromMs(extrastarttime));
    }

    MultipathTcpSrc mtcp(algo,eventlist,&mlogger);

    //MTCP flow 1
    tcpSrc = new TcpSrc(&logTcp,NULL,eventlist);
    tcpSrc->setName("Subflow1");
    logfile.writeName(*tcpSrc);
    tcpSnk = new TcpSink();
    tcpSnk->setName("Subflow1");
    logfile.writeName(*tcpSnk);

    sinkLogger.monitorSink(tcpSnk);
    tcpRtxScanner.registerTcp(*tcpSrc);

    // tell it the route
    routeout = new route_t();
    routeout->push_back(&pqueue3);
    routeout->push_back(&queue);
    routeout->push_back(&pipe1);
    routeout->push_back(tcpSnk);

    routein  = new route_t();
    routein->push_back(tcpSrc);
    extrastarttime = 50*drand();

    //join multipath connection
    mtcp.addSubflow(tcpSrc);

    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromMs(extrastarttime));

    //MTCP flow 2
    tcpSrc = new TcpSrc(&logTcp,NULL,eventlist);
    tcpSrc->setName("Subflow2");
    logfile.writeName(*tcpSrc);
    tcpSnk = new TcpSink();
    tcpSnk->setName("Subflow2");
    logfile.writeName(*tcpSnk);
    sinkLogger.monitorSink(tcpSnk);
    tcpRtxScanner.registerTcp(*tcpSrc);

    // tell it the route
    routeout = new route_t();
    routeout->push_back(&pqueue4);
    routeout->push_back(&pipe2);
    routeout->push_back(&queue);
    routeout->push_back(&pipe1);
    routeout->push_back(tcpSnk);

    routein  = new route_t();
    routein->push_back(tcpSrc);
    extrastarttime = 50*drand();

    //join multipath connection
    mtcp.addSubflow(tcpSrc);

    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromMs(extrastarttime));

    // Record the setup
    int pktsize = TcpPacket::DEFAULTDATASIZE;
    logfile.write("# pktsize="+ntoa(pktsize)+" bytes");
    logfile.write("# bottleneckrate="+ntoa(speedAsPktps(SERVICE))+" pkt/sec");

    logfile.write("# buffer="+ntoa((double)(queue._maxsize)/((double)pktsize))+" pkt");
    double rtt = timeAsSec(RTT1);
    logfile.write("# rtt="+ntoa(rtt));
    rtt = timeAsSec(RTT2+RTT1);
    logfile.write("# rtt="+ntoa(rtt));

    // GO!
    while (eventlist.doNextEvent()) {}
}

string ntoa(double n) {
    stringstream s;
    s << n;
    return s.str();
}
string itoa(uint64_t n) {
    stringstream s;
    s << n;
    return s.str();
}
