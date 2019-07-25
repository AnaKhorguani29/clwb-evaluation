## Cache line invalidation policy of Intel flush instructions

### Description
This program detects cache line invalidation policy of three Intel flush instructions: *clflush*, *clflushopt* and *clwb*. 
It evaluates whether issuing a flush instruction on a cache line presented in the cache of one or several processors invalidates the cache line from all levels of the cache hierarchy. It considers different possible initial states for the cache line, different scenarios regarding the position of the cache line and the core issuing the flush instruction. Also considering the case of multiple NUMA nodes.

### Requirements
* Hardware support for each instruction can be checked in CPU flags. If any of the instruction is not supported then the program will not compile. In this case, corresponding instruction can be commented in clwb_all_impl.c file
* For the program to give reliable results, hardware prefetchers should be disabled. To do this, MSR tool should be installed: 
```javascript
sudo apt-get install msr-tools
sudo modprobe msr

disable prefetchers: sudo wrmsr -a 0x1a4 15
enable prefetchers: sudo wrmsr -a 0x1a4 0
check whether changes were applied: sudo rdmsr 0x1a4
```
* It's necessary to have installed PAPI and HWLOC libraries. This can be done from the official package repositories for most distributions. 
  * [PAPI](http://icl.cs.utk.edu/papi/)
  * [hwloc](https://www.open-mpi.org/software/hwloc/v2.0/)

* Not all the processors support PAPI native events that are used in the program. Therefore this should be checked with **papi_native_avail** command after installing the library. Used events are:
  * MEM_LOAD_RETIRED: L1_HIT, L2_HIT, L1_MISS, FB_HIT
  * L2_RQSTS: ALL_RFO, RFO_HIT
  * L1D: REPLACEMENT

### Compilation
Makefile is provided in *src* file.

### Execution  
The program can be executed from *src* file by running the following command:
```javascript
./clwb_all.run
```
It can take a parameter to define which instruction should be evaluated: *clflush*, *clflushopt* or *clwb*. By default it evaluates *clwb*.
To allow hardware event counter monitoring, the following command should be executed:
```javascript
sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid' 
```
Restoring the original state after running program:
```javascript
sudo sh -c 'echo 3 >/proc/sys/kernel/perf_event_paranoid' 
```

### Remarks
The output of the program lists all the scenarios that are evaluated. Initial state of the cache line is obtained by multithreading, where each thread either reads or modifies the address of an element. If all the threads read, then we get the cache line that is in a shared state in the cache, otherwise if threads modify the element then it will be in a modified state in one of the cores caches.

### Acknowledgments
This work was done for the first year master project in Université Grenoble Alpes, in LIG - team ERODS. 



