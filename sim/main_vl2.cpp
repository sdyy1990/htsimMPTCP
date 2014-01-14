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
#include "tcp.h"
#include "tcp_periodic.h"
#include "cbr.h"
#include <list>

string ntoa(double n);
string itoa(uint64_t n);

// Simulation params

#define PERIODIC 0
//#define NI 7        //Number of intermediate switches
//#define NA 14        //Number of aggregation switches
//#define NT 49        //Number of ToR switches

#define NI 3        //Number of intermediate switches
#define NA 6        //Number of aggregation switches
#define NT 9        //Number of ToR switches

//180 hosts

#define NS 20        //Number of servers per ToR switch

#define N (NS*NT)

#define NT2A 2      //Number of connections from a ToR to aggregation switches

#define TOR_ID(id) N+id
#define AGG_ID(id) N+NT+id
#define INT_ID(id) N+NT+NA+id
#define HOST_ID(hid,tid) tid*NS+hid

#define HOST_TOR(host) host/NS
#define HOST_TOR_ID(host) host%NS
#define TOR_AGG1(tor) tor%NA
#define TOR_AGG2(tor) (2*NA - tor - 1)%NA


int RTT = 1; // Identical RTT
int SERVICE = 80000; // Identical service time

//#define SWITCH_BUFFER (SERVICE * RTT / 1000)
#define SWITCH_BUFFER 100

#define RANDOM_BUFFER 3

#define FEEDER_BUFFER 1000

EventList eventlist;

Pipe * pipes_ni_na[NI][NA];
Pipe * pipes_na_nt[NA][NT];
Pipe * pipes_nt_ns[NT][NS];
RandomQueue * queues_ni_na[NI][NA];
RandomQueue * queues_na_nt[NA][NT];
RandomQueue * queues_nt_ns[NT][NS];
Pipe * pipes_na_ni[NA][NI];
Pipe * pipes_nt_na[NT][NA];
Pipe * pipes_ns_nt[NS][NT];
RandomQueue * queues_na_ni[NA][NI];
RandomQueue * queues_nt_na[NT][NA];
RandomQueue * queues_ns_nt[NS][NT];
Logfile* lg;

route_t* create_path(int src, int dest, int choice1, int choice2) {
    route_t* routeout = new route_t();
    Queue* pqueue = new Queue(speedFromPktps(SERVICE), memFromPkt(FEEDER_BUFFER), eventlist, NULL);
    pqueue->setName("PQueue_" + ntoa(src) + "_" + ntoa(dest));
    lg->writeName(*pqueue);

    routeout->push_back(pqueue);

    //server to TOR switch
    routeout->push_back(queues_ns_nt[HOST_TOR_ID(src)][HOST_TOR(src)]);
    routeout->push_back(pipes_ns_nt[HOST_TOR_ID(src)][HOST_TOR(src)]);

    assert(queues_ns_nt[HOST_TOR_ID(src)][HOST_TOR(src)]!=NULL);
    assert(pipes_ns_nt[HOST_TOR_ID(src)][HOST_TOR(src)]!=NULL);

    int agg_switch;
    if (src%20<10)
        //use link to first agg switch
        agg_switch = TOR_AGG1(HOST_TOR(src));
    else
        agg_switch = TOR_AGG2(HOST_TOR(src));

    //TOR switch to AGG switch
    routeout->push_back(queues_nt_na[HOST_TOR(src)][agg_switch]);
    routeout->push_back( pipes_nt_na[HOST_TOR(src)][agg_switch]);

    assert(queues_nt_na[HOST_TOR(src)][agg_switch]!=NULL);
    assert(pipes_nt_na[HOST_TOR(src)][agg_switch]!=NULL);

    //AGG to INT switch
    //now I need to connect to the choice1 server
    //0<=choice1<NI
    routeout->push_back(queues_na_ni[agg_switch][choice1]);
    routeout->push_back( pipes_na_ni[agg_switch][choice1]);

    assert(queues_na_ni[agg_switch][choice1]!=NULL);
    assert(pipes_na_ni[agg_switch][choice1]!=NULL);

    //now I need to connect to the choice2 agg server - this is more tricky!
    //0<=choice2<NT2A

    int agg_switch_2;
    if (choice2==0)
        agg_switch_2 = TOR_AGG1(HOST_TOR(dest));
    else
        agg_switch_2 = TOR_AGG2(HOST_TOR(dest));

    //INT to agg switch
    routeout->push_back(queues_ni_na[choice1][agg_switch_2]);
    routeout->push_back(pipes_ni_na[choice1][agg_switch_2]);

    assert(queues_ni_na[choice1][agg_switch_2]!=NULL);
    assert(pipes_ni_na[choice1][agg_switch_2]!=NULL);

    //agg to TOR
    routeout->push_back(queues_na_nt[agg_switch_2][HOST_TOR(dest)]);
    routeout->push_back(pipes_na_nt[agg_switch_2][HOST_TOR(dest)]);
    assert(queues_na_nt[agg_switch_2][HOST_TOR(dest)]!=NULL);
    assert(pipes_na_nt[agg_switch_2][HOST_TOR(dest)]!=NULL);

    //tor to server
    routeout->push_back(queues_nt_ns[HOST_TOR(dest)][HOST_TOR_ID(dest)]);
    routeout->push_back(pipes_nt_ns[HOST_TOR(dest)][HOST_TOR_ID(dest)]);

    assert(queues_nt_ns[HOST_TOR(dest)][HOST_TOR_ID(dest)]!=NULL);
    assert(pipes_nt_ns[HOST_TOR(dest)][HOST_TOR_ID(dest)]!=NULL);

    return routeout;
}

void exit_error(char* progr) {
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP" << endl;
    exit(1);
}


int main(int argc, char **argv) {
    eventlist.setEndtime(timeFromSec(20));
    Clock c(timeFromSec(50 / 100.), eventlist);
    int algo = COUPLED_EPSILON;
    double epsilon = 1;


    if (argc > 1) {
        epsilon = -1;
        if (!strcmp(argv[1], "UNCOUPLED"))
            algo = UNCOUPLED;
        else if (!strcmp(argv[1], "COUPLED_INC"))
            algo = COUPLED_INC;
        else if (!strcmp(argv[1], "FULLY_COUPLED"))
            algo = FULLY_COUPLED;
        else if (!strcmp(argv[1], "COUPLED_TCP"))
            algo = COUPLED_TCP;
        else if (!strcmp(argv[1], "COUPLED_SCALABLE_TCP"))
            algo = COUPLED_SCALABLE_TCP;
        else if (!strcmp(argv[1], "COUPLED_EPSILON")) {
            algo = COUPLED_EPSILON;
            if (argc > 2)
                epsilon = atof(argv[2]);
            printf("Using epsilon %f\n", epsilon);
        } else
            exit_error(argv[0]);
    }

    srand(time(NULL));

    cout <<  "Using algo="<<algo<< " epsilon=" << epsilon << endl;
    // prepare the loggers
    stringstream filename(ios_base::out);
    filename << "../data/logout.dat";
    cout << "Outputting to " << filename.str() << endl;
    //Logfile
    Logfile logfile(filename.str(), eventlist);

    std::ofstream topology("topology.vl2");
    if (!topology) {
        cout << "Cannot open topology file"<<endl;
        exit(1);
    }

    topology << N + NT + NA + NI << endl << endl;

    lg = &logfile;

    logfile.setStartTime(timeFromSec(0));
    //QueueLoggerSimple logQueue1 = QueueLoggerSimple(); logfile.addLogger(logQueue1);
    //QueueLoggerSimple logPQueue = QueueLoggerSimple(); logfile.addLogger(logPQueue);

    SinkLoggerSampling sinkLogger = SinkLoggerSampling(timeFromMs(100), eventlist);
    logfile.addLogger(sinkLogger);
    QueueLoggerSampling* queueLogger;

    //TcpLoggerSimple logTcp;logfile.addLogger(logTcp);


    TcpSrc* tcpSrc;
    TcpSink* tcpSnk;
    route_t* routeout, *routein;
    double extrastarttime;

    //set all links/pipes to NULL

    for (int j=0; j<NI; j++)
        for (int k=0; k<NA; k++) {
            queues_ni_na[j][k] = NULL;
            pipes_ni_na[j][k] = NULL;
            queues_na_ni[k][j] = NULL;
            pipes_na_ni[k][j] = NULL;
        }

    for (int j=0; j<NA; j++)
        for (int k=0; k<NT; k++) {
            queues_na_nt[j][k] = NULL;
            pipes_na_nt[j][k] = NULL;
            queues_nt_na[k][j] = NULL;
            pipes_nt_na[k][j] = NULL;
        }

    for (int j=0; j<NT; j++)
        for (int k=0; k<NS; k++) {
            queues_nt_ns[j][k] = NULL;
            pipes_nt_ns[j][k] = NULL;
            queues_ns_nt[k][j] = NULL;
            pipes_ns_nt[k][j] = NULL;
        }

    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(10), eventlist);

    // ToR switch to server
    for (int j = 0; j < NT; j++) {
        for (int k = 0; k < NS; k++) {
            // Downlink
            queueLogger = new QueueLoggerSampling(timeFromMs(1000), eventlist);
            logfile.addLogger(*queueLogger);
            queues_nt_ns[j][k] = new RandomQueue(speedFromPktps(SERVICE/10), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
            queues_nt_ns[j][k]->setName("Queue-nt-ns-" + ntoa(j) + "-" + ntoa(k));
            logfile.writeName(*(queues_nt_ns[j][k]));

            pipes_nt_ns[j][k] = new Pipe(timeFromMs(RTT), eventlist);
            pipes_nt_ns[j][k]->setName("Pipe-nt-ns-" + ntoa(j) + "-" + ntoa(k));
            logfile.writeName(*(pipes_nt_ns[j][k]));

            // Uplink
            queueLogger = new QueueLoggerSampling(timeFromMs(1000), eventlist);
            logfile.addLogger(*queueLogger);
            queues_ns_nt[k][j] = new RandomQueue(speedFromPktps(SERVICE/10), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
            queues_ns_nt[k][j]->setName("Queue-ns-nt-" + ntoa(k) + "-" + ntoa(j));
            logfile.writeName(*(queues_ns_nt[k][j]));

            pipes_ns_nt[k][j] = new Pipe(timeFromMs(RTT), eventlist);
            pipes_ns_nt[k][j]->setName("Pipe-ns-nt-" + ntoa(k) + "-" + ntoa(j));
            logfile.writeName(*(pipes_ns_nt[k][j]));


            topology << HOST_ID(k,j) <<" "<< TOR_ID(j) << " "<<1 << endl;
            topology << TOR_ID(j) << " "<<HOST_ID(k,j) << " "<<1 << endl;
        }
    }

    //ToR switch to aggregation switch
    for (int j = 0; j < NT; j++) {
        //Connect the ToR switch to NT2A aggregation switches
        for (int l=0; l<NT2A; l++) {
            int k;
            if (l==0)
                k = TOR_AGG1(j);
            else
                k = TOR_AGG2(j);

            // Downlink
            queueLogger = new QueueLoggerSampling(timeFromMs(1000), eventlist);
            logfile.addLogger(*queueLogger);
            queues_na_nt[k][j] = new RandomQueue(speedFromPktps(SERVICE), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
            queues_na_nt[k][j]->setName("Queue-na-nt-" + ntoa(k) + "-" + ntoa(j));
            logfile.writeName(*(queues_nt_ns[j][k]));

            pipes_na_nt[k][j] = new Pipe(timeFromMs(RTT), eventlist);
            pipes_na_nt[k][j]->setName("Pipe-na-nt-" + ntoa(k) + "-" + ntoa(j));
            logfile.writeName(*(pipes_na_nt[k][j]));

            // Uplink
            queueLogger = new QueueLoggerSampling(timeFromMs(1000), eventlist);
            logfile.addLogger(*queueLogger);
            queues_nt_na[j][k] = new RandomQueue(speedFromPktps(SERVICE), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
            queues_nt_na[j][k]->setName("Queue-nt-na-" + ntoa(j) + "-" + ntoa(k));
            logfile.writeName(*(queues_nt_na[j][k]));

            pipes_nt_na[j][k] = new Pipe(timeFromMs(RTT), eventlist);
            pipes_nt_na[j][k]->setName("Pipe-nt-na-" + ntoa(j) + "-" + ntoa(k));
            logfile.writeName(*(pipes_nt_na[j][k]));

            topology << AGG_ID(k) << " "<<TOR_ID(j) << " "<< 2 << endl;
            topology << TOR_ID(j) << " "<<AGG_ID(k) << " "<< 2 << endl;
        }
    }

    // Aggregation switch to intermediate switch

    for (int j = 0; j < NI; j++) {
        for (int k = 0; k < NA; k++) {
            // Downlink
            queueLogger = new QueueLoggerSampling(timeFromMs(1000), eventlist);
            logfile.addLogger(*queueLogger);

            queues_ni_na[j][k] = new RandomQueue(speedFromPktps(SERVICE/2), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
            queues_ni_na[j][k]->setName("Queue-ni-na-" + ntoa(j) + "-" + ntoa(k));
            logfile.writeName(*(queues_ni_na[j][k]));

            pipes_ni_na[j][k] = new Pipe(timeFromMs(RTT), eventlist);
            pipes_ni_na[j][k]->setName("Pipe-ni-na-" + ntoa(j) + "-" + ntoa(k));
            logfile.writeName(*(pipes_ni_na[j][k]));

            // Uplink

            queueLogger = new QueueLoggerSampling(timeFromMs(1000), eventlist);
            logfile.addLogger(*queueLogger);

            queues_na_ni[k][j] = new RandomQueue(speedFromPktps(SERVICE), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
            queues_na_ni[k][j]->setName("Queue-na-ni-" + ntoa(k) + "-" + ntoa(j));
            logfile.writeName(*(queues_na_ni[k][j]));

            pipes_na_ni[k][j] = new Pipe(timeFromMs(RTT), eventlist);
            pipes_na_ni[k][j]->setName("Pipe-na-ni-" + ntoa(k) + "-" + ntoa(j));
            logfile.writeName(*(pipes_na_ni[k][j]));

            topology << AGG_ID(k) << " " <<INT_ID(j) << " "<<1 << endl;
            topology << INT_ID(j) << " " <<AGG_ID(k) << " "<<1 << endl;
        }
    }


    MultipathTcpSrc* mtcp;
    int subflow_count = 3;


    int* is_dest = new int[N];
    int dest;
    for (int i=0; i<N; i++)
        is_dest[i] = 0;

    // Permutation connections
    for (int src = 0; src < N; src++) {
        int r = rand()%(N-src);
        for (dest = 0; dest<N; dest++) {
            if (r==0&&!is_dest[dest])
                break;
            if (!is_dest[dest])
                r--;
        }

        if (r!=0||is_dest[dest]) {
            cout <<"Wrong connections r " <<  r << "is_dest "<<is_dest[dest]<<endl;
            exit(1);
        }

        if (src==dest) {
            //find first other destination that is different!
            do {
                dest = (dest+1)%N;
            }
            while (is_dest[dest]);

            if (src==dest) {
                printf("Wrong connections 2!\n");
                exit(1);
            }
        }
        is_dest[dest] = 1;

        if (algo == COUPLED_EPSILON)
            mtcp = new MultipathTcpSrc(algo, eventlist, NULL, epsilon);
        else
            mtcp = new MultipathTcpSrc(algo, eventlist, NULL);

        int choice,choice2;
        choice = 0;

        for (int inter = 0; inter < subflow_count; inter++) {
            tcpSrc = new TcpSrc(NULL, NULL, eventlist);
            tcpSrc->setName("mtcp_" + ntoa(src) + "_" + ntoa(inter) + "_" + ntoa(dest));
            logfile.writeName(*tcpSrc);
            tcpSnk = new TcpSink();
            tcpSnk->setName("mtcp_sink_" + ntoa(src) + "_" + ntoa(inter) + "_" + ntoa(dest));
            logfile.writeName(*tcpSnk);

            tcpRtxScanner.registerTcp(*tcpSrc);

            // tell it the route

            if (subflow_count==4) {
                if (inter%NT2A==0)
                    choice = rand()%NI;

                choice2 = inter%NT2A;
            }
            else if (subflow_count==6) {
                choice = inter/2;
                choice2 = inter%NT2A;
            }
            else if (subflow_count==3) {
                choice = inter;
                choice2 = rand()%2;
            }
            else {
                choice = rand()%NI;
                choice2 = rand()%2;
            }
            routeout = create_path(src,dest,choice,choice2);
            routeout->push_back(tcpSnk);

            routein = new route_t();
            routein->push_back(tcpSrc);
            extrastarttime = 50 * drand();

            //join multipath connection

            mtcp->addSubflow(tcpSrc);

            if (inter == 0) {
                mtcp->setName("multipath" + ntoa(src) + "-" + ntoa(dest));
                logfile.writeName(*mtcp);
            }

            tcpSrc->connect(*routeout, *routein, *tcpSnk, timeFromMs(extrastarttime));
            sinkLogger.monitorSink(tcpSnk);
        }
    }

    // Record the setup
    int pktsize = TcpPacket::DEFAULTDATASIZE;
    logfile.write("# pktsize=" + ntoa(pktsize) + " bytes");
    logfile.write("# subflows=" + ntoa(subflow_count));
    logfile.write("# bottleneckrate = " + ntoa(SERVICE) + " pkt/sec");
    logfile.write("# buffer = " + ntoa((double) (queues_na_ni[0][1]->_maxsize) / ((double) pktsize)) + " pkt");
    double rtt = timeAsSec(timeFromMs(RTT));
    logfile.write("# rtt =" + ntoa(rtt));



    // GO!
    while (eventlist.doNextEvent()) {
    }



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
