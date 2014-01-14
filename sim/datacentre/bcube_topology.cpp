#include "bcube_topology.h"
#include <vector>
#include "string.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include "main.h"

extern int RTT;

string ntoa(double n);
string itoa(uint64_t n);

BCubeTopology::BCubeTopology(Logfile* lg, EventList* ev,FirstFit * fit) {
    logfile = lg;
    eventlist = ev;
    ff = fit;

    init_network();
}

#define SWITCH_ID(srv,level) switch_from_srv(srv,level)
//(level==0?srv/NUM_PORTS:srv%(int)pow(NUM_PORTS,level+1))
#define ADDRESS(srv,level) (level==0?srv%NUM_PORTS:srv/(int)pow(NUM_PORTS,level))

int BCubeTopology::switch_from_srv(int srv, int level) {
    //switch address base is given by level
    int switch_addr = 0;//level * NUM_SRV/NUM_PORTS;

    int crt_n = (int)pow(NUM_PORTS,K-1);
    for (int i=K; i>=0; i--) {
        if (i==level)
            continue;

        switch_addr += crt_n * addresses[srv][i];
        crt_n /= NUM_PORTS;
    }

    return switch_addr;
}

int BCubeTopology::srv_from_address(unsigned int* address) {
    int addr = 0,crt_n = 0;

    crt_n = (int)pow(NUM_PORTS,K);
    //printf("Computing addr from:");
    for (int i=K; i>=0; i--) {
        //    printf("%d ",address[i]);
        addr += crt_n * address[i];
        crt_n /= NUM_PORTS;
    }
    //printf("\n");

    return addr;
}

void BCubeTopology::address_from_srv(int srv, unsigned int* address) {
    int addr = srv,crt_n = 0;

    crt_n = (int)pow(NUM_PORTS,K);
    for (int i=K; i>=0; i--) {
        address[i] = addr / crt_n;
        addr = addr % crt_n;
        crt_n /= NUM_PORTS;
    }
}

vector<int>* BCubeTopology::get_neighbours(int src) {
    unsigned int addr[K+1],i;

    vector<int>* ns = new vector<int>();

    for (int level=0; level<=K; level++) {
        for (i=0; i<=K; i++)
            addr[i] = addresses[src][i];

        for (int i=0; i<NUM_PORTS; i++) {
            addr[level] = i;
            if (addr[level]==addresses[src][level])
                continue;

            ns->push_back(srv_from_address(addr));
        }
    }
    return ns;
}

int BCubeTopology::get_neighbour(int src, int level) {
    unsigned int addr[K+1],i;

    for (i=0; i<=K; i++)
        addr[i] = addresses[src][i];

    //find neighbour at required level
    //NUM_PORTS can't be 1 otherwise we get infinite loop;

    while ((addr[level] = rand()%NUM_PORTS)==addresses[src][level]);

    return srv_from_address(addr);
}

void BCubeTopology::init_network() {
    QueueLoggerSampling* queueLogger;

    assert(NUM_PORTS>0);
    assert(K>=0);
    assert(NUM_SRV==(int)pow(NUM_PORTS,K+1));

    for (int i=0; i<NUM_SRV; i++) {
        address_from_srv(i,addresses[i]);
        for (int k=0; k<K; k++) {
            for (int j=0; j<NUM_SW; j++) {
                pipes_srv_switch[i][j][k] = NULL;
                queues_srv_switch[i][j][k] = NULL;
                pipes_switch_srv[j][i][k] = NULL;
                queues_switch_srv[j][i][k] = NULL;
            }
        }
    }

    //  addresses[i][k] = ADDRESS(i,k);

    for (int k=0; k<=K; k++) {
        //create links for level K
        for (int i=0; i<NUM_SRV; i++) {
            int j;

            j = SWITCH_ID(i,k);

            //printf("SWITCH ID for server %d level %d is %d\n",i,k,j);

            //index k of the address

            queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
            //queueLogger = NULL;
            logfile->addLogger(*queueLogger);

            queues_srv_switch[i][j][k] = new RandomQueue(speedFromPktps(HOST_NIC), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
            queues_srv_switch[i][j][k]->setName("SRV_" + ntoa(i) + "(level_" + ntoa(k)+"))_SW_" +ntoa(j));
            logfile->writeName(*(queues_srv_switch[i][j][k]));

            pipes_srv_switch[i][j][k] = new Pipe(timeFromUs(RTT), *eventlist);
            pipes_srv_switch[i][j][k]->setName("Pipe-SRV_" + ntoa(i) + "(level_" + ntoa(k)+")-SW_" +ntoa(j));
            logfile->writeName(*(pipes_srv_switch[i][j][k]));

            queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
            //queueLogger = NULL;
            logfile->addLogger(*queueLogger);

            queues_switch_srv[j][i][k] = new RandomQueue(speedFromPktps(HOST_NIC), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
            queues_switch_srv[j][i][k]->setName("SW_" + ntoa(j) + "(level_" + ntoa(k)+")-SRV_" +ntoa(i));
            logfile->writeName(*(queues_switch_srv[j][i][k]));

            pipes_switch_srv[j][i][k] = new Pipe(timeFromUs(RTT), *eventlist);
            pipes_switch_srv[j][i][k]->setName("Pipe-SW_" + ntoa(j) + "(level_" + ntoa(k)+")-SRV_" +ntoa(i));
            logfile->writeName(*(pipes_switch_srv[j][i][k]));
        }
    }
}

/*void check_non_null(route_t* rt){
  int fail = 0;
  for (unsigned int i=1;i<rt->size()-1;i+=2)
    if (rt->at(i)==NULL){
      fail = 1;
      break;
    }

  if (fail){
    //    cout <<"Null queue in route"<<endl;
    for (unsigned int i=1;i<rt->size()-1;i+=2)
      printf("%p ",rt->at(i));
    cout<<endl;
    assert(0);
  }
  }*/


route_t* BCubeTopology::bcube_routing(int src,int dest, int* permutation) {
    static int pqid = 0;
    route_t* routeout = new route_t();
    Queue* pqueue = new Queue(speedFromPktps(HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, NULL);
    pqueue->setName("PQueue_" + ntoa(src) + "_" + ntoa(dest)+"_"+ntoa(pqid++));
    routeout->push_back(pqueue);

    unsigned int crt_addr[K+1],crt, level,nsrv;
    int i;
    crt = src;

    for (i=0; i<=K; i++)
        crt_addr[i] = addresses[src][i];

    int aaa = srv_from_address(crt_addr);
    //  printf("CRT is %d, SRV FROM ADDRESS %d\n",crt,aaa);
    assert(crt==aaa);

    for (i=K; i>=0; i--) {
        level = permutation[i];
        nsrv = (int)pow(NUM_PORTS,level+1);

        if (addresses[src][level]!=addresses[dest][level]) {
            //add hop from crt_addr by progressing on id level
            //go upto switch in level and correct digit
            //switch id

            routeout->push_back(queues_srv_switch[crt][SWITCH_ID(crt,level)][level]);
            routeout->push_back(pipes_srv_switch[crt][SWITCH_ID(crt,level)][level]);

            //now correct digit
            crt_addr[level] = addresses[dest][level];
            crt = srv_from_address(crt_addr);

            assert(srv_from_address(crt_addr)==crt);

            routeout->push_back(queues_switch_srv[SWITCH_ID(crt,level)][crt][level]);
            routeout->push_back(pipes_switch_srv[SWITCH_ID(crt,level)][crt][level]);
        }
    }
    return routeout;
}

//i is the level
route_t* BCubeTopology::dc_routing(int src,int dest, int i) {
    int permutation[K+1];
    int m = K,j;

    for (j = i; j>=i-K; j--) {
        permutation[m] = (j+K+1) % (K+1);
        m--;
    }

    return bcube_routing(src,dest,permutation);
}

route_t* BCubeTopology::alt_dc_routing(int src,int dest, int i,int c) {
    int permutation[K+1];
    int m = K,j;

    for (int ti=0; ti<=K; ti++)
        permutation[ti] = ti;

    route_t* path = bcube_routing(src,c,permutation);

    //beware negative!!!
    for (j = i-1; j>=i-1-K; j--) {
        permutation[m] = (j+K+1) % (K+1);
        m--;
    }

    route_t* path2 = bcube_routing(c,dest,permutation);

    //stitch the two paths
    for (unsigned int i=1; i<path2->size(); i++)
        path->push_back(path2->at(i));

    delete path2;

    return path;
}

vector<route_t*>* BCubeTopology::get_paths(int src, int dest) {
    vector<route_t*>* paths = new vector<route_t*>();

    for (int i=K; i>=0; i--) {
        if (addresses[src][i]!=addresses[dest][i])
            paths->push_back(dc_routing(src,dest,i));
        else
            paths->push_back(alt_dc_routing(src,dest,i,get_neighbour(src,i)));
    }
    return paths;
}

void BCubeTopology::print_paths(std::ofstream & p,int src,vector<route_t*>* paths) {
    for (unsigned int i=0; i<paths->size(); i++)
        print_path(p, src, paths->at(i));
}

void BCubeTopology::print_path(std::ofstream &paths,int src,route_t* route) {
    paths << "SRC_" << src << " ";

    for (unsigned int i=1; i<route->size()-1; i+=2) {
        if (route->at(i)!=NULL)
            paths << ((RandomQueue*)route->at(i))->str() << " ";
        else
            paths << "NULL ";
    }
    paths << endl;
}
