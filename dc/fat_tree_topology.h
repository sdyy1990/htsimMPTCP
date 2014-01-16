#ifndef FAT_TREE
#define FAT_TREE
#include "main.h"
#include "randomqueue.h"
#include "pipe.h"
#include "config.h"
#include "loggers.h"
#include "network.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"
#include <ostream>

#define K 8
#define NK (K*K/2)
#define NC (K*K/4)
#define NSRV (K*K*K/4)
//#define N K*K*K/4

#define HOST_POD_SWITCH(src) (2*src/K)
//#define HOST_POD_ID(src) src%NSRV
#define HOST_POD(src) (src/NC)

#define MIN_POD_ID(pod_id) (pod_id*K/2)
#define MAX_POD_ID(pod_id) ((pod_id+1)*K/2-1)

class FatTreeTopology: public Topology{
 public:
  Pipe * pipes_nc_nup[NC][NK];
  Pipe * pipes_nup_nlp[NK][NK];
  Pipe * pipes_nlp_ns[NK][NSRV];
  RandomQueue * queues_nc_nup[NC][NK];
  RandomQueue * queues_nup_nlp[NK][NK];
  RandomQueue * queues_nlp_ns[NK][NSRV];

  Pipe * pipes_nup_nc[NK][NC];
  Pipe * pipes_nlp_nup[NK][NK];
  Pipe * pipes_ns_nlp[NSRV][NK];
  RandomQueue * queues_nup_nc[NK][NC];
  RandomQueue * queues_nlp_nup[NK][NK];
  RandomQueue * queues_ns_nlp[NSRV][NK];

  Logfile* logfile;
  EventList* eventlist;
  
  FatTreeTopology(Logfile* log,EventList* ev);

  void init_network();
  virtual vector<route_t*>* get_paths(int src, int dest);

  void count_queue(RandomQueue*);
  void print_path(std::ofstream& paths,int src,route_t* route);
  vector<int>* get_neighbours(int src) { return NULL;};  
 private:
  map<RandomQueue*,int> _link_usage;
  int find_lp_switch(RandomQueue* queue);
  int find_up_switch(RandomQueue* queue);
  int find_core_switch(RandomQueue* queue);
  int find_destination(RandomQueue* queue);
};

#endif
