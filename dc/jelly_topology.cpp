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
vector< vector< int > > JellyTopology::get_paths_V(int src, int dest) {
   int now = assignedSwitch[src];
   int end = assignedSwitch[dest];
   vector< vector< int> > ans;
   if (now == end) {
      vector<int> VA;
      VA.push_back(src+IAMHOST);
      VA.push_back(now);
      VA.push_back(dest+IAMHOST);
      ans.push_back(VA);
   }
   vector<vector<int> > * pV;
   pV = &(mapP[make_pair(now,end)]);
   int cnt = 0 ;
   for (vector<vector<int> >:: iterator iV = (pV->begin()); iV!=(pV->end()) && cnt < max_pathcount; iV++,cnt++) {
        vector<int> tmp(*iV);
        tmp.insert(tmp.begin(),src+IAMHOST);
        tmp.push_back(dest+IAMHOST);
        ans.push_back(tmp);
   //     for (vector<int>::iterator iVV= tmp.begin(); iVV!=tmp.end(); iVV++) 
//            cout <<"."<<*iVV;
//            cout << endl;
   }
//   cout << "here " << src <<" " << dest<<" " << endl;
   return ans;
}

