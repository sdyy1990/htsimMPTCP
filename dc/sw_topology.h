#ifndef SW_TOPO_H
#define SW_TOPO_H
#include "main.h"
#include "pipe.h"
#include "network.h"
#include "topology.h"
#include "eventlist.h"
#include "randomqueue.h"
#include "loggers.h"
static const int MAXX_HOST = 1024;
static const int MAXX_SWITCH  = 1024;
static const int MAXX_PORT = 64;

class SWTopology: public Topology {
public:
    int Dim,dx,dy,dz;
    int mx,my,mz;
    int _port, _switch , _n;
    map< pair<int,int> , pair<RandomQueue * , Pipe* > > mapQ;
    vector< vector< int> > neighbors;
    map<int,int> assignedSwitch;
    vector<int> cX,cY,cZ;
    Logfile * logfile;
    EventList * eventlist;
    void init_network();
    virtual    vector<route_t *> * get_paths(int src, int dest);
    virtual int get_host_count();
    vector< int > * get_neighbours(int src) { return NULL; };
    SWTopology(Logfile * log, EventList *ev, char * fname) ;
    SWTopology();
    virtual int virtual_distance(int src, int dest);
    void addlink(int a, int b);
    virtual route_t * get_path_with_firsthop(int, int , int );
    
};




#endif 
