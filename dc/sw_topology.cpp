#include "sw_topology.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "main.h"
using namespace std;
extern int RTT;
string ntoa(double n);
string itoa(uint64_t n);
SWTopology::SWTopology() {
}
SWTopology::SWTopology(Logfile *lg, EventList *ev,char * fname) {
    logfile = lg;
    eventlist= ev;

    ifstream ifs;
    ifs.open(fname,std::ifstream::in);
    string s;
    mapQ.clear();
    mx=my=mz = 0;
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
        if (func[0]=='D') {
            ss >> Dim;
            continue;
        }
        if (func[0]=='N') {
            ss >> _n;
            continue;
        }
        if (func[0]=='E') {
            int a,b;
            ss >> a >> b;
            a--;
            b--;
            neighbors[a].push_back(b);
            neighbors[b].push_back(a);
            addlink(a,b);
        }
        if (func[0]=='C') {
            int t,x,y,z;
            ss >> t >> x >> y >> z;
            cX[t] = x;
            cY[t] = y;
            cZ[t] = z;
            if (x>mx) mx = x;
            if (y>my) my = y;
            if (z>mz) mz = z;
        }
        if (func[0] == 'h') {
            int x,y;
            ss >> x >> y;
            x--;
            y--;
            assignedSwitch[x] = y;
            addlink(x+IAMHOST,y);
            neighbors[y].push_back(x+IAMHOST);
        }
    }
    mx++;
    my++;
    mz++;
    if (Dim==3) while (my%3) my++;
    init_network();

}
void SWTopology::init_network() {
}
int SWTopology::get_host_count() {
    return assignedSwitch.size();
}
vector<vector<int> > SWTopology::get_paths_V(int src, int dest) {

    int now = assignedSwitch[src];
    int end = assignedSwitch[dest];
    vector<vector<int> > pathV;
    if (now == end) {
        vector<int> PP;
        PP.push_back(src+IAMHOST);
        PP.push_back(now);
        PP.push_back(dest+IAMHOST);
        pathV.push_back(PP);
    }
    else {
        vector<int> first_hop_choice;
        int mindest = 0x3FFFFFF;
        int mindestid = -1;
        int destnow = virtual_distance(now,end);
        for (int i = 0 ; i < neighbors[now].size(); i++) if (neighbors[now][i]<IAMHOST) {
                int idest;
                if ((idest = virtual_distance(neighbors[now][i],end)) < destnow) {
                    first_hop_choice.push_back(neighbors[now][i]);
                    if (idest< mindest) {
                        mindest = idest;
                        mindestid = first_hop_choice.size() -1;
                    }
                }
            }
        if (mindestid!=0) {
            int t = first_hop_choice[mindestid];
            first_hop_choice[mindestid] = first_hop_choice[0];
            first_hop_choice[0] = t;
        }
        for (int i = 0 ; i < first_hop_choice.size(); i++) {
            pathV.push_back( get_path_with_firsthop(src,dest,first_hop_choice[i]));
        }
    }
    return pathV;
}
vector<pair<route_t *, route_t *> > * SWTopology::get_paths(int src, int dest) { // src and dest refers to HOST , not switch;
    vector<vector<int> > pathV = get_paths_V(src,dest);
    vector<pair<route_t *, route_t *> > *paths = new vector<pair<route_t *, route_t *> > ();
    for (int i = 0 ; i < pathV.size(); i++) {
        route_t * rout, * rin;
        rout = new route_t ();
        rin = new route_t();
        for (int j = 0 ; j < pathV[i].size() -1; j++) {
            rout->push_back(mapQ[make_pair(pathV[i][j],pathV[i][j+1])].first);
            rout->push_back(mapQ[make_pair(pathV[i][j],pathV[i][j+1])].second);
        }
        for (int j = pathV[i].size() -1 ; j >1 ; j--) {
            rin->push_back(mapQ[make_pair(pathV[i][j],pathV[i][j-1])].first);
            rin->push_back(mapQ[make_pair(pathV[i][j],pathV[i][j-1])].second);
        }
        rout->erase(rout->begin());
        rin->erase(rin->begin());
        paths->push_back(make_pair(rout,rin));

    }
    return paths;
}
vector<int>  SWTopology::get_path_with_firsthop(int src,int dest, int first_hop) {
    vector<int> ans;
    int now = first_hop;
    int end = assignedSwitch[dest];
    ans.push_back(src+IAMHOST);
    ans.push_back(assignedSwitch[src]);
    ans.push_back(now);
    while (now!=end) {
        std::cout <<  "   end  " << end << "    now" << now << endl;
        int mindest = 0x3FFFFFFF;
        int nxt ;
        int nowdst;
        for (int i = 0 ; i < neighbors[now].size(); i++) if (neighbors[now][i] < IAMHOST) {
                if ( (nowdst = virtual_distance(neighbors[now][i],end))<mindest) {
                    mindest = nowdst;
                    nxt = neighbors[now][i];
                }
            }
        ans.push_back(nxt);
        now = nxt;


    }
    ans.push_back(dest+IAMHOST);
    return ans;
}

int SWTopology::virtual_distance(int src, int dest) {
    int dx = cX[src]-cX[dest];
    int dy = cY[src]-cY[dest];
    int dz = cZ[src]-cZ[dest];
#define MMM(t,m) ((t<m-t)?t:m-t)
    int bx = MMM(dx,mx);
    int by = MMM(dy,my);
    int bz = MMM(dz,mz);

    if (Dim == 3) {
        return bx*bx*3+ by*by + bz * bz *2;
    }
    if (Dim == 2)
        return bx*bx + by*by;
    if (Dim == 3)
        return bx*bx;
}
void SWTopology::addlink(int src, int dest) {
    RandomQueue * r1 = new RandomQueue(speedFromPktps(HOST_NIC),memFromPkt(RANDOM_BUFFER+SWITCH_BUFFER), *eventlist,NULL,memFromPkt(RANDOM_BUFFER));
    Pipe * p1 = new Pipe(timeFromUs(RTT),*eventlist);
    r1->setName(itoa(src)+"->"+itoa(dest));
    mapQ[make_pair(src,dest)] = make_pair(r1,p1);

    r1 = new RandomQueue(speedFromPktps(HOST_NIC),memFromPkt(RANDOM_BUFFER+SWITCH_BUFFER), *eventlist,NULL,memFromPkt(RANDOM_BUFFER));
    p1 = new Pipe(timeFromUs(RTT),*eventlist);
    r1->setName(itoa(dest)+"->"+itoa(src));
    mapQ[make_pair(dest,src)] = make_pair(r1,p1);

}
