CC = g++
CFLAGS =  -Wall -g
CRT=`pwd`
INCLUDE= -I/$(CRT)/htsimlib -I$(CRT) 
#-I$(CRT)/ksp -I$(CRT)/ksp/boost
LIB=-L./htsimlib
#-Lksp

all:	htsim_dc
htsim_dc: main.o subflow_control.o connection_matrix.o shortflows.o sw_topology.o
	$(CC) $(CFLAGS) $^ $(LIB) -lhtsim -o htsim_dc


main.o: main.cpp htsimlib/htsimlib.a
	$(CC) $(INCLUDE) $(CFLAGS) -c main.cpp 

htsimlib/htsimlib.a:

shortflows.o: shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c shortflows.cpp 

connection_matrix.o: connection_matrix.cpp 
	$(CC) $(INCLUDE) $(CFLAGS) -c connection_matrix.cpp 


subflow_control.o: subflow_control.cpp subflow_control.h 
	$(CC) $(INCLUDE) $(CFLAGS) -c subflow_control.cpp

fat_tree_topology.o: fat_tree_topology.cpp fat_tree_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c fat_tree_topology.cpp

sw_topology.o: sw_topology.cpp sw_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c sw_topology.cpp

s2_topology.o: sw_topology.o s2_topology.h s2_topology.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c s2_topology.cpp
jelly_topology.o: sw_topology.o jelly_topology.h jelly_topology.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c jelly_topology.cpp
clean:	
	rm *.o htsim_dc
