#include "config.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include <math.h>
#include "network.h"
#include "queue.h"
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
simtime_picosec RTT1=timeFromMs(100);
simtime_picosec RTT2=timeFromMs(100);
double targetwnd = 30;
int NUMFLOWS = 2;
linkspeed_bps SERVICE1 = speedFromPktps(NUMFLOWS * targetwnd/timeAsSec(RTT1));
linkspeed_bps SERVICE2 = speedFromPktps(NUMFLOWS * targetwnd/timeAsSec(RTT2));
mem_b BUFFER1=memFromPkt(NUMFLOWS * targetwnd);
mem_b BUFFER2=memFromPkt(NUMFLOWS * targetwnd);

int main(int argc, char **argv) {
    EventList eventlist;
    eventlist.setEndtime(timeFromSec(500));
    Clock c(timeFromSec(50/100.), eventlist);

    // prepare the loggers
    stringstream filename(ios_base::out);
    filename << "../data/logout.dat";
    cout << "Outputting to " << filename.str() << endl;
    Logfile logfile(filename.str(),eventlist);

    logfile.setStartTime(timeFromSec(0.5));
    QueueLoggerSimple logQueue1 = QueueLoggerSimple();
    logfile.addLogger(logQueue1);
    QueueLoggerSimple logQueue2 = QueueLoggerSimple();
    logfile.addLogger(logQueue2);

    //TrafficLoggerSimple logger;
    //logfile.addLogger(logger);
    SinkLoggerSampling sinkLogger = SinkLoggerSampling(timeFromMs(1000),eventlist);

    logfile.addLogger(sinkLogger);

    //TcpLoggerSimple logTcp1, logTcp2, logMTcp1, logMTcp2;
    //logfile.addLogger(logTcp1);
    //logfile.addLogger(logTcp2);
    //logfile.addLogger(logMTcp1);
    //logfile.addLogger(logMTcp2);

    // Build the network
    Pipe pipe1(RTT1, eventlist);
    pipe1.setName("pipe1");
    logfile.writeName(pipe1);
    Pipe pipe2(RTT2, eventlist);
    pipe2.setName("pipe2");
    logfile.writeName(pipe2);
    Pipe pipe_back(timeFromMs(.1), eventlist);
    pipe_back.setName("pipe_back");
    logfile.writeName(pipe_back);

    Queue queue1(SERVICE1, BUFFER1, eventlist,&logQueue1);
    queue1.setName("Queue1");
    logfile.writeName(queue1);
    Queue queue2(SERVICE2, BUFFER2, eventlist,&logQueue2);
    queue2.setName("Queue2");
    logfile.writeName(queue2);

    ExoQueue exo1 = ExoQueue(0.001);
    ExoQueue exo2 = ExoQueue(0.001);

    Queue queue_back(max(SERVICE1,SERVICE2)*4, memFromPkt(1000), eventlist,NULL);
    queue_back.setName("queue_back");
    logfile.writeName(queue_back);

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(20), eventlist);

    //TCP flow 1
    TcpSrc* tcpSrc = new TcpSrc(NULL,NULL,eventlist);
    tcpSrc->setName("tcp1");
    logfile.writeName(*tcpSrc);
    TcpSink* tcpSnk = new TcpSink();
    tcpSnk->setName("tcpsink1");
    logfile.writeName(*tcpSnk);
    sinkLogger.monitorSink(tcpSnk);

    tcpRtxScanner.registerTcp(*tcpSrc);

    // tell it the route
    route_t* routeout = new route_t();
    routeout->push_back(&exo1);
    routeout->push_back(&pipe1);
    routeout->push_back(tcpSnk);

    route_t* routein  = new route_t();
    //routein->push_back(&queue_back); routein->push_back(&pipe_back);
    routein->push_back(tcpSrc);

    double extrastarttime = drand();
    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromSec(extrastarttime));

    //TCP flow 2
    tcpSrc = new TcpSrc(NULL,NULL,eventlist);
    tcpSrc->setName("tcp2");
    logfile.writeName(*tcpSrc);
    tcpSnk = new TcpSink();
    tcpSnk->setName("tcpsink2");
    logfile.writeName(*tcpSnk);

    sinkLogger.monitorSink(tcpSnk);

    tcpRtxScanner.registerTcp(*tcpSrc);

    // tell it the route
    routeout = new route_t();
    routeout->push_back(&exo2);
    routeout->push_back(&pipe2);
    routeout->push_back(tcpSnk);

    routein  = new route_t(); //routein->push_back(&queue_back); routein->push_back(&pipe_back);
    routein->push_back(tcpSrc);
    extrastarttime = 5*drand();
    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromSec(extrastarttime));

    MultipathTcpSrc mtcp(COUPLED_INC);

    //MTCP flow 1
    tcpSrc = new TcpSrc(NULL,NULL,eventlist);
    tcpSrc->setName("mtcp1");
    logfile.writeName(*tcpSrc);
    tcpSnk = new TcpSink();
    tcpSnk->setName("mtcpsink1");
    logfile.writeName(*tcpSnk);

    sinkLogger.monitorSink(tcpSnk);
    tcpRtxScanner.registerTcp(*tcpSrc);

    // tell it the route
    routeout = new route_t();
    routeout->push_back(&exo1);
    routeout->push_back(&pipe1);
    routeout->push_back(tcpSnk);

    routein  = new route_t();
    //routein->push_back(&queue_back); routein->push_back(&pipe_back);
    routein->push_back(tcpSrc);
    extrastarttime = 5*drand();

    //join multipath connection
    mtcp.addSubflow(tcpSrc);

    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromSec(extrastarttime));

    //MTCP flow 2
    tcpSrc = new TcpSrc(NULL,NULL,eventlist);
    tcpSrc->setName("mtcp2");
    logfile.writeName(*tcpSrc);
    tcpSnk = new TcpSink();
    tcpSnk->setName("mtcpsink2");
    logfile.writeName(*tcpSnk);
    sinkLogger.monitorSink(tcpSnk);
    tcpRtxScanner.registerTcp(*tcpSrc);

    // tell it the route
    routeout = new route_t();
    routeout->push_back(&exo2);
    routeout->push_back(&pipe2);
    routeout->push_back(tcpSnk);

    routein  = new route_t();
    //routein->push_back(&queue_back); routein->push_back(&pipe_back);
    routein->push_back(tcpSrc);
    extrastarttime = 5*drand();

    //join multipath connection
    mtcp.addSubflow(tcpSrc);

    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromSec(extrastarttime));


    // Record the setup
    int pktsize = TcpPacket::DEFAULTDATASIZE;
    logfile.write("# pktsize="+ntoa(pktsize)+" bytes");
    logfile.write("# bottleneckrate1="+ntoa(speedAsPktps(SERVICE1))+" pkt/sec");
    logfile.write("# bottleneckrate2="+ntoa(speedAsPktps(SERVICE2))+" pkt/sec");
    logfile.write("# buffer1="+ntoa((double)(queue1._maxsize)/((double)pktsize))+" pkt");
    logfile.write("# buffer2="+ntoa((double)(queue2._maxsize)/((double)pktsize))+" pkt");
    double rtt = timeAsSec(RTT1);
    logfile.write("# rtt="+ntoa(rtt));
    rtt = timeAsSec(RTT2);
    logfile.write("# rtt="+ntoa(rtt));
    logfile.write("# numflows="+ntoa(NUMFLOWS));
    logfile.write("# targetwnd="+ntoa(targetwnd));

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
