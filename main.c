#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <libgen.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>

#include <sys/types.h>
#include <pthread.h>

#include <math.h>


#define ST_THRD_WAIT (proc_args_t*)(0)
#define ST_THRD_TERMINATE (proc_args_t*)(-1)


typedef struct proc_args_t {
    int i, N;
    int member_number;

} proc_args_t;

proc_args_t **threads_args = NULL; /* array of pointers */


char *exec_name;

void print_help();

void print_error(char *msg);
void *process(void *args);

char is_valid(long val);


int main(int argc, char *argv[])
{
    exec_name = basename(argv[0]);

    if (argc < 2) {
        print_error("Not enough arguments");
        print_help();
        exit(EXIT_FAILURE);
    }

    /* process params */
    errno = 0;

    int array_size_N = strtol(argv[1], NULL, 10);  /* N */
    int mems_count_n = strtol(argv[2], NULL, 10);  /* n */

    if ( (errno != 0) && !is_valid(array_size_N) && !is_valid(mems_count_n) ) {

       print_error("Bad arguments!");
       print_help();
       exit(EXIT_FAILURE);
    }

    /* threads input list */
    threads_args = (proc_args_t**)malloc(sizeof(proc_args_t*) * mems_count_n);

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    int i = 0;    

    /* empty threads list */
    for (i = 0; i < mems_count_n; ++i) {
        threads_args[i] = ST_THRD_TERMINATE;
    }

    int curr_thrd = 0;

    int j = 0;
    for (j = 0; j < array_size_N; ++j) {
        for (i = 0; i < mems_count_n; ++i) {
            proc_args_t* targs = malloc(sizeof(proc_args_t) );

            targs->i = i;
            targs->N = array_size_N;
            targs->member_number = i;

            /* wait for any thread to be ready */
            while (threads_args[curr_thrd] != ST_THRD_WAIT) {
                curr_thrd = (curr_thrd + 1) % mems_count_n ;

//                printf("%d\n", curr_thrd);
            }

            threads_args[curr_thrd] = targs;
        }
    }

    /* wait for all threads */
    do {
        j = mems_count_n;
        for (i = 0; i < mems_count_n; ++i) {
            if (threads_args[i] == ST_THRD_WAIT)
                --j;
        }
    } while (j != 0);

    /* terminate all threads */
    for (i = 0; i < mems_count_n; ++i) {
        threads_args[i] = ST_THRD_TERMINATE;
    }

    /* TODO: pick all results together */

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


void print_error(char *msg) {
    printf("%s: %s\n", exec_name, msg);

}   /* print_error */


void print_result(pthread_t id, double result) {

    /* ToDo: print result to terminal and file */

    printf("%lu %f\n", id, result);

}   /* print_result */


double get_sin_taylor_member(double x, int member_number) {
    double result = 1;
    for (int i = 1; i <= member_number * 2 + 1; i++) {
        result *= x / i;

    }
    return (member_number % 2) ? -result : result;
}

/* TODO: make with signals */
/*
 * sigmask
 *
 * tkill
 *
 * sigwait
 *
 * */


void *process(void *args) {
    pid_t tid = syscall(SYS_gettid);
    proc_args_t **args_p = (proc_args_t**)args;

    *args_p = ST_THRD_WAIT;

    double result = 0;

    do {
        while ( (*args_p) == ST_THRD_WAIT) ;    /* wait for job */

        double x = (2 * M_PI * (*args_p)->i)/(*args_p)->N;
        if (x != 0){
            x = M_PI - x;
        }

        result = get_sin_taylor_member(x, (*args_p)->member_number);

        print_result(tid, result);

        (*args_p) = ST_THRD_WAIT;

    } while ( (*args_p) != ST_THRD_TERMINATE);

    args_p = NULL;

    free(args);
    return NULL;

}   /* get_sin_taylor_member */
