#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define BLUE    "\033[34m"      /* Blue */
#define BLACK   "\033[30m"      /* Black */
#define YELLOW  "\033[33m"      /* Yellow */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */

#define MAX(_x, _y) ( ((_x) > (_y)) ? (_x) : (_y) )

#define NB_TESTS 6  /*we consider 6 different scenarios*/

#define PROXIMITY 10  /*we calculate with 10% approximation*/

#define NB_RUNS 12  /*10 runs will be considered from this 12 runs*/

#define NB_ACCEPTED_RUNS 9  /*from 10 runs that we consider we need 9 expected results*/

#define NB_THREADS 4  /*default number of threads for multithreading*/

#define MIN_SIZE_ARRAY 100000  /*minimum size of an array used for testing*/
