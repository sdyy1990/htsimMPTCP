#include "config.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include <string.h>
#include <math.h>
#include "network.h"
#include "randomqueue.h"
#include "pipe.h"
#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "clock.h"
#include "mtcp.h"
#include "cbr.h"
#include "exoqueue.h"

string ntoa(double n);
string itoa(uint64_t n);

// Simulation params

#define LINKS 5
#define FLOWS 1

int RTT [] = {100,100,100,100,100};
int SERVICE [] = {200,200,200,200,200};

#define RANDOM_BUFFER 3

#define FEEDER_BUFFER 2000

void exit_error(char* progr) {
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP" << endl;
    exit(1);
}

int main(int argc, char **argv) {
    EventList eventlist;
    eventlist.setEndtime(timeFromSec(10000));
    Clock c(timeFromSec(50/100.), eventlist);
    int algo = UNCOUPLED;
    double epsilon = 1.0;
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
        else if (!strcmp(argv[1],"COUPLED_SCALABLE_TCP"))
            algo = COUPLED_SCALABLE_TCP;
        else if (!strcmp(argv[1],"COUPLED_EPSILON")) {
            algo = COUPLED_EPSILON;
            if (argc>2)
                epsilon = atof(argv[2]);
            printf("Using epsilon %f\n",epsilon);
        }
        else
            exit_error(argv[0]);
    }

    srand(time(NULL));

    // prepare the loggers
    stringstream filename(ios_base::out);
    filename << "../data/logout.dat";
    cout << "Outputting to " << filename.str() << endl;
    Logfile logfile(filename.str(),eventlist);

    logfile.setStartTime(timeFromSec(0));
    //QueueLoggerSimple logQueue1 = QueueLoggerSimple(); logfile.addLogger(logQueue1);
    //QueueLoggerSimple logPQueue = QueueLoggerSimple(); logfile.addLogger(logPQueue);

    SinkLoggerSampling sinkLogger = SinkLoggerSampling(timeFromMs(1000),eventlist);
    logfile.addLogger(sinkLogger);
    QueueLoggerSampling* queueLogger;

    //TcpLoggerSimple logTcp;logfile.addLogger(logTcp);


    Pipe* pipes[LINKS];
    RandomQueue* queues[LINKS];

    for (int i=0; i<LINKS; i++) {
        queueLogger =  new QueueLoggerSampling(timeFromMs(1000),eventlist);
        logfile.addLogger(*queueLogger);
        queues[i] = new RandomQueue(speedFromPktps(SERVICE[i]), memFromPkt(SERVICE[i]*RTT[i]/1000+RANDOM_BUFFER),eventlist,queueLogger,memFromPkt(RANDOM_BUFFER));
        queues[i]->setName("Queue"+ntoa(i));
        logfile.writeName(*(queues[i]));

        pipes[i] = new Pipe(timeFromMs(RTT[i]),eventlist);
        pipes[i]->setName("Pipe"+ntoa(i));
        logfile.writeName(*(pipes[i]));
    }


    Queue* pqueue;
    TcpSrc* tcpSrc;
    TcpSink* tcpSnk;
    route_t* routeout,*routein;
    double extrastarttime;

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(10), eventlist);

    //do cbr flow
    //	CbrSrc cbr(eventlist,speedFromPktps(300),timeFromSec(10),timeFromSec(10));
    //CbrSink cSink;
    //routeout = new route_t(); routeout->push_back(queues[2]); routeout->push_back(pipes[2]); routeout->push_back(&cSink);
    //cbr.connect(*routeout,cSink,timeFromMs((int)(50*drand())));

    MultipathTcpSrc* mtcp;

    for (int i=0; i<LINKS-1; i++) {
        for (int k = 0; k<FLOWS; k++) {
            if (algo==COUPLED_EPSILON)
                mtcp = new MultipathTcpSrc(algo,eventlist,NULL,epsilon);
            else
                mtcp = new MultipathTcpSrc(algo,eventlist,NULL);

            for (int j = 0; j<2; j++) {
                //MTCP flow j
                tcpSrc = new TcpSrc(NULL,NULL,eventlist);
                tcpSrc->setName("mtcp_"+ntoa(k)+"_"+ntoa(i)+"_"+ntoa(j+1));
                logfile.writeName(*tcpSrc);
                tcpSnk = new TcpSink();
                tcpSnk->setName("mtcp_sink_"+ntoa(k)+"_"+ntoa(i)+"_"+ntoa(j+1));
                logfile.writeName(*tcpSnk);

                sinkLogger.monitorSink(tcpSnk);
                tcpRtxScanner.registerTcp(*tcpSrc);

                // tell it the route
                //create PQueue
                pqueue = new Queue(speedFromPktps(SERVICE[i+j]*2), memFromPkt(FEEDER_BUFFER), eventlist,NULL);
                pqueue->setName("PQueue_"+ntoa(k)+"_"+ntoa(i)+"_"+ntoa(j+1));
                logfile.writeName(*pqueue);
                routeout = new route_t();
                routeout->push_back(pqueue);
                routeout->push_back(queues[i+j]);
                routeout->push_back(pipes[i+j]);
                routeout->push_back(tcpSnk);

                routein  = new route_t();
                routein->push_back(tcpSrc);
                extrastarttime = 50*drand();

                //join multipath connection
                mtcp->addSubflow(tcpSrc);

                tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromMs(extrastarttime));
            }
        }
    }
    // Record the setup
    int pktsize = TcpPacket::DEFAULTDATASIZE;
    logfile.write("# pktsize="+ntoa(pktsize)+" bytes");
    for (int i=0; i<LINKS; i++) {
        logfile.write("# bottleneckrate "+ntoa(i)+"="+ntoa(SERVICE[i])+" pkt/sec");
        logfile.write("# buffer "+ntoa(i)+"="+ntoa((double)(queues[i]->_maxsize)/((double)pktsize))+" pkt");
        double rtt = timeAsSec(timeFromMs(RTT[i]));
        logfile.write("# rtt "+ntoa(i)+"="+ntoa(rtt));
    }

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
