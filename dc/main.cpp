#include "config.h"
#include <sstream>
#include <iostream>
#include <bitset>
#include <string.h>
#include <math.h>
#include "network.h"
#include "randomqueue.h"
#include "subflow_control.h"
#include "shortflows.h"
#include "pipe.h"
#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "clock.h"
#include "mtcp.h"
#include "tcp.h"
#include "tcp_transfer.h"
#include "cbr.h"
#include "topology.h"
#include "connection_matrix.h"
//#include "fat_tree_topology.h"
#include "sw_topology.h"
//#include "s2_topology.h"
//#include "jelly_topology.h"
//#include "star_topology.h"
#include <list>

// Simulation params

#define PRINT_PATHS 1

#define PERIODIC 0
#include "main.h"

int RTT = 100;					// Identical RTT microseconds = 0.1 ms
int N = 128;

unsigned int subflow_count = 1;

string ntoa (double n);
string itoa (uint64_t n);

#define USE_FIRST_FIT 0
#define FIRST_FIT_INTERVAL 100

EventList eventlist;

Logfile *lg;

void
exit_error (char *progr)
{
    cout << "Usage " << progr <<
         " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP"
         << endl;
    exit (1);
}

void
print_path (std::ofstream & paths, route_t * rt)
{
    for (unsigned int i = 1; i < rt->size () - 1; i += 2) {
        RandomQueue *q = (RandomQueue *) rt->at (i);

        if (q != NULL) {
            paths << q->str () << " ";
        } else {
            paths << "NULL ";
        }
    }

    paths << endl;
}
const int FATTREE_TOPO = 1;
const int SW_TOPO = 2;
const int S2_TOPO = 3;
const int JE_TOPO = 4;
const int STRIDE_TRAFFIC = 1;
const int PERMUTAION_TRAFFIC = 2;
const int SINGLE_FLOW_NIC_TRAFFIC = 3;
int
main (int argc, char **argv)
{
    cout.setf(ios::fixed);
    cout.precision(3);
    eventlist.setEndtime (timeFromSec (200));
    Clock c (timeFromSec (50 / 100.), eventlist);
    int algo = COUPLED_EPSILON;
    double epsilon = 1;
    int traffic_param = 0;
    int traffic_style = -1;
    stringstream filename (ios_base::out);
    uint64_t pktperflow = 1048576LL/1000 + 1;
    bool uselog = false;
    uint32_t topoType = FATTREE_TOPO;
    char * topofilename, *pathfilename;
    filename << "logout.dat";
    int i = 1;
    while (i < argc) {
        int i0=i;
        if (argc > i && !strcmp (argv[i], "-sub")) {
            subflow_count = atoi (argv[i + 1]);
            i += 2;
            cout << "Using subflow count " << subflow_count << endl;
        }
        if (argc > i && !strcmp (argv[i], "-flowM")) {
            pktperflow = atoi (argv[i + 1])*1048576LL/1000 + 1;
            i += 2;
        }
        if (argc > i && !strcmp (argv[i], "-tra")) {
            if (argv[i+1][0]=='S') 
                traffic_style = STRIDE_TRAFFIC;
            if (argv[i+1][0]=='P')
                traffic_style = PERMUTAION_TRAFFIC;
            if (argv[i+1][0]=='O')
                traffic_style = SINGLE_FLOW_NIC_TRAFFIC;
            traffic_param = atoi (argv[i + 2]);
            i += 3;
            cout << "Using subflow count " << subflow_count << endl;
        }
        if (argc >i && !strcmp ( argv[i],"-type")) {
            if (!strcmp(argv[i+1],"SW")) {
                topofilename = argv[i+2];    i+=3;        topoType = SW_TOPO;
            }
            if (!strcmp(argv[i+1],"S2")) {
                topofilename = argv[i+2];    i+=3;        topoType = S2_TOPO;
            }
            if (!strcmp(argv[i+1],"JE")) {
                topofilename = argv[i+2]; pathfilename = argv[i+3]; i+=4; topoType=JE_TOPO;
            }
            
        }
        if (i0!=i) continue;
        if (argc > i) {
            epsilon = -1;
            if (!strcmp (argv[i], "UNCOUPLED")) {
                algo = UNCOUPLED;
            } else if (!strcmp (argv[i], "COUPLED_INC")) {
                algo = COUPLED_INC;
            } else if (!strcmp (argv[i], "FULLY_COUPLED")) {
                algo = FULLY_COUPLED;
            } else if (!strcmp (argv[i], "COUPLED_TCP")) {
                algo = COUPLED_TCP;
            } else if (!strcmp (argv[i], "COUPLED_SCALABLE_TCP")) {
                algo = COUPLED_SCALABLE_TCP;
            } else if (!strcmp (argv[i], "COUPLED_EPSILON")) {
                algo = COUPLED_EPSILON;
                if (argc > i + 1) {
                    epsilon = atof (argv[i + 1]);
                }
                printf ("Using epsilon %f\n", epsilon);
            } else {
                exit_error (argv[0]);
            }
        }
    }

    srand (time (NULL));
    cout << "Using algo=" << algo << " epsilon=" << epsilon << endl;
    // prepare the loggers
    cout << "Logging to " << filename.str () << endl;
    //Logfile
    Logfile logfile (filename.str (), eventlist);
    if (!uselog) logfile.shutdown();
#if PRINT_PATHS
    filename << ".paths";
    cout << "Logging path choices to " << filename.str () << endl;
    std::ofstream paths (filename.str ().c_str ());

    if (!paths) {
        cout << "Can't open for writing paths file!" << endl;
        exit (1);
    }

#endif
    int tot_subs = 0;
    int cnt_con = 0;
    lg = &logfile;
    logfile.setStartTime (timeFromSec (0));
    SinkLoggerSampling sinkLogger =
        SinkLoggerSampling (timeFromMs (1000), eventlist);
    logfile.addLogger (sinkLogger);
    //TcpLoggerSimple logTcp;logfile.addLogger(logTcp);
    TcpSrc *tcpSrc;
    TcpSink *tcpSnk;
    //CbrSrc* cbrSrc;
    //CbrSink* cbrSnk;
    route_t *routeout, *routein;
    double extrastarttime;
    TcpRtxTimerScanner tcpRtxScanner (timeFromMs (10), eventlist);
    MultipathTcpSrc *mtcp;
    vector<MultipathTcpSrc*> mptcpVector;
    int dest;
    Topology *top;
  //  if (topoType == FATTREE_TOPO)        top    = new FatTreeTopology (&logfile, &eventlist);
    if (topoType == SW_TOPO)        top    = new SWTopology (&logfile, &eventlist, topofilename);
//    if (topoType == S2_TOPO)        top    = new S2Topology (&logfile, &eventlist, topofilename);
//    if (topoType == JE_TOPO)        { JellyTopology * jtop = new JellyTopology(&logfile, &eventlist, topofilename,pathfilename); top = jtop;    jtop->set_maxpathcount(subflow_count);        }


    N = top->get_host_count();
    vector < pair<route_t *,route_t *> >***net_paths;
    net_paths = new vector < pair<route_t *,route_t * > >**[N];
    int *is_dest = new int[N];

    for (int i = 0; i < N; i++) {
        is_dest[i] = 0;
        net_paths[i] = new vector < pair<route_t *,route_t *> >*[N];

        for (int j = 0; j < N; j++) {
            net_paths[i][j] = NULL;
        }
    }


    vector < int >*destinations;
    ConnectionMatrix *conns = new ConnectionMatrix (N);
    if (traffic_style == STRIDE_TRAFFIC) 
        conns->setStride(traffic_param,0);
    if (traffic_style == PERMUTAION_TRAFFIC)
        conns->setPermutation(traffic_param);
    if (traffic_style == SINGLE_FLOW_NIC_TRAFFIC)
        conns->setSingleFlow(traffic_param);
    map < int, vector < int >*>::iterator it;
    int connID = 0;
    MultipathTcpSrcListener *mlistener ;
    mlistener = new MultipathTcpSrcListener();
    for (it = conns->connections.begin (); it != conns->connections.end ();
            it++) {
        int src = (*it).first;
        destinations = (vector < int >*) (*it).second;
        vector < int >subflows_chosen;

        for (unsigned int dst_id = 0; dst_id < destinations->size (); dst_id++) {
            connID++;
            dest = destinations->at (dst_id);

            if (!net_paths[src][dest]) {
                net_paths[src][dest] = top->get_paths (src, dest);
/*                for (vectior<pair<route_t*,route_t*> >::iterator iA =
                            net_paths[src][dest]->begin();
                        iA!=net_paths[src][dest]->end();
                        iA++)
                    (*iA)->erase((*iA)->begin());
                    */
            }

            //we should create multiple connections. How many?
            //if (connID%3!=0)
            //continue;
            //      for (int connection = 0; connection < 1; connection++) {
            if (algo == COUPLED_EPSILON) {
                mtcp = new MultipathTcpSrc (algo, eventlist, NULL, epsilon);
            } else {
                mtcp = new MultipathTcpSrc (algo, eventlist, NULL);
            }
            mtcp->listener = mlistener;
            mptcpVector.push_back(mtcp);
            //uint64_t bb = generateFlowSize();
            //      if (subflow_control)
            //subflow_control->add_flow(src,dest,mtcp);
            subflows_chosen.clear ();
            int it_sub;
            int crt_subflow_count = subflow_count;
            tot_subs += crt_subflow_count;
            cnt_con++;
            it_sub =
                crt_subflow_count >
                net_paths[src][dest]->
                size ()? net_paths[src][dest]->size () : crt_subflow_count;
            int use_all = it_sub == net_paths[src][dest]->size ();

            //if (connID%10!=0)
            //it_sub = 1;
            uint64_t pktpersubflow = pktperflow / it_sub;
            bitset<1024> used;
            used.reset();
            for (int inter = 0; inter < it_sub; inter++) {
                //              if (connID%10==0){
                tcpSrc = new TcpSrc (NULL, NULL, eventlist);
                tcpSrc->set_max_packets(pktpersubflow);
                tcpSnk = new TcpSink ();
                /*}
                   else {
                   tcpSrc = new TcpSrcTransfer(NULL,NULL,eventlist,bb,net_paths[src][dest]);
                   tcpSnk = new TcpSinkTransfer();
                   } */
                //if (connection==1)
                //tcpSrc->set_app_limit(9000);
                tcpSrc->setName ("mtcp_" + ntoa (src) + "_" +
                                 ntoa (inter) + "_" + ntoa (dest));

                logfile.writeName (*tcpSrc);
                tcpSnk->setName ("mtcp_sink_" + ntoa (src) + "_" +
                                 ntoa (inter) + "_" + ntoa (dest));
                logfile.writeName (*tcpSnk);
                tcpRtxScanner.registerTcp (*tcpSrc);

                int choice = 0;
                if (inter!=0) {
                    while (true) {
                        choice = rand () % net_paths[src][dest]->size ();
                        if (used[choice]==0)
                            break;
                    }
                }
                used[choice] = 1;
                subflows_chosen.push_back (choice);

                if (choice >= net_paths[src][dest]->size ()) {
                    printf ("Weird path choice %d out of %u\n", choice,
                            net_paths[src][dest]->size ());
                    exit (1);
                }

#if PRINT_PATHS
                paths << "Route from " << ntoa (src) << " to " <<
                      ntoa (dest) << "  (" << choice << ") -> ";
                print_path (paths, net_paths[src][dest]->at (choice).first);
#endif
                routeout =
                    new route_t (*(net_paths[src][dest]->at (choice).first));
                routeout->push_back (tcpSnk);
                routein = 
                    new route_t (*(net_paths[src][dest]->at (choice).second));
                routein->push_back (tcpSrc);
                extrastarttime = 50 * drand () /100;
                //join multipath connection
                mtcp->addSubflow (tcpSrc);

                if (inter == 0) {
                    mtcp->setName ("multipath" + ntoa (src) + "_" +
                                   ntoa (dest) +")");
                    logfile.writeName (*mtcp);
                }

                tcpSrc->connect (*routeout, *routein, *tcpSnk,
                                 timeFromMs (extrastarttime));
#ifdef PACKET_SCATTER
                tcpSrc->set_paths (net_paths[src][dest]);
                cout << "Using PACKET SCATTER!!!!" << endl << end;
#endif


                sinkLogger.monitorSink (tcpSnk);
                //           }
                //   }
            }
        }
    }

    //ShortFlows* sf = new ShortFlows(2560, eventlist, net_paths,conns,lg, &tcpRtxScanner);
    cout << "Mean number of subflows " << ntoa ((double) tot_subs /
            cnt_con) << endl;
    // Record the setup
    int pktsize = TcpPacket::DEFAULTDATASIZE;
    logfile.write ("# pktsize=" + ntoa (pktsize) + " bytes");
    logfile.write ("# subflows=" + ntoa (subflow_count));
    logfile.write ("# hostnicrate = " + ntoa (HOST_NIC) + " pkt/sec");
    logfile.write ("# corelinkrate = " + ntoa (HOST_NIC * CORE_TO_HOST) +
                   " pkt/sec");
    //logfile.write("# buffer = " + ntoa((double) (queues_na_ni[0][1]->_maxsize) / ((double) pktsize)) + " pkt");
    double rtt = timeAsSec (timeFromUs (RTT));
    logfile.write ("# rtt =" + ntoa (rtt));
    // GO!
    double last = 0.0;
    vector<uint64_t> finished_bytes;
    double tick = 10.0;
    finished_bytes.resize(mptcpVector.size());
    while (eventlist.doNextEvent ()) {
        if (mlistener->_total_finished == cnt_con) break;
        if (timeAsMs(eventlist.now())>last+tick) {
            printf("time=%6.3lf,finish=%6d,",(last =  timeAsMs(eventlist.now())) , mlistener->_total_finished);
            double tot = 0;
            double maxb = 0.0;
            vector<uint64_t>::iterator iB = finished_bytes.begin();
            for (vector<MultipathTcpSrc*>::iterator iA = mptcpVector.begin();  iA!=mptcpVector.end(); iA++,iB++) {
                uint64_t ttmp;
//              printf("% 4.3lf,",((ttmp=(*iA)->compute_total_bytes())-*iB)/(1000*tick));
                double wq;
                ttmp = (*iA)->compute_total_bytes();
                tot += (wq=(ttmp - *iB)); 
                if (wq/(1000*tick) > 125) printf("% 4lf,",wq/(1000*tick));
                if (-*iB + ttmp > maxb) maxb =- *iB + ttmp;
                *iB = ttmp;
            }

            printf("\n,avr=% 8.3lf,",tot/mptcpVector.size()/(1000*tick));
            printf("max= %8.3lf\n",maxb/(1000*tick));

        }
    }
    cout << timeAsMs(eventlist.now()) << endl;
}

string
ntoa (double n)
{
    stringstream s;

    if (n>IAMHOST) {
        n-=IAMHOST;
        s <<"H";
    }
    s << n;
    return s.str ();
}

string
itoa (uint64_t n)
{
    stringstream s;
    if (n>IAMHOST) {
        n-=IAMHOST;
        s <<"H";
    }
    s << n;
    return s.str ();
}
