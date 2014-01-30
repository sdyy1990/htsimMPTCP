#ifndef S2_TOPO_H
#define S2_TOPO_H
#include "main.h"
#include "pipe.h"
#include "topology.h"
#include "eventlist.h"
#include "randomqueue.h"
#include "loggers.h"
#include "sw_topology.h"
class S2Topology: public SWTopology {
public: 
    int L;
    vector<vector<int> > coor;
    map<int,double> hostcoor;
    S2Topology(Logfile * log, EventList *ev, char * fname);
    map< pair<int,int> , vector<int> > mapP;

private:
    int virtual_distance(int src, int dest);
    vector<int> get_path_with_firsthop(int , int , int );
};
    
#endif 
