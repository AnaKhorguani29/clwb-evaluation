#include "../include/clwb_all.h"

/*Initialization of results array*/
void init_results(){
  for(int i=0; i<NB_TESTS; i++){
    results[i] = -10;
  }
}

/*Read function for reader threads*/
void * read_array (void * arg){
  thread_args * args = (thread_args *) arg;
  int cpuid = args->cpuid;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  if(numa_versions == NUMA_ONE || numa_versions == NUMA_SECOND_FLUSHING){
    CPU_SET(numa1_pu_indexes[cpuid], &cpuset);
  }
  else if(numa_versions == NUMA_SECOND_MULTITHREADING || numa_versions ==NUMA_SECOND){
    CPU_SET(numa2_pu_indexes[cpuid], &cpuset);
  }
  else if(numa_versions == NUMA_MIX){
    if(cpuid%2 == 0){
      CPU_SET(numa1_pu_indexes[cpuid], &cpuset);
    }
    else{
      CPU_SET(numa2_pu_indexes[cpuid], &cpuset);
    }
  }

  if (numa_versions != NO_NUMAS) {
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)){
       fprintf(stderr, "process pinning failed!\n");
       exit(1);
    }
  }

  int temp;
  for(int i=0; i < arr_size ; i++){
    temp =args->arr[i].value;
  }
}

/*Write function for writer threads*/
void * write_array (void * arg){
  thread_args * args = (thread_args *) arg;
  int cpuid = args->cpuid;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  if(numa_versions == NUMA_ONE || numa_versions == NUMA_SECOND_FLUSHING){
    CPU_SET(numa1_pu_indexes[cpuid], &cpuset);
  }
  else if(numa_versions == NUMA_SECOND_MULTITHREADING || numa_versions ==NUMA_SECOND){
    CPU_SET(numa2_pu_indexes[cpuid], &cpuset);
  }
  else if(numa_versions == NUMA_MIX){
    if(cpuid%2 == 0){
      CPU_SET(numa1_pu_indexes[cpuid], &cpuset);
    }
    else{
      CPU_SET(numa2_pu_indexes[cpuid], &cpuset);
    }
  }

  if (numa_versions != NO_NUMAS) {
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)){
       fprintf(stderr, "process pinning failed!\n");
       exit(1);
    }
  }

  for(int i=0; i < arr_size ; i++){
    args->arr[i].value = 2;
  }
}

/*Flushing cache line after reading in main loop*/
void check_load (int * result, scenarios sc_1, volatile object * array, int array_size, flush_inst flush){

  int EventSet = PAPI_NULL;
  unsigned int native = 0x0;

  int val_size = 3;
  long_long values[val_size];
  long_long final_values[val_size];

  if (PAPI_create_eventset(&EventSet) != PAPI_OK)
    printf("PAPI error on event creation: %d\n", 1);

  /*Identifying counters to be used*/
  if (PAPI_event_name_to_code("MEM_LOAD_RETIRED.L1_HIT",&native) != PAPI_OK)
    printf("PAPI error on event defining: %d\n", 2);

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
      printf("PAPI error on event adding: %d\n", 3);

  if (PAPI_event_name_to_code("L1D.REPLACEMENT",&native) != PAPI_OK)
      printf("PAPI error on event defining: %d\n", 4);

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
        printf("PAPI error on event adding: %d\n", 5);

  if (PAPI_event_name_to_code("MEM_LOAD_RETIRED.L2_HIT",&native) != PAPI_OK)
    printf("PAPI error on event defining: %d\n", 6);

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
      printf("PAPI error on event adding: %d\n", 7);

  if (PAPI_start(EventSet) != PAPI_OK)
    printf("PAPI error on start counting: %d\n", 20);

  int tmp;
  int exp_behavior = 0;
  int no_invalidation = 0;

  pthread_t * threads;
  threads=malloc(nb_threads*sizeof(pthread_t));

  for(int k=0; k<NB_RUNS; k++){

    if(sc_1 != NO_MULTITHREADING){

      /*In case we are checking multithreading*/
      for(int i = 0; i < nb_threads; i++){

        thread_args * arg = malloc(sizeof(thread_args));
        arg->arr = array;
        arg->cpuid =i;

        if(sc_1 == READ_MULTITHREADING){
          if(pthread_create (&threads[i], NULL, read_array, (void*) arg) != 0){
            fprintf(stderr,"Failed to create thread for read multithreading\n");
            exit(1);
          }
        }
        else if(sc_1 == WRITE_MULTITHREADING){
          if(pthread_create (&threads[i], NULL, write_array, (void*) arg) != 0){
            fprintf(stderr,"Failed to create thread  for write multithreading\n");
            exit(1);
          }
        }
      }

      for(int i = 0; i < nb_threads; i++){
         if(pthread_join(threads[i], NULL)){
             fprintf(stderr, "Could not join thread %d\n", i);
             exit(1);
         }
      }
    }

    if (PAPI_read(EventSet, values) != PAPI_OK)
        printf("PAPI error on counter reading: %d\n", 21);

    for(int i = 0; i<val_size; i++){
        final_values[i] = values[i];
    }

    /*main loop for which we read counters*/
    for(int i=0; i < array_size ; i++){

      tmp = array[i].value;

      asm volatile ("" ::: "memory");

      if(flush==CLFLUSH){
        _mm_clflush(&array[i]);
      }
      else if(flush==CLFLUSHOPT){
        _mm_clflushopt(&array[i]);
      }
      else{
        _mm_clwb(&array[i]);
      }

      _mm_mfence();
      asm volatile ("" ::: "memory");

      tmp = array[i].value;

    }

    if (PAPI_read(EventSet, values) != PAPI_OK)
          printf("PAPI error on counter reading: %d\n", 27);

    for(int i = 0; i<val_size; i++){
        final_values[i] = values[i] - final_values[i];
    }

    if(k>1){
      if( ((final_values[0] + final_values[1]) >= 2*array_size - (2*array_size*PROXIMITY)/100) &&
          ((final_values[0] + final_values[1]) <= 2*array_size + (2*array_size*PROXIMITY)/100) ){
        exp_behavior++;
      }
      if( (final_values[0] >= array_size - (array_size*PROXIMITY)/100) || (final_values[2] >= array_size - (array_size*PROXIMITY)/100)
           || ( (final_values[0] + final_values[2]) >= array_size - (array_size*PROXIMITY)/100) ){
        no_invalidation++;
      }
    }
  }

  if(NB_ACCEPTED_RUNS>exp_behavior){
    * result = -1;
  }
  else{
    if(NB_ACCEPTED_RUNS <= no_invalidation){
      * result = 1;
    }
    else{
      * result = 0;
    }
  }

  if (PAPI_stop(EventSet, values) != PAPI_OK)
       printf("PAPI error on stopping counter reading: %d\n", 30);

  if (PAPI_destroy_eventset(&EventSet) == PAPI_EISRUN)
    printf("PAPI error on destroying eventset: %d\n", 31);

  free(threads);
  return;
}


/*Flushing cache line after modifing in main loop*/
void check_store (int * result, scenarios sc_1, object * array, int array_size, flush_inst flush){

  int EventSet = PAPI_NULL;
  unsigned int native = 0x0;

  int val_size = 2;
  long_long values[val_size];
  long_long final_values[val_size];

  if (PAPI_create_eventset(&EventSet) != PAPI_OK)
    printf("PAPI error: %d\n", 101);

  if (PAPI_event_name_to_code("L2_RQSTS.ALL_RFO",&native) != PAPI_OK)
      printf("PAPI error: %d\n",102);

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
        printf("PAPI error: %d\n", 103);

  if (PAPI_event_name_to_code("L2_RQSTS.RFO_HIT",&native) != PAPI_OK)
    printf("PAPI error: %d\n", 104);

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
      printf("PAPI error: %d\n", 105);

  if (PAPI_start(EventSet) != PAPI_OK)
    printf("PAPI error: %d\n", 120);

  int exp_behavior = 0;
  int no_invalidation = 0;

  pthread_t * threads;
  threads=malloc(nb_threads*sizeof(pthread_t));

  for(int k=0; k<NB_RUNS; k++){

    if(sc_1 != NO_MULTITHREADING){

      for(int i = 0; i < nb_threads; i++){
        thread_args * arg = malloc(sizeof(thread_args));
        arg->arr = array;
        arg->cpuid =i;

        if(sc_1 == READ_MULTITHREADING){
          if(pthread_create (&threads[i], NULL, read_array, (void*) arg) != 0){
            fprintf(stderr,"Failed to create thread\n");
            exit(1);
          }
        }
        else if(sc_1 == WRITE_MULTITHREADING){
          if(pthread_create (&threads[i], NULL, write_array, (void*) arg) != 0){
            fprintf(stderr,"Failed to create thread\n");
            exit(1);
          }
        }
      }
      for(int i = 0; i < nb_threads; i++){
         if(pthread_join(threads[i], NULL)){
             fprintf(stderr, "Could not join thread %d\n", i);
             exit(1);
         }
      }
    }

    if (PAPI_read(EventSet, values) != PAPI_OK)
        printf("PAPI error: %d\n", 121);

    for(int i = 0; i<val_size; i++){
      final_values[i] = values[i];
    }

    for(int i=0; i < array_size; i++){

      array[i].value = 3;

      asm volatile ("" ::: "memory");

      if(flush==CLFLUSH){
        _mm_clflush(&array[i]);
      }
      else if(flush==CLFLUSHOPT){
        _mm_clflushopt(&array[i]);
      }
      else{
        _mm_clwb(&array[i]);
      }

      _mm_mfence();
      asm volatile ("" ::: "memory");

      array[i].value = 4;
    }

    if (PAPI_read(EventSet, values) != PAPI_OK)
          printf("PAPI error: %d\n", 127);

    for(int i = 0; i<val_size; i++){
        final_values[i] = values[i] - final_values[i];
    }

    if(k>1){
      /*At least we should get 1 RFO request for the first write operation*/

      if(final_values[0] >= array_size - (array_size*PROXIMITY)/100){
        exp_behavior++;
      }
      if( (final_values[0] <=  array_size + (array_size*PROXIMITY)/100) || (final_values[1] >=  array_size - (array_size*PROXIMITY)/100) ){
        no_invalidation++;
      }
    }
  }

  if(NB_ACCEPTED_RUNS>exp_behavior){
    * result = -1;
  }
  else{
    if(NB_ACCEPTED_RUNS <= no_invalidation){
      * result = 1;
    }
    else{
      * result = 0;
    }
  }

  if (PAPI_stop(EventSet, values) != PAPI_OK)
       printf("PAPI error: %d\n", 130);

  if (PAPI_destroy_eventset(&EventSet) == PAPI_EISRUN)
    printf("PAPI error: %d\n", 131);

  free(threads);
  return;
}



/*defining whther prefetching is anabled ot not*/
void check_prefetchers (volatile object * array, int array_size){

  int EventSet = PAPI_NULL;
  unsigned int native = 0x0;

  int val_size = 4;
  long_long values[val_size];
  long_long final_values[val_size];

  if (PAPI_create_eventset(&EventSet) != PAPI_OK)
    printf("PAPI error on event creation: %d\n", 1);

  /*Identifying counters to be used*/
  if (PAPI_event_name_to_code("MEM_LOAD_RETIRED.L1_HIT",&native) != PAPI_OK){
    printf("PAPI error on event defining: %d\n", 202);
    printf(RED "Undable to start monitoring Hardware counters!!! \nAborting ... \n" RESET);
    exit(-1);
  }

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
      printf("PAPI error on event adding: %d\n", 203);

  if (PAPI_event_name_to_code("MEM_LOAD_RETIRED.FB_HIT",&native) != PAPI_OK)
    printf("PAPI error on event defining: %d\n", 204);

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
      printf("PAPI error on event adding: %d\n", 205);

  if (PAPI_event_name_to_code("MEM_LOAD_RETIRED.L1_MISS",&native) != PAPI_OK)
    printf("PAPI error on event defining: %d\n", 206);

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
      printf("PAPI error on event adding: %d\n", 207);

  if (PAPI_event_name_to_code("MEM_LOAD_RETIRED.L2_HIT",&native) != PAPI_OK)
    printf("PAPI error on event defining: %d\n", 208);

  if (PAPI_add_event(EventSet, native) != PAPI_OK)
      printf("PAPI error on event adding: %d\n", 209);


  if (PAPI_start(EventSet) != PAPI_OK)
    printf("PAPI error on start counting: %d\n", 220);

  int tmp;
  int prefetching = 0;
  int exp_behavior=0;

  for(int k=0; k<NB_RUNS; k++){

    if (PAPI_read(EventSet, values) != PAPI_OK)
        printf("PAPI error on counter reading: %d\n", 221);

    for(int i = 0; i<val_size; i++){
        final_values[i] = values[i];
    }

    /*main loop for which we read counters*/
    for(int i=0; i < array_size ; i++){
      tmp = array[i].value;
      _mm_mfence();
    }

    if (PAPI_read(EventSet, values) != PAPI_OK)
          printf("PAPI error on counter reading: %d\n", 227);

    for(int i = 0; i<val_size; i++){
        final_values[i] = values[i] - final_values[i];
    }

    if(k>1){
      if( (final_values[0] + final_values[1] + final_values[2]) >= array_size - (array_size*PROXIMITY)/100){
        exp_behavior++;
      }
      if( (final_values[0] + final_values[1] + final_values[3]) >= array_size - (array_size*PROXIMITY)/100 ){
        prefetching++;
      }
    }
  }

  /*Evaluate results. We need at least 9 "expected" runs from 10*/
  if(NB_ACCEPTED_RUNS>exp_behavior){
    /*undefined behavior*/
    printf(RED "UNDEFINED BEHAVIOR WHILE EVALUATING PREFETCHING!!!\n" RESET);
  }
  else if(NB_ACCEPTED_RUNS <= prefetching){
    /*prefetching is enabled*/
    printf(RED "PREFETCHERS ARE ENABLED, THEREFORE RESULTS ARE NOT RELIABLE!!!\n" RESET);
  }

  if (PAPI_stop(EventSet, values) != PAPI_OK)
       printf("PAPI error on stopping counter reading: %d\n", 230);

  if (PAPI_destroy_eventset(&EventSet) == PAPI_EISRUN)
    printf("PAPI error on destroying eventset: %d\n", 231);

  return;
}
