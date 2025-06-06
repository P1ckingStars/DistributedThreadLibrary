# Distributed Thread Library
## To Build
requirement:
> System: Linux (with page size of 4096)
> Dependency: rpclib (there is a compiled lib in dependency folder, but in case you are not x86 machine you can recompile from the link: https://github.com/rpclib/rpclib)
> Compiler: g++ (or other c++ compiler)
Now you can go to build folder and run make:
```
git clone https://github.com/P1ckingStars/DistributedThreadLibrary.git
cd DistributedThreadLibrary
mkdir build
cd build
cmake ../
cmake --build .
```
You now have all the object files you need to compile with your code
## To use
Include header neccessary header files to use the library. You don't write main as you usually do for most of C++ program, instead write (see `examples/test1.cpp`):
```
void dsm_main1(void * args) {
  // your code
}
```
Then after you compile your code into object file:
```
g++ -C yourcode.cpp
```
You can now link every object file in build and rpclib to your object file into executable using linker.ld script. See CMakeList.txt or makefiles in build_nsp.

## To run
You can play around by running `buidl/simulate.sh`,and then go to simulate folder run `./node0/run.sh` and `./node1/run.sh`
The command line argument is shown in the script file
```
./dsmapp <is_master> <number of pages needed> <ip addr> <port> <optional ip addr> <optional port>
```
For each clusters of nodes you want to run, you can only have one master, set <is_master> to 0 means master, other value means normal nodes. 
<Number of pages needed> currently unused.
<ip addr> and <port> is the address and port you want to use for the node.
<optional ip addr> and <optional port> is not needed for master but will be needed for normal nodes, this indicates the machine you want to connect to (normally its master, but you can connect to any nodes)
Note: Only the pages on master is marked as valid when initialized, so a cluster without a master may not work!
