#ifndef MAIN_H
#define MAIN_H

#include <string>


#define HOST_NIC 8000 // host nic speed in pps
#define CORE_TO_HOST 10

//basic setup!

#define NI 2        //Number of intermediate switches
#define NA 6        //Number of aggregation switches
#define NT 9        //Number of ToR switches (180 hosts)

#define NS 20        //Number of servers per ToR switch
#define TOR_AGG2(tor) (10*NA - tor - 1)%NA


/*
#define NI 4        //Number of intermediate switches
#define NA 9        //Number of aggregation switches
#define NT 18        //Number of ToR switches (180 hosts)

#define NS 10        //Number of servers per ToR switch
#define TOR_AGG2(tor) (tor+1)%NA
*/
/*
#define NI 10        //Number of intermediate switches
#define NA 6        //Number of aggregation switches
#define NT 30        //Number of ToR switches (180 hosts)

#define NS 6        //Number of servers per ToR switch
#define TOR_AGG2(tor) (tor+1)%NA
*/

#define NT2A 2      //Number of connections from a ToR to aggregation switches

#define TOR_ID(id) N+id
#define AGG_ID(id) N+NT+id
#define INT_ID(id) N+NT+NA+id
#define HOST_ID(hid,tid) tid*NS+hid

#define HOST_TOR(host) host/NS
#define HOST_TOR_ID(host) host%NS
#define TOR_AGG1(tor) tor%NA


#define SWITCH_BUFFER 100
#define RANDOM_BUFFER 2
#define FEEDER_BUFFER 1000

#endif
