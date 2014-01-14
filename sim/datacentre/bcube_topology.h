#ifndef BCUBE
#define BCUBE
#include "main.h"
#include "randomqueue.h"
#include "pipe.h"
#include "config.h"
#include "loggers.h"
#include "network.h"
#include "firstfit.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"
#include <ostream>

//number of levels
#define K 2
//number of ports per SWITCH
#define NUM_PORTS 5
//total number of servers
#define NUM_SRV 125
//switches per level
//=(K+1)*NUM_SRV/NUM_PORTS
#define NUM_SW 75

class BCubeTopology: public Topology{
 public:
  Pipe * pipes_srv_switch[NUM_SRV][NUM_SW][K+1];
  Pipe * pipes_switch_srv[NUM_SW][NUM_SRV][K+1];
  RandomQueue * queues_srv_switch[NUM_SRV][NUM_SW][K+1];
  RandomQueue * queues_switch_srv[NUM_SW][NUM_SRV][K+1];
  unsigned int addresses[NUM_SRV][K+1];

  FirstFit * ff;
  Logfile* logfile;
  EventList* eventlist;
  
  BCubeTopology(Logfile* log,EventList* ev,FirstFit* f);

  void init_network();
  virtual vector<route_t*>* get_paths(int src, int dest);
 

  void count_queue(RandomQueue*);
  void print_path(std::ofstream& paths,int src,route_t* route);
  void print_paths(std::ofstream& p, int src, vector<route_t*>* paths);
  vector<int>* get_neighbours(int src);
 private:
  map<RandomQueue*,int> _link_usage;

  int srv_from_address(unsigned int* address);
  void address_from_srv(int srv, unsigned int* address);
  int get_neighbour(int src, int level);
  int switch_from_srv(int srv, int level);

  route_t* bcube_routing(int src,int dest, int* permutation);
  route_t* dc_routing(int src,int dest, int i);  
  route_t* alt_dc_routing(int src,int dest, int i,int c);  
};

#endif
