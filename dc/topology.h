#ifndef TOPOLOGY
#define TOPOLOGY
#include "network.h"

class Topology {
 public:
  virtual vector<route_t*>* get_paths(int src,int dest)=0;
  virtual vector<int>* get_neighbours(int src) = 0;  
};

#endif
