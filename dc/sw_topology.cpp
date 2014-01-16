#include "sw_topology.h"
#include <fstream>
#include <sstream>
#include "main.h"
using namespace std;
extern int RTT;
string ntoa(double n);
string itoa(uint64_t n);
SWTopology::SWTopology(Logfile *lg, EventList *ev,char * fname) {
    logfile = lg;
    eventlist= ev;
    
    ifstream ifs;
    ifs.open(fname,std::ifstream::in);
    string s;
    mapQ.clear();
    while (true) {
        getline(ifs,s);
        if (s.size()<=0) break;
        stringstream ss(s);
        string func;
        ss >> func;
        if (func[0]=='N') { 
            ss >> _switch; 
            cX.resize(_switch);
            cY.resize(_switch);
            cZ.resize(_switch);
            neighbors.resize(_switch);
            assignedSwitch.clear(); 
            continue;    
        }
        if (func[0]=='D') { ss >> Dim; continue;    }
        if (func[0]=='N') { ss >> _n; continue;    }
        if (func[0]=='E') {
            int a,b;
            ss >> a >> b; a--; b--;
            neighbors[a].push_back(b);
            neighbors[b].push_back(a);
            addlink(a,b);
        }
        if (func[0]=='C') {
            int t,x,y,z;
            ss >> t >> x >> y >> z;
            cX[t] = x; cY[t] = y; cZ[t] = z;
        }
        if (func[0] == 'h') {
            int x,y;
            ss >> x >> y; x--; y--;
            assignedSwitch[x] = y;
            addlink(x+IAMHOST,y);
            neighbors[y].push_back(x+IAMHOST);
        }
    }
   
    init_network();

}
void SWTopology::init_network() {

}

vector<route_t *> * SWTopology::get_paths(int src, int dest) {
    return NULL;
}
int SWTopology::virtual_distance(int src, int dest) {
return 0;
}
void SWTopology::addlink(int src, int dest) {
    RandomQueue * r1 = new RandomQueue(speedFromPktps(HOST_NIC),memFromPkt(RANDOM_BUFFER+SWITCH_BUFFER), *eventlist,NULL,memFromPkt(RANDOM_BUFFER));
    Pipe * p1 = new Pipe(timeFromUs(RTT),*eventlist);
    mapQ[make_pair(src,dest)] = make_pair(r1,p1);

    r1 = new RandomQueue(speedFromPktps(HOST_NIC),memFromPkt(RANDOM_BUFFER+SWITCH_BUFFER), *eventlist,NULL,memFromPkt(RANDOM_BUFFER));
    p1 = new Pipe(timeFromUs(RTT),*eventlist);
    mapQ[make_pair(dest,src)] = make_pair(r1,p1);
        
}
