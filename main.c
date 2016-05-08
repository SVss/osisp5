#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <limits.h>

#include <libgen.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>

#include <sys/types.h>
#include <pthread.h>

#include <math.h>


#define ST_THRD_READY (proc_args_t*)(-1)

const char *RESULTS_ERROR = "Error counting results";

typedef struct proc_args_t {
    double x;
    int i;
    int member_number;

} proc_args_t;


typedef struct thread_info {
    pthread_t tid;
    proc_args_t *args;

} thread_info;


char *exec_name;
FILE* temp_f = NULL;
char *result_filename = "/tmp/taylor.tmp";


void print_error(const char *msg);
void print_help();

void sig_usr1_handler(int signo) {
    /* empty signal handler */
}


void kill_thread(pthread_t thread_id) {
    if (thread_id != 0) {
        pthread_kill(thread_id, SIGUSR1);
    }
}   /* kill_thread */


void *process(void *args);
char is_valid(long val);


int main(int argc, char *argv[])
{
    exec_name = basename(argv[0]);

/* process arguments */
    if (argc < 2) {
        print_error("Not enough arguments");
        print_help();
        exit(EXIT_FAILURE);
    }

    errno = 0;

    int array_size_N = strtol(argv[1], NULL, 10);  /* N */
    int mems_count_n = strtol(argv[2], NULL, 10);  /* n */

    if ( (errno != 0) && !is_valid(array_size_N) && !is_valid(mems_count_n) ) {

       print_error("Bad arguments!");
       print_help();
       exit(EXIT_FAILURE);
    }


    if ( (temp_f = fopen("/tmp/log.txt", "w+") ) == NULL) {
        print_error("Can't create temporary file");
        exit((EXIT_FAILURE));
    }

/* clear list of threads info */
    thread_info *threads_list = (thread_info*)calloc(mems_count_n, sizeof(thread_info));

    int m = 0;
    for (m = 0; m < mems_count_n; ++m) {
        threads_list[m].args = ST_THRD_READY;
    }

/* processing */
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    signal(SIGUSR1, sig_usr1_handler);

    int curr_thrd = 0;
    double x = 0;

    int i = 0;
    for (i = 0; i < array_size_N; ++i) {
        x = 2 * M_PI * i / array_size_N;
        if (x != 0) {
            x = M_PI - x;
        }

        for (m = 0; m < mems_count_n; ++m) {
            proc_args_t* targs = malloc(sizeof(proc_args_t) );

            targs->x = x;
            targs->i = i;
            targs->member_number = m;

    /* wait for any thread to be ready */
            do {
                curr_thrd = (curr_thrd + 1) % mems_count_n;
            } while (threads_list[curr_thrd].args != ST_THRD_READY);

            kill_thread(threads_list[curr_thrd].tid);

    /* pass new aguments */
            threads_list[curr_thrd].args = targs;

            if (pthread_create( &(threads_list[m].tid), &thread_attr,
                                &process, (void*)(threads_list + curr_thrd) ) != 0) {

                print_error("Error creating thread!");
                exit(EXIT_FAILURE);
            }

        }   /* m */

    }   /* i */

#ifdef DEBUG
    printf("\nTask complited\nWaiting for threads...\n\n");
    fflush(stdout);
#endif

/* kill all threads */
    for (m = 0; m < mems_count_n; ++m) {
        while (threads_list[m].args != ST_THRD_READY)
            ; /* wait */

        kill_thread(threads_list[m].tid);
    }

    free(threads_list);

/* pick all results together */
    double *results = (double*)alloca(sizeof(double)*array_size_N);
    memset(results, 0, sizeof(double)*array_size_N);

    rewind(temp_f); /* !!! important */
    for (i = 0; i < array_size_N*mems_count_n; ++i) {
        if (fscanf(temp_f, "%*d %d %lf", &m, &x) == 2) {
            if ( (m >=0) && (m < array_size_N) ) {
                results[m] += x;
#ifdef DEBUG
                printf("%lf\n", x);
#endif
            } else {

            }
        } else {
            print_error(RESULTS_ERROR);
            exit((EXIT_FAILURE));
        }
    }

/* print results */
    FILE *result_f = fopen(result_filename, "wt");
    if (result_f == NULL) {
        print_error("Can't create result file");
        exit((EXIT_FAILURE));
    }

    for (m = 0; m < array_size_N; ++m) {
        fprintf(result_f, "%lf\n", results[m]);
    }

    fclose(result_f);

    return 0;
}   /* main */


void print_help() {
    printf("\n  osisp5 N n\n"
           "  Count sine function for array of N elements with Taylor series.\n\n"
           "  Arguments (natural numbers):\n"
           "  \tN - number of elements in array\n"
           "  \t\teach element y[i]=sin(2*PI*i/N) ( i=0,1,2…N-1 )\n\n"
           "  \tn - Taylor series members quantity\n"
           "  \t\t(the more n is - the better approximation is)\n\n");

}   /* print_help */


char is_valid(long val) {
    return !(val == LONG_MAX || val == LONG_MIN || val == 0);
}   /* is_valid */


void print_error(const char *msg) {
    printf("%s: %s\n", exec_name, msg);

}   /* print_error */


void print_result(double result, int n) {
    pid_t id = syscall(SYS_gettid);

    printf("%d %d %lf\n", id, n, result);
    fflush(stdout);

    fprintf(temp_f, "%d %d %lf\n", id, n, result);
    fflush(temp_f);

}   /* print_result */


double get_sin_taylor_member(double x, int member_number) {
    double result = 1;
    for (int i = 1; i <= member_number * 2 + 1; i++) {
        result *= x / i;
    }
    return (member_number % 2) ? -result : result;
}



void *process(void *args) {

    thread_info* info = ( (thread_info*)args);
    double result = 0;
    int sig_num = 0;

    result = get_sin_taylor_member(info->args->x, info->args->member_number);

    print_result(result, info->args->i);

    sigset_t sig_set;
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGUSR1);

    free(info->args);
    info->args = ST_THRD_READY;


/* waiting to exit */
    sigwait(&sig_set, &sig_num);

#ifdef DEBUG
    printf("%d\tis terminated\n", tid);
    fflush(stdout);
#endif

    return NULL;

}   /* process */
