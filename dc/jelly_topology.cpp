#include "jelly_topology.h"
#include <iostream>

JellyTopology::JellyTopology(Logfile * lg,
EventList * ev, char * fname, char * pathname) {
    logfile = lg; eventlist = ev;
    ifstream ifs; ifs.open(fname,std::ifstream::in);
    string s;
    mapQ.clear();
    mapP.clear();
    while (true) {
        getline(ifs,s); if (s.size()<=0) break;
        stringstream ss(s);
        string func; ss >> func;
        if (func[0] == 'N') {
            ss >> _switch;
            neighbors.resize(_switch);
            assignedSwitch.clear();
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

        if (func[0] == 'H' || func[0]=='h') {
            int x,y;   ss >> x >> y;
            x--;   y--;
            assignedSwitch[x] = y;
            addlink(x+IAMHOST,y);
            neighbors[y].push_back(x+IAMHOST);
        }
    }
    ifs.close();
    ifs.open(pathname,std::ifstream::in);
    while (true) {
        getline(ifs,s);
        if (s.size()<3) break;
        stringstream ss (s);
        vector<int> V; int k;
        while (ss >> k) V.push_back(k-1);
        pair<int,int> ip = make_pair(*(V.begin()),*(V.rbegin()));
        mapP[ip].push_back(V);
        if (0)
        for (vector< vector<int> >::iterator iv= mapP[ip].begin(); iv!=mapP[ip].end(); iv++){
        for (vector<int>::iterator ivv = iv->begin(); ivv!=iv->end(); ivv++) {
            cout << *ivv <<" ";
        }
        cout << endl;
        }
        
    }
}
void JellyTopology::set_maxpathcount(int n) {
    max_pathcount = n;
}
vector<route_t *> * JellyTopology::get_paths(int src, int dest) {
    int now = assignedSwitch[src];
    int end = assignedSwitch[dest];
    vector<route_t * > * paths = new vector<route_t *> ();
    if (now == end) {
        route_t * routeout;
        routeout = new route_t();
        routeout->push_back(mapQ[make_pair(src+IAMHOST,now)].first);
        routeout->push_back(mapQ[make_pair(src+IAMHOST,now)].second);
        routeout->push_back(mapQ[make_pair(now,dest+IAMHOST)].first);
        routeout->push_back(mapQ[make_pair(now,dest+IAMHOST)].second);
        paths -> push_back(routeout);
        return paths;
    }
    vector< vector<int>  > * pV;
    pV = &(mapP[make_pair(now,end)]);
    int cnt = 0;
    for (vector< vector<int> >::iterator iV = (pV->begin()); iV!=pV->end() && cnt < max_pathcount; iV++) {
        route_t * routeout; routeout = new route_t();
        routeout->push_back(mapQ[make_pair(src+IAMHOST,now)].first);
        routeout->push_back(mapQ[make_pair(src+IAMHOST,now)].second);
        for (int i = 0 ; i < iV->size() -1 ;   i++) {
            routeout->push_back(mapQ[make_pair((iV->at(i)),(iV->at(i+1)))].first);
            routeout->push_back(mapQ[make_pair(iV->at(i),iV->at(i+1))].second);
        }
        routeout->push_back(mapQ[make_pair(end,dest+IAMHOST)].first);
        routeout->push_back(mapQ[make_pair(end,dest+IAMHOST)].second);
        paths->push_back(routeout);

    }
    return paths;
}


