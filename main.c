#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <libgen.h>

#include <unistd.h>
#include <sys/syscall.h>

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

void print_error(char *msg);
void *process(void *args);

char is_valid(long val);


int main(int argc, char *argv[])
{
    exec_name = basename(argv[0]);

    if (argc < 2) {
        print_error("Not enough arguments");
        exit(EXIT_FAILURE);
    }

    /* process params */
    errno = 0;

    int elm_count_N = strtol(argv[1], NULL, 10);  /* N */
    int mems_count_n = strtol(argv[2], NULL, 10);  /* n */

    if ( (errno != 0) && !is_valid(elm_count_N) && !is_valid(mems_count_n) ) {

       print_error("Bad arguments!");
       exit(EXIT_FAILURE);
    }

    threads_args = (proc_args_t**)malloc(sizeof(proc_args_t*) * mems_count_n);

    pthread_t tid = 0;

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    int i = 0;
    for (i = 0; i < mems_count_n; ++i) {   /* creating threads */
        threads_args[i] = ST_THRD_TERMINATE;

        if (pthread_create(&tid, &thread_attr, &process, (threads_args+i) ) == -1) {
            print_error("Error creating thread!\n");
            exit(EXIT_FAILURE);
        }
    }

    int curr_thrd = 0;

    int j = 0;
    for (j = 0; j < elm_count_N; ++j) {
        for (i = 0; i < mems_count_n; ++i) {
            proc_args_t* targs = malloc(sizeof(proc_args_t) );

            targs->i = i;
            targs->N = elm_count_N;
            targs->member_number = i;

            /* wait for any thread to be ready */
            while (threads_args[curr_thrd] != ST_THRD_WAIT) {
                ++curr_thrd;

                if (curr_thrd == mems_count_n)
                    curr_thrd = 0;

                printf("%d\n", curr_thrd);
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


char is_valid(long val) {
    return !(val == LONG_MAX || val == LONG_MIN || val == 0);
}   /* is_valid */


void print_error(char *msg) {
    printf("%s: %s\nr", exec_name, msg);

}   /* print_error */


double get_sin_taylor_member(const int i, const int N, const int member_number) {
    unsigned long long num = 1, den = 1;

    for (int j = 1; j <= (2*member_number + 1); ++j) {
        num *= 2*M_PI*i/N;
        den *= j;
    }

    double result = num / den;
    return (member_number % 2) ? -result : result;

}   /* get_sin_taylor_member */


void print_result(pthread_t id, double result) {
    /* ToDo: print result to terminal and file */
    printf("%lu\t%f\n", id, result);

}   /* print_result */


void *process(void *args) {
    pid_t self_id = syscall(SYS_gettid);
    proc_args_t **args_p = (proc_args_t**)args;

    *args_p = ST_THRD_WAIT;

    do {
        while ( (*args_p) == ST_THRD_WAIT) ;    /* wait for job */

        double result = get_sin_taylor_member( (*args_p)->i, (*args_p)->N, (*args_p)->member_number);

        print_result(self_id, result);

    } while ( (*args_p) != ST_THRD_TERMINATE);

    free(args);
    return NULL;

}   /* get_sin_taylor_member */
