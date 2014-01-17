#include "s2_topology.h"
#include <iostream>
S2Topology::S2Topology(Logfile * log, EventList *ev, char * fname) {
    logfile = log;
    eventlist = ev;
    ifstream ifs;
    ifs.open(fname, std::ifstream::in);
    string s;
    mapQ.clear();
    while (true) {
        getline(ifs,s);
        if (s.size() <=0) break;
        stringstream ss(s); string func;  ss >> func;
        if (func[0] == 'N') {
            ss >> _switch;
            neighbors.resize(_switch);
            coor.resize(_switch);
        }
        if (func[0] == 'L') {
            ss >> L;
        }
        if (func[0] == 'E') {
            int a,b;
            ss >> a >> b;
            a--; b--;
            neighbors[a].push_back(b); neighbors[b].push_back(a);
            addlink(a,b);
        }
        if (func[0] == 'C') {
            int t; ss >> t;
            for (int i = 0 ; i < L ; i ++) {
                int k; ss >>k; coor[t].push_back(k);
            }
        }
        if (func[0] =='H') {
            int x, y; double cor;
            ss >> x >> y >> cor; x--; y--;
            assignedSwitch[x] = y; 
            hostcoor[x] = cor;
            addlink(x+IAMHOST,y);
            neighbors[y].push_back(x+IAMHOST);
        }
    }
}
int S2Topology::virtual_distance(int src, int dest) {
//    std::cout <<" This is S2 Distance";
    int tot = 0x3FFFFFFF;
    for (int i = 0 ; i < L ; i++) {
        int t = coor[src][i] - coor[dest][i];
        if (t<0) t = -t;
        if (t+t>1000000) t= 1000000-t;
        if( t< tot) tot = t;
    }
    return tot;
       
}


route_t * S2Topology::get_path_with_firsthop(int src,int dest, int first_hop) {
    route_t * routeout;
    routeout = new route_t();
    int now = first_hop;
    int end = assignedSwitch[dest];
    routeout->push_back(mapQ[make_pair(src+IAMHOST,assignedSwitch[src])].first);
    routeout->push_back(mapQ[make_pair(src+IAMHOST,assignedSwitch[src])].second);
    routeout->push_back(mapQ[make_pair(assignedSwitch[src],now)].first);
    routeout->push_back(mapQ[make_pair(assignedSwitch[src],now)].second);
    cout << endl;
    while (now!=end) {
        int mindest = 0x3FFFFFFF;
        int nxt ;
        int nowdst = virtual_distance(now,end);
        for (int i = 0 ; i < neighbors[now].size(); i++) if (neighbors[now][i] < IAMHOST) {
                int nei;
                if ( (virtual_distance(nei = neighbors[now][i],end)<nowdst))
                    for (int j = 0 ; j < neighbors[nei].size(); j++)
                        if (neighbors[nei][j] < IAMHOST)
                        {
                            int nndst = virtual_distance(neighbors[nei][j],end);
                            if (nndst < mindest) {
                              nxt = neighbors[now][i];
                              mindest = nndst;
                       }
                }
            }
        routeout->push_back(mapQ[make_pair(now,nxt)].first);
        routeout->push_back(mapQ[make_pair(now,nxt)].second);
        now = nxt;


    }
    routeout->push_back(mapQ[make_pair(now,dest+IAMHOST)].first);
    routeout->push_back(mapQ[make_pair(now,dest+IAMHOST)].second);
    return routeout;
}
