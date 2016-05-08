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


#define ST_THRD_READY (proc_args_t*)(-1)

typedef struct proc_args_t {
    int i, N;
    int member_number;

} proc_args_t;


typedef struct thread_info {
    pthread_t tid;
    proc_args_t *args;

} thread_info;


char *exec_name;

void print_error(char *msg);
void print_help();

void sig_usr1_handler(int signo);
void *process(void *args);

char is_valid(long val);

/*
#define RELIABLE_OUTPUT
*/

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

#ifdef RELIABLE_OUTPUT
    signal(SIGUSR1, sig_usr1_handler);
#endif

    int curr_thrd = 0;
    pthread_t thread_id = 0;

    int i = 0;
    for (i = 0; i < array_size_N; ++i) {
        for (m = 0; m < mems_count_n; ++m) {
            proc_args_t* targs = malloc(sizeof(proc_args_t) );

            targs->i = i;
            targs->N = array_size_N;
            targs->member_number = m;

/* wait for any thread to be ready */
            while (threads_list[curr_thrd].args != ST_THRD_READY) {
                curr_thrd = (curr_thrd + 1) % mems_count_n ;
            }

            if ( (thread_id = threads_list[curr_thrd].tid) != 0) {
                pthread_kill(thread_id, SIGUSR1);
            }

            threads_list[curr_thrd].args = targs;

            if (pthread_create( &(threads_list[m].tid), &thread_attr,
                                &process, (void*)(threads_list + curr_thrd) ) != 0) {

                print_error("Error creating thread!");
                exit(EXIT_FAILURE);
            }
        }
    }

/* terminate all threads */
    for (m = 0; m < mems_count_n; ++i) {
        while (threads_list[m].args != ST_THRD_READY) ; /* wait */
        pthread_kill(threads_list[m].tid, SIGUSR1); /* terminate */
    }

    /* TODO: pick all results together */

    getchar();

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
    for (int i = 1; i <= 2*member_number + 1; ++i) {
        result *= x / i;
    }

    return (member_number & 1)*(-1)*result;
}


#ifdef RELIABLE_OUTPUT
void sig_usr1_handler(int signo) {
    print_result(tid, result);
}
#endif



void *process(void *args) {
    pid_t tid = syscall(SYS_gettid);

    thread_info* info = ( (thread_info*)args);

    double result = 0;
    double x = (2 * M_PI * (info->args->i) ) / (info->args->N);
    /*
    if (x != 0){
        x = M_PI - x;
    }
*/
    result = get_sin_taylor_member(x, info->args->member_number);

#ifndef RELIABLE_OUTPUT
    print_result(tid, result);
#endif

    int sig_num = 0;
    sigset_t sig_set;
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGUSR1);

    free(info->args);
    info->args = ST_THRD_READY;

/* waiting to exit */
    printf("%d\tis waiting\n", tid);
    sigwait(&sig_set, &sig_num);

    printf("%d\tis terminated\n", tid);

    return NULL;

}   /* get_sin_taylor_member */
