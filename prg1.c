/*
 * Subject RTOS 48450
 * Project: Assignment 2 - Prg1
 * Name: Tess Dunlop
 * Student Number: 11655646
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */

/*Name of the file that we are reading from*/
#define DATA_FILENAME "data.txt"
/*Name of the file that we are writing to*/
#define SOURCE_FILENAME "src.txt"
/*Defined message length*/
#define MESSLENGTH 1024

/*Struct holding the parameters that I will pass through to my threads*/
struct ThreadParams
{
	/*Flag indicating the end of the file*/
	int fileEnd;
	/*Pipe*/
	int fd[2];
	/*Mutex*/
	pthread_mutex_t mutex;
	/*A place to pass the string from thread 2 (when it comes out of the pipe) to thread 3*/
	char *outPipe;
	/*Thread semaphores*/
	sem_t one, two, three;
};


/*Declare functions and threads*/
void *runnerOne(void *param);
void *runnerTwo(void *param);
void *runnerThree(void *param);
int initializeData(void *param);
int addToSharedMemory(char time[]);
void freeSpace(void *param);


int main(void)
{
	/*Start the clock*/
	//clock_t begin = clock() * 1000000 /CLOCKS_PER_SEC; //we want it in milliseconds so we can accurately see how much time has been taken
	
	struct timeval start, end;

	gettimeofday(&start, 0);
	
	struct ThreadParams threadParams;
	/*Check that the data initialisation was successful*/
	if(!initializeData(&threadParams)){
		printf("there was an error initialising");
		return 0;
	}
	/*Create thread IDs*/
	pthread_t tid1, tid2, tid3;
	/*Create the threads*/
	pthread_create(&tid1, NULL, runnerOne, &threadParams);
	pthread_create(&tid2, NULL, runnerTwo, &threadParams);
	pthread_create(&tid3, NULL, runnerThree, &threadParams);

	/*Waits for thread termination*/
	if(pthread_join(tid1, NULL) !=0
	&& pthread_join(tid2, NULL) != 0
	&&pthread_join(tid3, NULL) != 0){
		printf("error with thread join");
	}

	/*Free up space and memory by deleting what we no longer need*/
	freeSpace(&threadParams);

	/*Record end clock time*/
	//clock_t end = clock() * 1000000 /CLOCKS_PER_SEC;
	gettimeofday(&end, 0);
	
	/*String used to store the time spent*/
	char time_spent[1024];
	/*Calculate the time spent and write the time spent to the string*/
	sprintf(time_spent, "%ld", ((end.tv_usec)- (start.tv_usec)));	// Calculate Time
	printf("Duration running: %s Microseconds\n", time_spent);
	/*Write the value of the string to shared memory*/
	addToSharedMemory(time_spent);

}

void *runnerOne(void *param)
{
	/*Read parameter that was passed in*/
	struct ThreadParams *threadParams = param;
	FILE *file;
	/*Open data file*/
	file = fopen("data.txt", "r");
	/*Declare string*/
	char line[128];
	/*If the file exists*/
	if (file) {
		/*lock mutex so no other processes run - important to do this as we are assigning a value which may be read by other threads*/
		pthread_mutex_lock(&(threadParams->mutex));
		// /*Set file end to 0 meaning that it hasn't ended yet*/
		// threadParams->fileEnd = 0;
		/*We have finished writing to this value therefore can unlock and allow other threads to run*/
		pthread_mutex_unlock(&(threadParams->mutex));
		/*While we are reading lines of the file*/
		while ( fgets ( line, sizeof line, file ) != NULL )
     	{
			/*Wait until we have been signaled*/
			sem_wait(&(threadParams->one));
			/*fd[1] = write to pipe the line we have just read from document*/
			write(threadParams->fd[1], line, strlen(line));
			printf("%s", line);
			/*Signal the next thread to start*/
			sem_post(&(threadParams->two));
      	}
	  	/*Wait until we are signaled to close the file*/
      	sem_wait(&(threadParams->one));
     	fclose (file);
		/*Mutex lock as we are writing to another variable*/
     	pthread_mutex_lock(&(threadParams->mutex));
		 /*Signal that we are now at the end of the file and that there is no more lines to read*/
      	threadParams->fileEnd = 1;
		  /*Unlock so other processes can read from the params*/
     	pthread_mutex_unlock(&(threadParams->mutex));
		 /*Signal that sem2 can start again*/
     	sem_post(&(threadParams->two));
	}
	else{
		/*No file was found*/
		printf("file not found");
	}
}

void *runnerTwo(void *param)
{
	/*Read params*/
	struct ThreadParams *threadParams = param;
	/*While we haven't reached the end of the file*/
	while(!threadParams->fileEnd){
		/*Wait until sem 2 has been signaled*/
		sem_wait(&(threadParams->two));
		int n;
		/*fd[0] = read - Store the value that we have read from the pipe into outpipe*/
		n = read(threadParams->fd[0], threadParams->outPipe, MESSLENGTH);
		/*Null terminator - identifies end of the string*/
		threadParams->outPipe[n] = '\0';
		/*Signal that sem 3 can start*/
		sem_post(&(threadParams->three));
	}
}

void *runnerThree(void *param)
{
	/*Read from params*/
	struct ThreadParams *threadParams = param;
	/*Flag to indicate when we have reached the header file and therefore can start writing to the source file*/
	int canWrite = 0;
	FILE *file;
	/*Open the source file for writing*/
	file = fopen("src.txt", "w");
	printf("file end3: %d\n", threadParams->fileEnd);
	/*While we haven't reached the end of the file*/
	while(!threadParams->fileEnd){
		/*Wait until it has been signaled*/
		sem_wait(&(threadParams->three));
		/*If can write is true*/
		if(canWrite){
			/*Write to file the value that is in outPipe*/
			fprintf(file, "%s", threadParams->outPipe);

			printf("canWrite = %d\n",canWrite);
		}
		/*String we are looking for to indicate that we can start writing to the file*/
		char check[] = "end_header";
		/*sizeof(check)-1 because we dont want to include the new line character that is at the end of the file*/
        /*Compare the string in the outPipe to the value of the check string to see if they match*/
		if (strncmp(threadParams->outPipe, check, sizeof(check)-1) == 0 ){
            /*If they match then we can start writing the strings to the source file*/
			canWrite = 1;
        }
		/*Signal that sem 1 can now start*/
		sem_post(&(threadParams->one));
	}
}


int initializeData(void *param)
{
	/*Get params*/
	struct ThreadParams *threadParams = param;
	threadParams->outPipe = malloc(MESSLENGTH*sizeof(int));
	/*Try and create initialize mutex and print error if unsuccessful*/
	if(pthread_mutex_init(&(threadParams->mutex), NULL) != 0){
   		printf("there was an issue creating the mutex");
  	 }
	/*Initialise semaphores*/
   sem_init(&(threadParams->one), 0, 0);
   sem_init(&(threadParams->two), 0, 0);
   sem_init(&(threadParams->three), 0, 0);
   /*Signal that sem 1 can start*/
   sem_post(&(threadParams->one));
	/*Lock while we are writing to the variable*/
   pthread_mutex_lock(&(threadParams->mutex));
   /*Initialise file end to 0 to inicate that we haven't finished reading the file yet*/
   threadParams->fileEnd = 0;
   /*Unlock now that we have finished writing to variable*/
   pthread_mutex_unlock(&(threadParams->mutex));

	/*Create pipe*/
   if(pipe(threadParams->fd) < 0 ){
	   /*Error creating pipe*/
		perror("pipe error");
	}
	return 1;
}

int addToSharedMemory(char time[]){
	/* the size (in bytes) of shared memory object */
  	const int SIZE = 1024;
	/* Name of the shared memory object */
  	const char *name = "shared";
 	/* Shared memory file descriptor */
  	int shm_fd;
 	/* Pointer to shared memory object */
  	char *ptr;

  	/* Create the shared memory object */
  	if ((shm_fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR))< 0){
		printf("Issue creating shared memory object\n");
	}
	else{
	
		ftruncate(shm_fd, SIZE);
		/* Memory map the shared memory object */
		ptr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if(ptr != MAP_FAILED){
			/*Allocate the time variable to memory*/
			memcpy(ptr,time,sizeof(&time));
			printf("successfully mapped to shared memory\n");
			return 1;
		}
	}
	return 0;
}

void freeSpace(void *param)
{
	/* Declare variables to be used*/
	struct ThreadParams *threadParams = param;

	/* Free Resources */
	/*Free Pipe*/
	free(threadParams->outPipe);
	/*Destroy mutex*/
	pthread_mutex_destroy(&(threadParams->mutex));
	/*Destroy semaphores*/
	sem_destroy(&(threadParams->one));
	sem_destroy(&(threadParams->two));
	sem_destroy(&(threadParams->three));

}
