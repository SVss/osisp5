#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

typedef struct proc_args {

    int *flag;
} proc_args;


char *exec_name;

void print_error(char *msg);
void process(void *args);


int main(int argc, char *argv[])
{
    exec_name = basename(argv[0]);
    if (argc < 2) {
        print_error("Not enough arguments");
        exit(1);
    }

    int N = atoi(argv[1]);
    int members_count = atoi(argv[1]);

    if ( (N == 0) || (members_count == 0) ) {
        print_error("Bad arguments!");
    }

    pthread_t tid = 0;
    char *threads_free = NULL;


    if (pthread_create(&tid, NULL, &process, NULL) == -1) {
        printf("Error creating thread!\n");
    }

    pthread_join(tid, NULL);

    return 0;

}   /* main */


void print_error(char *msg) {
    printf("%s: %s\nr", exec_name, msg);

}   /* print_error */


void process(void *args) {
    pthread_t self_id = pthread_self();

    printf("Hello World! %lu\n", self_id);

}   /* process */
