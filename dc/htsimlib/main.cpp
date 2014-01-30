#include "config.h"
#include <sstream>
#include <string.h>
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
#include "cbr.h"

string ntoa(double n);
string itoa(uint64_t n);

// Simulation params
simtime_picosec RTT1=timeFromMs(1);
simtime_picosec RTT2=timeFromMs(1);
double targetwnd = 30;
int NUMFLOWS = 2;

#define TCP_1 1
#define TCP_2 1

linkspeed_bps SERVICE1 = speedFromPktps(8000);//NUMFLOWS * targetwnd/timeAsSec(RTT1));
linkspeed_bps SERVICE2 = speedFromPktps(8000);//NUMFLOWS * targetwnd/timeAsSec(RTT2));

#define RANDOM_BUFFER 3

#define FEEDER_BUFFER 2000

mem_b BUFFER1=memFromPkt(RANDOM_BUFFER+30);//NUMFLOWS * targetwnd);
mem_b BUFFER2=memFromPkt(RANDOM_BUFFER+30);//NUMFLOWS * targetwnd);

void exit_error(char* progr) {
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON][COUPLED_SCALABLE_TCP]" << endl;
    exit(1);
}

int main(int argc, char **argv) {
    EventList eventlist;
    eventlist.setEndtime(timeFromSec(1000));
    Clock c(timeFromSec(50/100.), eventlist);
    int algo = UNCOUPLED;
    double epsilon = 1;

    if (argc>1) {
        if (!strcmp(argv[1],"UNCOUPLED"))
            algo = UNCOUPLED;
        else if (!strcmp(argv[1],"COUPLED_INC"))
            algo = COUPLED_INC;
        else if (!strcmp(argv[1],"FULLY_COUPLED"))
            algo = FULLY_COUPLED;
        else if (!strcmp(argv[1],"COUPLED_TCP"))
            algo = COUPLED_TCP;
        else if (!strcmp(argv[1],"COUPLED_SCALABLE_TCP"))
            algo = COUPLED_SCALABLE_TCP;
        else if (!strcmp(argv[1],"COUPLED_EPSILON")) {
            algo = COUPLED_EPSILON;
            if (argc>2) {
                epsilon = atof(argv[2]);
                printf("Using epsilon %f\n",epsilon);
            }
        }
        else
            exit_error(argv[0]);
    }

    srand(0);//time(NULL));

    // prepare the loggers
    stringstream filename(ios_base::out);
    filename << "/dev/null";
    cout << "Outputting to " << filename.str() << endl;
    Logfile logfile(filename.str(),eventlist);

    logfile.setStartTime(timeFromSec(0.5));
    QueueLoggerSimple logQueue = QueueLoggerSimple();
    logfile.addLogger(logQueue);
    	QueueLoggerSimple logPQueue1 = QueueLoggerSimple(); logfile.addLogger(logPQueue1);
    //QueueLoggerSimple logPQueue3 = QueueLoggerSimple(); logfile.addLogger(logPQueue3);
    QueueLoggerSimple logPQueue = QueueLoggerSimple();
    logfile.addLogger(logPQueue);
    MultipathTcpLoggerSimple mlogger = MultipathTcpLoggerSimple();
    logfile.addLogger(mlogger);

    TrafficLoggerSimple logger;
    logfile.addLogger(logger);
    SinkLoggerSampling sinkLogger = SinkLoggerSampling(timeFromMs(100),eventlist);

    logfile.addLogger(sinkLogger);


    QueueLoggerSampling qs1 = QueueLoggerSampling(timeFromMs(1000),eventlist);
    logfile.addLogger(qs1);
    QueueLoggerSampling qs2 = QueueLoggerSampling(timeFromMs(1000),eventlist);
    logfile.addLogger(qs2);

    TcpLoggerSimple logTcp;
    logfile.addLogger(logTcp);

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

    RandomQueue queue1(SERVICE1, BUFFER1, eventlist,&qs1,memFromPkt(RANDOM_BUFFER));
    queue1.setName("Queue1");
    logfile.writeName(queue1);

    RandomQueue queue2(SERVICE2, BUFFER2, eventlist,&qs2,memFromPkt(RANDOM_BUFFER));
    queue2.setName("Queue2");
    logfile.writeName(queue2);
    Queue pqueue2(SERVICE2*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
    pqueue2.setName("PQueue2");
    logfile.writeName(pqueue2);
    Queue pqueue3(SERVICE1*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
    pqueue3.setName("PQueue3");
    logfile.writeName(pqueue3);
    Queue pqueue4(SERVICE2*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
    pqueue4.setName("PQueue4");
    logfile.writeName(pqueue4);

    Queue* pqueue;

    Queue queue_back(max(SERVICE1,SERVICE2)*4, memFromPkt(1000), eventlist,NULL);
    queue_back.setName("queue_back");
    logfile.writeName(queue_back);

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(10), eventlist);

    //TCP flows on path 1
    TcpSrc* tcpSrc;
    TcpSink* tcpSnk;
    CbrSrc* cbrSrc;
    CbrSink* cbrSnk;
    route_t* routeout;
    route_t* routein;
    double extrastarttime;

    for (int i=0; i<TCP_1; i++) {
        cbrSrc = new CbrSrc(eventlist,speedFromPktps(9000),timeFromSec(0.01),timeFromSec(0.1));
        cbrSrc->setName("Cbr1");
        logfile.writeName(*cbrSrc);
        cbrSnk = new CbrSink();
        cbrSnk->setName("cbr1");
        logfile.writeName(*cbrSnk);

        // tell it the route
        pqueue = new Queue(SERVICE1*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
        pqueue->setName("PQueue1_"+ntoa(i));
        logfile.writeName(*pqueue);

        routeout = new route_t();
        routeout->push_back(pqueue);
        routeout->push_back(&queue1);
        routeout->push_back(&pipe1);
        routeout->push_back(cbrSnk);

        extrastarttime = drand()*50;
        cbrSrc->connect(*routeout,*cbrSnk,timeFromMs(extrastarttime));
        //	  sinkLogger.monitorSink(cbrSnk);
    }

    //TCP flow on path 2
    /*	for (int i=0;i<TCP_2;i++){
      cbrSrc = new CbrSrc(eventlist,speedFromPktps(10)); cbrSrc->setName("Cbr2"); logfile.writeName(*cbrSrc);
      cbrSnk = new CbrSink(); cbrSnk->setName("Cbr2"); logfile.writeName(*cbrSnk);

      pqueue = new Queue(SERVICE2*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
      pqueue->setName("PQueue2_"+ntoa(i)); logfile.writeName(*pqueue);

      // tell it the route
      routeout = new route_t(); routeout->push_back(pqueue); routeout->push_back(&queue2); routeout->push_back(&pipe2); routeout->push_back(cbrSnk);

      extrastarttime = 50*drand();
      cbrSrc->connect(*routeout,*cbrSnk,timeFromMs(extrastarttime));
      //	  sinkLogger.monitorSink(cbrSnk);
      }*/

    MultipathTcpSrc* mtcp;
    if (algo==COUPLED_EPSILON)
        mtcp = new MultipathTcpSrc(algo,eventlist,&mlogger,epsilon);
    else
        mtcp = new MultipathTcpSrc(algo,eventlist,&mlogger);

    //MTCP flow 1
    tcpSrc = new TcpSrc(NULL,NULL,eventlist);
    tcpSrc->setName("Subflow1");
    logfile.writeName(*tcpSrc);
    tcpSnk = new TcpSink();
    tcpSnk->setName("Subflow1");
    logfile.writeName(*tcpSnk);

    tcpRtxScanner.registerTcp(*tcpSrc);

    // tell it the route
    routeout = new route_t();
    routeout->push_back(&pqueue3);
    routeout->push_back(&queue1);
    routeout->push_back(&pipe1);
    routeout->push_back(tcpSnk);

    routein  = new route_t();
    //routein->push_back(&queue_back); routein->push_back(&pipe_back);
    routein->push_back(tcpSrc);
    extrastarttime = 50*drand();

    //join multipath connection
    mtcp->addSubflow(tcpSrc);

    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromMs(extrastarttime));
    sinkLogger.monitorSink(tcpSnk);

    //MTCP flow 2
    tcpSrc = new TcpSrc(NULL,NULL,eventlist);
    tcpSrc->setName("Subflow2");
    logfile.writeName(*tcpSrc);
    tcpSnk = new TcpSink();
    tcpSnk->setName("Subflow2");
    logfile.writeName(*tcpSnk);
    tcpRtxScanner.registerTcp(*tcpSrc);

    // tell it the route
    routeout = new route_t();
    routeout->push_back(&pqueue4);
    routeout->push_back(&queue2);
    routeout->push_back(&pipe2);
    routeout->push_back(tcpSnk);

    routein  = new route_t();
    //routein->push_back(&queue_back); routein->push_back(&pipe_back);
    routein->push_back(tcpSrc);
    extrastarttime = 50*drand();

    //join multipath connection
    mtcp->addSubflow(tcpSrc);

    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromMs(extrastarttime));
    sinkLogger.monitorSink(tcpSnk);

    // Record the setup
    int pktsize = TcpPacket::DEFAULTDATASIZE;
    logfile.write("# epsilon="+ntoa(epsilon));
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
