#include "clwb_all_impl.c"

int arr_size;

int main(int argc, char* argv[])
    {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);

      hwloc_topology_t topology;
      hwloc_obj_t numa1;
      hwloc_obj_t numa2;
      hwloc_obj_t obj1, obj2, obj_11, obj_12;
      int pu_depth, depth;

      hwloc_topology_init(&topology);
      hwloc_topology_load(topology);

      numaNodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NUMANODE);

      if (numaNodes>1){

        obj1 = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, 0);
        obj2 = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, 1);
        int cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);

        pu_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PU);
        depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NUMANODE);

        while(hwloc_get_nbobjs_by_depth(topology, depth)!=cores){
          depth++;
          obj1 = obj1->children[0];
          obj2 = obj2->children[0];
        }
        obj1 = obj1->parent;
        obj2 = obj2->parent;

        nb_pu = (cores/numaNodes);
        numa1_pu_indexes = (int *) malloc(nb_pu * sizeof(int));
        numa2_pu_indexes = (int *) malloc(nb_pu * sizeof(int));

        for(int i=0; i<nb_pu; i++){
          numa1_pu_indexes[i]=0;
          numa2_pu_indexes[i]=0;
        }

        for(int i=0; i<nb_pu; i++){

          obj_11 = obj1->children[i];
          obj_12 = obj2->children[i];

          for(int p=0; p<pu_depth-depth; p++){
             obj_11 = obj_11->children[0];
             obj_12 = obj_12->children[0];
          }
          numa1_pu_indexes[i] = obj_11->os_index;
          numa2_pu_indexes[i] = obj_12->os_index;
        }

        CPU_SET(numa1_pu_indexes[0], &cpuset);

        if (sched_setaffinity(getpid (), sizeof(cpu_set_t), &cpuset))
         {
             fprintf(stderr, "process pinning failed!\n");
             exit(1);
         }

        nb_threads = nb_pu;
        numa_versions = NUMA_ONE;
      }
      else{
        numa_versions = NO_NUMAS;
        nb_threads = NB_THREADS;
      }

      int L2_size = 0;
      int L3_size = 0;

      /*Define the size of L2 and L3 caches*/
      for (int i = 0; i < 32; i++) {
          uint32_t eax, ebx, ecx, edx;
          eax = 4;
          ecx = i;
          __asm__ (
              "cpuid"
              : "+a" (eax)
              , "=b" (ebx)
              , "+c" (ecx)
              , "=d" (edx)
          );
          int cache_type = eax & 0x1F;
          if (cache_type == 0)
              break;

          int cache_level = (eax >>= 5) & 0x7;
          unsigned int cache_sets = ecx + 1;
          unsigned int cache_ways_of_associativity = ((ebx >>= 22) & 0x3FF) + 1;

          /*1 - Data Cache and 3 - Unified Cache */
          if( (cache_type == 1 || cache_type == 3) & (cache_level ==2) ){
            L2_size = cache_sets*cache_ways_of_associativity;
          }
          else if((cache_type == 1 || cache_type == 3) & (cache_level ==3) ){
            L3_size = cache_sets*cache_ways_of_associativity;
          }
      }

      /*Guarantee that the array does not fit in L2 cache, with multiple times. minimum 100,000 elements*/
      arr_size = MAX(2*L3_size, 10*L2_size);
      arr_size = MAX(arr_size, MIN_SIZE_ARRAY);

      volatile object * array;
      if(posix_memalign((void **) &array, 64, arr_size * sizeof(object) )!=0)
      {
        printf("alignment error\n");
      }

      /*Initialization of an array*/
      for(int i=0; i < arr_size; i++){
        array[i].value = 1;
        _mm_clflush(&array[i]);
      }
      _mm_mfence();

      init_results();

      int retval;
      retval = PAPI_library_init(PAPI_VER_CURRENT);
      if (retval != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI library init error!\n");
        exit(1);
      }

      printf("[========== Running Cache Line Invalidation Tests ==========] \n" );

      check_prefetchers(array, arr_size);

      flush_inst flush = CLWB;

      if (argc >1){
         char *testvar = argv[1];
         if( strcmp(testvar, flush_param[0])== 0 ){
           flush = CLFLUSH;
         }
         else if(strcmp(testvar, flush_param[1])== 0){
           flush = CLFLUSHOPT;
         }
         else if(strcmp(testvar, flush_param[2])!= 0){
           printf("Operation is not valid! Using default flush instruction!\n");
         }
      }
      printf(YELLOW "[*** Evaluating %s operation ***]\n" RESET, flush_param[flush]);

      printf("[---------- Beginning of output ----------] \n" );

      if (numa_versions != NO_NUMAS) {
        printf(YELLOW "[*** Array allocated in the First NUMA node ***] \n" RESET);
        printf(YELLOW "[*** All accesses to the cache line issued from the First NUMA node ***] \n" RESET);
      }

      for(int i= NO_MULTITHREADING, k=0; i<= WRITE_MULTITHREADING; i++, k++ ){
        check_load(&results[k], i, array, arr_size, flush);
        k++;
        check_store(&results[k], i, array, arr_size, flush);
      }

      for(int i=0; i<NB_TESTS; i++){
        printf("%s: ", text[i]);
        switch (results[i]) {
            case -1: printf(BLUE "UNDEFINED\n" RESET); break;
            case 0: printf(RED "INVALIDATION\n" RESET); break;
            case 1: printf(GREEN "NO INVALIDATION\n" RESET); break;
            default: printf("UNEXPECTED RESULT\n"); break;
        }
      }

      /*************************************************************************************/

      if (numa_versions != NO_NUMAS) {

        for (int k=0; k<4; k++){

            switch (k) {
                case 0: printf(YELLOW "[*** Initial state obtained by accessing the cache line from two NUMA nodes, Operations applied from the First NUMA node ***]\n" RESET);
                        numa_versions = NUMA_MIX;
                        break;
                case 1: printf(YELLOW "[*** Initial state obtained by accessing the cache line from the Second NUMA node, Operations applied from the First NUMA node ***]\n" RESET);
                        numa_versions = NUMA_SECOND_MULTITHREADING;
                        break;
                case 2: printf(YELLOW "[*** Initial state obtained by accessing the cache line from the First NUMA node, Operations applied from the Second NUMA node ***]\n" RESET);
                        numa_versions = NUMA_SECOND_FLUSHING;
                        CPU_ZERO(&cpuset);
                        CPU_SET(numa2_pu_indexes[0], &cpuset);
                        if (sched_setaffinity(getpid (), sizeof(cpu_set_t), &cpuset)){
                           fprintf(stderr, "process pinning failed!\n");
                           exit(1);
                        }
                        break;
                case 3: printf(YELLOW "[*** Except array allocation, All accesses to the cache line issued from the Second NUMA node ***] \n" RESET);
                        numa_versions = NUMA_SECOND;
                        break;
            }

            init_results();

            for(int i= READ_MULTITHREADING, k=2; i<= WRITE_MULTITHREADING; i++, k++ ){
              check_load(&results[k], i, array, arr_size, flush);
              k++;
              check_store(&results[k], i, array, arr_size, flush);
            }

            for(int i=2; i<NB_TESTS; i++){
              printf("%s: ", text[i]);
              switch (results[i]) {
                  case -1: printf(BLUE "UNDEFINED\n" RESET); break;
                  case 0: printf(RED "INVALIDATION\n" RESET); break;
                  case 1: printf(GREEN "NO INVALIDATION\n" RESET); break;
                  default: printf("UNEXPECTED RESULT\n"); break;
              }
            }
          }
      }

      printf("[----------    End of output    ----------] \n" );

      free(array);
  		return 0;
}
