#ifndef JELLY_TOPO_H
#define JELLY_TOPO_H
#include "main.h"
#include "pipe.h"
#include "topology.h"
#include "eventlist.h"
#include "randomqueue.h"
#include "loggers.h"
#include "sw_topology.h"
class JellyTopology: public SWTopology {
public: 
    JellyTopology(Logfile * log, EventList *ev, char * fname , char * pathname);

    vector< vector<int> > get_paths_V( int src, int dest); 
    map< pair<int,int> , vector< vector< int> > >  mapP;
    void set_maxpathcount(int n);
    int max_pathcount;
};
#endif 
