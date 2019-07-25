#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <x86intrin.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#include <pthread.h>
#include <stdbool.h>

#include <papi.h>
#include "hwloc.h"

#include "clwb_all_config.h"

static int results[NB_TESTS];
extern int arr_size;

static int numaNodes;
static int nb_pu;
int * numa1_pu_indexes;
int * numa2_pu_indexes;

static int nb_threads;

typedef struct _object{
  int value;
  char pad[60];
} object;

typedef struct _thread_args{
  int cpuid;
  volatile object * arr;
} thread_args;

//Cache line invalidation while
char* text[] = { "Initial state of the cache line: not in cache,  Operations applied: read,  flush",  
                 "Initial state of the cache line: not in cache,  Operations applied: write, flush",
                 "Initial state of the cache line: shared,        Operations applied: read,  flush",
                 "Initial state of the cache line: shared,        Operations applied: write, flush",
                 "Initial state of the cache line: modified,      Operations applied: read,  flush",
                 "Initial state of the cache line: modified,      Operations applied: write, flush"
               };

typedef enum{
    NO_MULTITHREADING,
    READ_MULTITHREADING,
    WRITE_MULTITHREADING
} scenarios;

typedef enum{
    NO_NUMAS,
    NUMA_ONE,
    NUMA_MIX,
    NUMA_SECOND_MULTITHREADING,
    NUMA_SECOND_FLUSHING,
    NUMA_SECOND
} numa_cases;

static numa_cases numa_versions;

typedef enum{
    CLFLUSH,
    CLFLUSHOPT,
    CLWB
} flush_inst;

char* flush_param[] = { "clflush",
                        "clflushopt",
                        "clwb",
                      };
