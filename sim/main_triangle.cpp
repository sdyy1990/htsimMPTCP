#include "config.h"
#include <string.h>
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
simtime_picosec RTT=timeFromMs(5);

double targetwnd = 30;
int NUMFLOWS = 2;

#define MTCP 2

linkspeed_bps SERVICE = speedFromPktps(10000);//NUMFLOWS * targetwnd/timeAsSec(RTT1));

#define RANDOM_BUFFER 10

#define FEEDER_BUFFER 2000

mem_b BUFFER=memFromPkt(RANDOM_BUFFER+50);//NUMFLOWS * targetwnd);

void exit_error(char* progr) {
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED]" << endl;
    exit(1);
}

int main(int argc, char **argv) {
    EventList eventlist;
    eventlist.setEndtime(timeFromSec(100));
    Clock c(timeFromSec(50/100.), eventlist);
    int algo = UNCOUPLED;

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
    MultipathTcpLoggerSimple mlogger = MultipathTcpLoggerSimple();
    logfile.addLogger(mlogger);
    SinkLoggerSampling sinkLogger = SinkLoggerSampling(timeFromMs(100),eventlist);

    logfile.addLogger(sinkLogger);

    RandomQueue* queue_fw[3];
    RandomQueue* queue_back[3];
    Pipe* pipe_fw[3];
    Pipe* pipe_back[3];

    for (int i=0; i<3; i++) {
        pipe_fw[i] = new Pipe(RTT,eventlist);
        pipe_fw[i]->setName("pipefw"+ntoa(i));
        logfile.writeName(*(pipe_fw[i]));

        pipe_back[i] = new Pipe(RTT,eventlist);
        pipe_back[i]->setName("pipeback"+ntoa(i));
        logfile.writeName(*(pipe_back[i]));

        QueueLoggerSampling* qs = new QueueLoggerSampling(timeFromMs(100),eventlist);
        logfile.addLogger(*qs);

        queue_fw[i] = new RandomQueue(SERVICE, BUFFER, eventlist,qs,memFromPkt(RANDOM_BUFFER));
        queue_fw[i]->setName("Queuefw"+ntoa(i));
        logfile.writeName(*(queue_fw[i]));

        qs = new QueueLoggerSampling(timeFromMs(100),eventlist);
        logfile.addLogger(*qs);
        queue_back[i] = new RandomQueue(SERVICE, BUFFER, eventlist,qs,memFromPkt(RANDOM_BUFFER));
        queue_back[i]->setName("Queueback"+ntoa(i));
        logfile.writeName(*(queue_back[i]));
    }

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(10), eventlist);

    TcpSrc* tcpSrc;
    TcpSink* tcpSnk;
    route_t* routeout;
    route_t* routein;
    double extrastarttime;

    for (int k=0; k<MTCP; k++)
        for (int i=0; i<3; i++)
            for (int j=0; j<3; j++)
                if (i!=j) {
                    MultipathTcpSrc* mtcp = new MultipathTcpSrc(algo,eventlist,&mlogger);

                    mtcp->setName("mtcp"+ntoa(i)+"_"+ntoa(j));
                    logfile.writeName(*mtcp);
                    //MTCP flow 1
                    tcpSrc = new TcpSrc(NULL,NULL,eventlist);
                    tcpSrc->setName("Subflow1_"+ntoa(i)+"_"+ntoa(j)+"_"+ntoa(k));
                    logfile.writeName(*tcpSrc);
                    tcpSnk = new TcpSink();
                    tcpSnk->setName("Subflow1_"+ntoa(i)+"_"+ntoa(j)+"_"+ntoa(k));
                    logfile.writeName(*tcpSnk);

                    tcpRtxScanner.registerTcp(*tcpSrc);

                    Queue *pqueue = new Queue(SERVICE*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
                    //pqueue->setName("PQueue"+ntoa(i));
                    //logfile.writeName(*pqueue);

                    // tell it the route
                    routeout = new route_t();
                    routeout->push_back(pqueue);

                    if ((i+1)%3==j) {
                        routeout->push_back(queue_fw[i]);
                        routeout->push_back(pipe_fw[i]);
                    }
                    else {
                        routeout->push_back(queue_back[(i+2)%3]);
                        routeout->push_back(pipe_back[(i+2)%3]);
                    }

                    routeout->push_back(tcpSnk);

                    routein  = new route_t();
                    routein->push_back(tcpSrc);
                    extrastarttime = 50*drand();

                    //join multipath connection
                    mtcp->addSubflow(tcpSrc);
                    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromMs(extrastarttime));

                    sinkLogger.monitorSink(tcpSnk);

                    //MTCP flow 2
                    tcpSrc = new TcpSrc(NULL,NULL,eventlist);
                    tcpSrc->setName("Subflow2"+ntoa(i)+"_" +ntoa(j)+"_"+ntoa(k));
                    logfile.writeName(*tcpSrc);
                    tcpSnk = new TcpSink();
                    tcpSnk->setName("Subflow2_"+ntoa(i)+"_" + ntoa(j)+"_"+ntoa(k));
                    logfile.writeName(*tcpSnk);

                    tcpRtxScanner.registerTcp(*tcpSrc);

                    pqueue = new Queue(SERVICE*2, memFromPkt(FEEDER_BUFFER), eventlist,NULL);
                    pqueue->setName("PQueue2"+ntoa(i));
                    logfile.writeName(*pqueue);

                    // tell it the route
                    routeout = new route_t();
                    routeout->push_back(pqueue);

                    if ((i+1)%3!=j) {
                        routeout->push_back(queue_fw[i]);
                        routeout->push_back(pipe_fw[i]);

                        routeout->push_back(queue_fw[(i+1)%3]);
                        routeout->push_back(pipe_fw[(i+1)%3]);
                    }
                    else {
                        routeout->push_back(queue_back[(i+2)%3]);
                        routeout->push_back(pipe_back[(i+2)%3]);

                        routeout->push_back(queue_back[(i+1)%3]);
                        routeout->push_back(pipe_back[(i+1)%3]);
                    }

                    routeout->push_back(tcpSnk);

                    routein  = new route_t();
                    routein->push_back(tcpSrc);
                    extrastarttime = 50*drand();

                    //join multipath connection
                    mtcp->addSubflow(tcpSrc);
                    tcpSrc->connect(*routeout,*routein,*tcpSnk,timeFromMs(extrastarttime));

                    sinkLogger.monitorSink(tcpSnk);
                }
    // Record the setup
    int pktsize = TcpPacket::DEFAULTDATASIZE;
    logfile.write("# pktsize="+ntoa(pktsize)+" bytes");
    logfile.write("# bottleneckrate="+ntoa(speedAsPktps(SERVICE))+" pkt/sec");


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
