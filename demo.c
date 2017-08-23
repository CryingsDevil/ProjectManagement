/*
 * This is a skeleton program for COMSM2001 (Server Software) coursework 1
 * "the project marking problem". Your task is to synchronise the threads
 * correctly by adding code in the places indicated by comments in the
 * student, marker and run functions.
 * You may create your own global variables and further functions.
 * The code in this skeleton program can be used without citation in the files
 * that you submit for your coursework.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include <errno.h>
#include <string.h>

/*
 * Parameters of the program. The constraints are D < T and
 * S*K <= M*N.
 */
struct demo_parameters {
    int S;   /* Number of students */
    int M;   /* Number of markers */
    int K;   /* Number of markers per demo */
    int N;   /* Number of demos per marker */
    int T;   /* Length of session (minutes) */
    int D;   /* Length of demo (minutes) */
};

/* Global object holding the demo parameters. */
struct demo_parameters parameters;

/* The demo start time, set in the main function. Do not modify this. */
struct timeval starttime;

/* 
 * You may wish to place some global variables here. 
 * Remember, globals are shared between threads.
 * You can also create functions of your own.
 */
typedef struct{
    int ID;
    pthread_mutex_t marker;
    pthread_mutex_t student;
    pthread_cond_t demoFinish;
    pthread_cond_t demoBegin;
}students;
static students *studentSS[100];

typedef struct{
    int ID;
    int noEnoughTime;
    int markerGrabbed;
    int finished;
    pthread_cond_t grabbing;
    pthread_cond_t grabbed;
    pthread_cond_t markerIn;
    pthread_mutex_t student;
    pthread_mutex_t marker;
}studentGrabing;

static studentGrabing *studentGrab;


/*
 * timenow(): returns current simulated time in "minutes" (cs).
 * Assumes that starttime has been set already.
 * This function is safe to call in the student and marker threads as
 * starttime is set in the run() function.
 */
int timenow() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - starttime.tv_sec) * 100 + (now.tv_usec - starttime.tv_usec) / 10000;
}

/* delay(t): delays for t "minutes" (cs) */
void delay(int t) {
    struct timespec rqtp, rmtp;
    t *= 10;
    rqtp.tv_sec = t / 1000;
    rqtp.tv_nsec = 1000000 * (t % 1000);
    nanosleep(&rqtp, &rmtp);
}

/* panic(): simulates a student's panicking activity */
void panic() {
    delay(random() % (parameters.T - parameters.D));
}

/* demo(): simulates a demo activity */
void demo() {
    delay(parameters.D);
}

/*
 * A marker thread. You need to modify this function.
 * The parameter arg is the number of the current marker and the function
 * doesn't need to return any values.
 * Do not modify the printed output as it will be used as part of the testing.
 */
void *marker(void *arg) {
    int markerID = *(int *)arg;
    
    /*
     * The following variable is used in the printf statements when a marker is
     * grabbed by a student. It shall be set by this function whenever the
     * marker is grabbed - and before the printf statements referencing it are
     * executed.
     */
    int studentID;
    
    /* 1. Enter the lab. */
    printf("%d marker %d: enters lab\n", timenow(), markerID);

    /* A marker marks up to N projects. */
    /* 2. Repeat (N times). 
     *    (a) Wait to be grabbed by a student.
     *    (b) Wait for the student's demo to begin
     *        (you may not need to do anything here).
     *    (c) Wait for the demo to finish.
     *        Do not just wait a given time -
     *        let the student signal when the demo is over.
     *    (d) Exit the lab.
     */
    /* 
     * 3. If the end of the session approaches (i.e. there is no time
     *    to start another demo) then the marker waits for the current
     *    demo to finish (if they are currently attending one) and then
     *    exits the lab.
     */
    int job = 0;
    while(job < parameters.N && parameters.T - timenow() > parameters.D){
        pthread_mutex_lock(&studentGrab->marker);
        while(studentGrab->ID == -1)pthread_cond_wait(&studentGrab->grabbing, &studentGrab->marker);

        if(studentGrab->markerGrabbed < parameters.K){
            studentGrab->markerGrabbed++;
            studentID = studentGrab->ID;
            pthread_cond_broadcast(&studentGrab->markerIn);
            pthread_mutex_unlock(&studentGrab->marker);
            
            
            pthread_mutex_lock(&studentSS[studentID]->marker);
            /* The following line shall be printed when a marker is grabbed by a student. */
            printf("%d marker %d: grabbed by student %d (job %d)\n", timenow(), markerID, studentID, job + 1);
            pthread_cond_wait(&studentSS[studentID]->demoBegin, &studentSS[studentID]->marker);
            pthread_mutex_unlock(&studentSS[studentID]->marker);
            
            if(parameters.T - timenow() > parameters.D){
                pthread_mutex_lock(&studentSS[studentID]->marker);
                pthread_cond_wait(&studentSS[studentID]->demoFinish, &studentSS[studentID]->marker);
                /* The following line shall be printed when a marker has finished attending a demo. */
                printf("%d marker %d: finished with student %d (job %d)\n", timenow(), markerID, studentID, job + 1);
                job++;
                
                pthread_mutex_unlock(&studentSS[studentID]->marker);

            }
            
            
        }else{
            pthread_mutex_unlock(&studentGrab->marker);
        }
    }
    

    

    /* 
     * When the marker exits the lab, exactly one of the following two lines shall be
     * printed, depending on whether the marker has finished all their jobs or there
     * is no time to complete another demo.
     */
    if(job == parameters.N){
        printf("%d marker %d: exits lab (finished %d jobs)\n", timenow(), markerID, parameters.N);
    }else{
        printf("%d marker %d: exits lab (timeout)\n", timenow(), markerID);
    }
    
    return NULL;
}


/*
 * A student thread. You must modify this function.
 */
void *student(void *arg) {
    /* The ID of the current student. */
    int studentID = *(int *)arg;
    studentSS[studentID] = malloc(sizeof(students));
    if(studentSS[studentID] == NULL){
        printf("Student thread %d create struct failed\n", studentID);
        abort();
    }else{
        studentSS[studentID]->ID = studentID;

        if(pthread_mutex_init(&studentSS[studentID]->marker, NULL) != 0 
            || pthread_mutex_init(&studentSS[studentID]->student, NULL) != 0 
            || pthread_cond_init(&studentSS[studentID]->demoFinish, NULL) != 0 
            || pthread_cond_init(&studentSS[studentID]->demoBegin, NULL) != 0){
            printf("Student thread %d initialize mutex or condition failed\n", studentID);
            abort();
        }
    }
    /* 1. Panic! */
    printf("%d student %d: starts panicking\n", timenow(), studentID);
    panic();

    /* 2. Enter the lab. */
    printf("%d student %d: enters lab\n", timenow(), studentID);

    /* 3. Grab K markers. */
    if(parameters.T - timenow() > parameters.D){
        pthread_mutex_lock(&studentGrab->student);
        if(studentGrab->ID == -2){
            printf("%d student %d: exits lab (timeout)\n", timenow(), studentID);
            pthread_mutex_unlock(&studentGrab->student);
            return NULL;
        }
        while(studentGrab->ID != -1 && studentGrab->finished == 0)pthread_cond_wait(&studentGrab->grabbed, &studentGrab->student);
        studentGrab->ID = studentID;
        studentGrab->finished = 0;
        pthread_cond_broadcast(&studentGrab->grabbing);
        while(studentGrab->markerGrabbed < parameters.K && studentGrab->noEnoughTime == 0)pthread_cond_wait(&studentGrab->markerIn, &studentGrab->student); 
        
        if(studentGrab->noEnoughTime == 1){
            printf("%d student %d: exits lab (timeout)\n", timenow(), studentID);
            pthread_cond_broadcast(&studentGrab->grabbed);
            pthread_cond_broadcast(&studentSS[studentID]->demoBegin);
            pthread_mutex_unlock(&studentGrab->student);
            return NULL;
        }
        studentGrab->markerGrabbed = 0;
        studentGrab->ID = -1;
        studentGrab->finished = 1;
        pthread_cond_broadcast(&studentGrab->grabbed);
        pthread_mutex_unlock(&studentGrab->student);
        /* 4. Demo! */
        /*
         * If the student succeeds in grabbing K markers and there is enough time left
         * for a demo, the following three lines shall be executed in order.
         * If the student has not started their demo and there is not sufficient time
         * left to do a full demo, the following three lines shall not be executed
         * and the student proceeds to step 5.
         */
        delay(3);
        pthread_mutex_lock(&studentSS[studentID]->student);
        printf("%d student %d: starts demo\n", timenow(), studentID);
        pthread_cond_broadcast(&studentSS[studentID]->demoBegin);
        demo();
        printf("%d student %d: ends demo\n", timenow(), studentID);
        pthread_cond_broadcast(&studentSS[studentID]->demoFinish);
        printf("%d student %d: exits lab (finished)\n", timenow(), studentID);
        pthread_mutex_unlock(&studentSS[studentID]->student);
    }else{
        printf("%d student %d: exits lab (timeout)\n", timenow(), studentID);
    }
    
    


    /* 5. Exit the lab. */


    /* 
     * Exactly one of the following two lines shall be printed, depending on
     * whether the student got to give their demo or not.
     */
    
    delay(10);
    pthread_mutex_lock(&studentSS[studentID]->marker);
    pthread_mutex_lock(&studentSS[studentID]->student);
    pthread_mutex_unlock(&studentSS[studentID]->student);
    pthread_mutex_unlock(&studentSS[studentID]->marker);
    int err = pthread_mutex_destroy(&studentSS[studentID]->student);
    if(err != 0)printf("Student %d Mutex student destroyed: %s\n", studentID, strerror(err));
    err = pthread_mutex_destroy(&studentSS[studentID]->marker);
    if(err != 0)printf("Student %d Mutex marker destroyed: %s\n", studentID, strerror(err));
    err = pthread_cond_destroy(&studentSS[studentID]->demoFinish);
    if(err != 0)printf("Student %d condition demoFinish destroyed: %s\n", studentID, strerror(err));
    err = pthread_cond_destroy(&studentSS[studentID]->demoBegin);
    if(err != 0)printf("Student %d condition demoBegin destroyed: %s\n", studentID, strerror(err));

    return NULL;
}

/* The function that runs the session.
 * You MAY want to modify this function.
 */
void run() {
    int i;
    int markerID[100], studentID[100];
    pthread_t markerT[100], studentT[100];

    printf("S=%d M=%d K=%d N=%d T=%d D=%d\n",
        parameters.S,
        parameters.M,
        parameters.K,
        parameters.N,
        parameters.T,
        parameters.D);
    gettimeofday(&starttime, NULL);  /* Save start of simulated time */

    studentGrab = malloc(sizeof(studentGrabing));
    if(studentGrab == NULL){
        puts("No enough memory");
        abort();
    }else{
        studentGrab->ID = -1;
        studentGrab->noEnoughTime = 0;
        studentGrab->markerGrabbed = 0;
        studentGrab->finished = 0;
        int success = pthread_mutex_init(&studentGrab->student , NULL);
        int success1 = pthread_mutex_init(&studentGrab->marker , NULL);
        int success2 = pthread_cond_init(&studentGrab->grabbing , NULL);
        int success3 = pthread_cond_init(&studentGrab->grabbed , NULL);
        int success4 = pthread_cond_init(&studentGrab->markerIn , NULL);
        if(success != 0 || success1 != 0 || success2 != 0 || success3 != 0 || success4 != 0){
            puts("Initialization error");
            abort();
        }
    }
    
    /* Create S student threads */
    for (i = 0; i<parameters.S; i++) {
        studentID[i] = i;
        pthread_create(&studentT[i], NULL, student, &studentID[i]);
    }
    /* Create M marker threads */
    for (i = 0; i<parameters.M; i++) {
        markerID[i] = i;
        pthread_create(&markerT[i], NULL, marker, &markerID[i]);
    }

    /* With the threads now started, the session is in full swing ... */
    delay(parameters.T - parameters.D);

    /* 
     * When we reach here, this is the latest time a new demo could start.
     * You might want to do something here or soon after.
     */
    while(parameters.T - timenow() > parameters.D);
    delay(parameters.D / 4);    
    pthread_mutex_lock(&studentGrab->marker);
    studentGrab->markerGrabbed = parameters.K + 1;
    pthread_cond_broadcast(&studentGrab->markerIn);
    pthread_mutex_unlock(&studentGrab->marker);

    delay(3);
    pthread_mutex_lock(&studentGrab->student);
    studentGrab->noEnoughTime = 1;
    studentGrab->ID = -2;
    
    delay(3);
    
    pthread_cond_broadcast(&studentGrab->grabbing);
    delay(3);
    pthread_cond_broadcast(&studentGrab->grabbing);
    pthread_cond_broadcast(&studentGrab->grabbed);
    pthread_mutex_unlock(&studentGrab->student);

    /* Wait for student threads to finish */
    for (i = 0; i<parameters.S; i++) {
        pthread_join(studentT[i], NULL);
    }
    /* Wait for marker threads to finish */
    for (i = 0; i<parameters.M; i++) {
        pthread_join(markerT[i], NULL);
    }
    delay(10);
    pthread_mutex_lock(&studentGrab->marker);
    pthread_mutex_lock(&studentGrab->student);
    pthread_mutex_unlock(&studentGrab->student);
    pthread_mutex_unlock(&studentGrab->marker);
    int err = pthread_mutex_destroy(&studentGrab->student);
    if(err != 0)printf("Run Mutex student destroyed: %s\n", strerror(err));
    err = pthread_mutex_destroy(&studentGrab->marker);
    if(err != 0)printf("Run Mutex marker destroyed: %s\n", strerror(err));
    err = pthread_cond_destroy(&studentGrab->grabbed);
    if(err != 0)printf("Run condition grabbed destroyed: %s\n", strerror(err));
    err = pthread_cond_destroy(&studentGrab->grabbing);
    if(err != 0)printf("Run condition grabbing destroyed: %s\n", strerror(err));
    err = pthread_cond_destroy(&studentGrab->markerIn);
    if(err != 0)printf("Run condition markerIn destroyed: %s\n", strerror(err));

}

/*
 * main() checks that the parameters are ok. If they are, the interesting bit
 * is in run() so please don't modify main().
 */
int main(int argc, char *argv[]) {
    if (argc < 6) {
        puts("Usage: demo S M K N T D\n");
        exit(1);
    }
    parameters.S = atoi(argv[1]);
    parameters.M = atoi(argv[2]);
    parameters.K = atoi(argv[3]);
    parameters.N = atoi(argv[4]);
    parameters.T = atoi(argv[5]);
    parameters.D = atoi(argv[6]);
    if (parameters.M > 100 || parameters.S > 100) {
        puts("Maximum 100 markers and 100 students allowed.\n");
        exit(1);
    }
    if (parameters.D >= parameters.T) {
        puts("Constraint D < T violated.\n");
        exit(1);
    }
    if (parameters.S*parameters.K > parameters.M*parameters.N) {
        puts("Constraint S*K <= M*N violated.\n");
        exit(1);
    }

    // We're good to go.

    run();

    return 0;
}