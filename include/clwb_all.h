#ifndef __CLWB_ALL_H__
#define __CLWB_ALL_H__

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


void check_load (int * result, scenarios sc_1, volatile object * array, int array_size, flush_inst flush);

void check_store (int * result, scenarios sc_1, object * array, int array_size, flush_inst flush);

void check_prefetchers (volatile object * array, int array_size);

void init_results();

#endif
