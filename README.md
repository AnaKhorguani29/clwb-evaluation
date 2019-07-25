## Cache line invalidation policy for Intel flush instructions

### Description

This program detects cache line invalidation policy for three Intel flush instructions: *clflush*, *clflushopt* and *clwb*. 

### Requirements

* Hardware support for each instruction can be checked in cpu flags. If any of the instruction is not supported then the program will not compile. (In this case, corresponding instruction can be commented in clwb_all_impl.c file)

* For the program to give reliable results hardware prefetchers should be disabled. To do this, MSR tool should be installed: 
```javascript
sudo apt-get install msr-tools
sudo modprobe msr

disable prefetchers: sudo wrmsr -a 0x1a4 15
enable prefetchers: sudo wrmsr -a 0x1a4 0
check if the changes were applied: sudo rdmsr 0x1a4
```

* It's necessary to have insalled PAPI library. [PAPI](http://icl.cs.utk.edu/papi/)

* Not all the processors support PAPI native events that are used in the program. Therefore this should be checked with **papi_native_avail** command after installing the library. Used events are:
  * MEM_LOAD_RETIRED: L1_HIT, L2_HIT, L1_MISS, FB_HIT
  * L2_RQSTS: ALL_RFO, RFO_HIT
  * L1D: REPLACEMENT

* It's necessary to have insalled hwloc library. [hwloc](https://www.open-mpi.org/software/hwloc/v2.0/)


### Compilation

Makefile is provied in *src* file.

### Execution  

```javascript
The program can executed from *src* file by running the following command: **./clwb_all.run** 
```

It can take parameter defining which instruction to evaluate: *clflush*, *clflushopt* or *clwb*. By default it evaluates *clwb*.


To allow hardware event counter monitiring, the fillowing command should be executed:
```javascript
sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid' 
```

Restoring the original state after runnung program:
```javascript
sudo sh -c 'echo 3 >/proc/sys/kernel/perf_event_paranoid' 
```

