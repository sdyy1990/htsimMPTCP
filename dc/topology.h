#ifndef TOPOLOGY
#define TOPOLOGY
#include "network.h"

class Topology {
 public:
  virtual vector<pair<route_t*,route_t*> >* get_paths(int src,int dest)=0;
  virtual vector<int>* get_neighbours(int src) = 0;  
  virtual int get_host_count() = 0 ;
};

#endif
