#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <libgen.h>

#include <sys/types.h>
#include <pthread.h>

#include <math.h>


typedef struct proc_args_t {
    int i, N;
    int member_number;

} proc_args_t;


#define ST_THRD_WAIT (proc_args_t*)(0)
#define ST_THRD_TERMINATE (proc_args_t*)(-1)

proc_args_t **threads_args = NULL; /* array of pointers */


char *exec_name;

void print_error(char *msg);
void *process(void *args);

char is_valid(const int val) {
    return !(val == LONG_MAX || val == LONG_MIN || val == 0);
}

int main(int argc, char *argv[])
{
    exec_name = basename(argv[0]);
    if (argc < 2) {
        print_error("Not enough arguments");
        exit(1);
    }

    errno = 0;
    int members_count = strtol(argv[1], NULL, 10);
    int threads_count = strtol(argv[2], NULL, 10);

    if ( (errno != 0) && !is_valid(members_count) && !is_valid(threads_count) ) {

       print_error("Bad arguments!");
       exit(EXIT_FAILURE);
    }

    threads_args = (proc_args_t**)malloc(sizeof(proc_args_t*) * threads_count);

    pthread_t tid = 0;

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    int i = 0;
    for (i = 0; i < threads_count; ++i) {   /* creating threads */
        threads_args[i] = ST_THRD_TERMINATE;

        if (pthread_create(&tid, &thread_attr, &process, (threads_args+i) ) == -1) {
            printf("Error creating thread!\n");
        }
    }

    int curr_thrd = 0;
    int j = 0;
    for (j = 0; j < members_count; ++j) {
        for (i = 0; i < threads_count; ++i) {

        }
    }

    /* ToDo: add results count here */

    return 0;

}   /* main */


void print_error(char *msg) {
    printf("%s: %s\nr", exec_name, msg);

}   /* print_error */


void print_result(pthread_t id, double result) {
    /* ToDo: print result to terminal and file */

}   /* print_result */


double get_sin_taylor_member(const int i, const int N, const int member_number) {
    unsigned long long num = 1, den = 1;

    for (int j = 0; j < (2*member_number + 1); ++j) {
        num *= 2*M_PI*i;
        den *= N;
    }

    double result = num / den;
    return (member_number % 2) ? -result : result;

}   /* get_sin_taylor_member */


void *process(void *args) {
    proc_args_t **args_p = (proc_args_t**)args;

    *args_p = ST_THRD_WAIT;

    do {
        pthread_t self_id = pthread_self();
        double result = get_sin_taylor_member( (*args_p)->i, (*args_p)->N, (*args_p)->member_number);

        print_result(self_id, result);

        while ( (*args_p) == ST_THRD_WAIT) ;    /* wait for job */

    } while ( (*args_p) != ST_THRD_TERMINATE);

}   /* get_sin_taylor_member */
