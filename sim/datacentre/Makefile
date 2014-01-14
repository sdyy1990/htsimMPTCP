CC = g++
CFLAGS = -O6 -Wall -g
CRT=`pwd`
INCLUDE= -I/$(CRT)/.. -I$(CRT) 
#-I$(CRT)/ksp -I$(CRT)/ksp/boost
LIB=-L.. 
#-Lksp

all:	htsim_dc
htsim_dc: main.o firstfit.o ../libhtsim.a fat_tree_topology.o subflow_control.o connection_matrix.o shortflows.o star_topology.o
	$(CC) $(CFLAGS) firstfit.o main.o fat_tree_topology.o subflow_control.o connection_matrix.o shortflows.o star_topology.o $(LIB) -lhtsim -o htsim_dc

main.o: main.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c main.cpp 

star_topology.o: star_topology.cpp star_topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c star_topology.cpp 

shortflows.o: shortflows.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c shortflows.cpp 

connection_matrix.o: connection_matrix.cpp 
	$(CC) $(INCLUDE) $(CFLAGS) -c connection_matrix.cpp 

firstfit.o: firstfit.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c firstfit.cpp

subflow_control.o: subflow_control.cpp subflow_control.h 
	$(CC) $(INCLUDE) $(CFLAGS) -c subflow_control.cpp

fat_tree_topology.o: fat_tree_topology.cpp fat_tree_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c fat_tree_topology.cpp

vl2_topology.o: vl2_topology.cpp vl2_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c vl2_topology.cpp

bcube_topology.o: bcube_topology.cpp bcube_topology.h topology.h
	$(CC) $(INCLUDE) $(CFLAGS) -c bcube_topology.cpp


clean:	
	rm *.o htsim_dc
