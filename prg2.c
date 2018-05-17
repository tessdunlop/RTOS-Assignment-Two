/*
 * Subject RTOS 48450
 * Project: Assignment 2 - Prg2
 * Name: Tess Dunlop
 * Student Number: 11655646
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void){
	/* the size (in bytes) of shared memory object */
  	const int SIZE = 1024;
	/* NAME OF THE SHARED MEMORY OBJECT */
  	const char *name = "shared";
 	/* shared memory file descriptor */
  	int shm_fd;
 	/* pointer to shared memory object */
  	char *ptr;

  	/* create the shared memory object */
  	if ((shm_fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR))< 0){
		printf("Issue creating shared memory object\n");
	}
	else{
		ftruncate(shm_fd, SIZE);
		/* memory map the shared memory object */
		ptr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
			if(ptr != MAP_FAILED){
			printf("Duration: %s Microseconds\n", ptr);
		}
	}
}
