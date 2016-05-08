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
//#define ST_THRD_DEAD (proc_args_t*)(-2)

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
FILE* temp = NULL;
char *result_tmp_file_name = "/tmp/taylor.tmp";


void print_error(char *msg);
void print_help();

void sig_usr1_handler(int signo) {
#ifdef DEBUG
    printf("%ld received SigUsr1\n", syscall(SYS_gettid) );
    fflush(stdout);
#endif
}

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

/* creatin temp file */
    temp = tmpfile();
    if (temp == NULL) {
        print_error("Can't create temporary file");
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
    pthread_t thread_id = 0;
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

    printf("\nTask complited\nWaiting for threads...\n\n");
    fflush(stdout);

/* wait for all threads */
    for (m = 0; m < mems_count_n; ++m) {
        while (threads_list[m].args != ST_THRD_READY)
            ; /* wait */

        if (threads_list[m].tid != 0) {
            pthread_kill(threads_list[m].tid, SIGUSR1); /* terminate */
        }
    }

    free(threads_list);

/* TODO: pick all results together */
    printf("All done!\nPress Enter...\n");
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


void print_result(double result, int n) {
    pid_t id = syscall(SYS_gettid);

    printf("%d %d %lf\n", id, n, result);
    fflush(stdout);

    fprintf(temp, "%d %d %lf\n", id, n, result);
    fflush(temp);

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
