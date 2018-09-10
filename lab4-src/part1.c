#include "part1.h"

#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// The producers add the characters in prod_str to the buffer, in order.

char *prod_str = "Hello, World!";

bounded_buffer buffer;

// This mutex must be held whenever you use the buffer.

pthread_mutex_t buffer_mutex;

// empty_sem is the number of characters in the buffer, that need to be emptied.
// Producers should signal empty_sem and consumers should wait on it.

sem_t empty_sem;

// full_sem is the opposite of empty_sem. It is the number of unfilled slots in
// the buffer that need to be filled. Consumers should signal it, and producers
// should wait on it.

sem_t full_sem;

// count how many characters are in the buffer
//int count;

// prod_cvar is the condition variable for producer thread

// pthread_cond_t prod_cvar;

/*
 * Produce the characters (that is, add them to the buffer) from prod_str, in
 * order. Signal consumers appropriately after each character. Receive an ID via
 * an (int *).
 */

void* producer(void *ptr) {
//  pthread_mutex_lock(&buffer_mutex);
//  int thread_id = *((int *)ptr);
  int thread_id = atoi((char *)ptr);
//  free(ptr);
//  ptr = NULL;
  
  printf("Producer %d starting\n", thread_id);
  fflush(NULL);

  //pthread_mutex_lock(&buffer_mutex);

  for (int i = 0; i < strlen(prod_str); i++) {
    // Add your code to wait on the semaphore and obtain the lock,
    // then add prod_str[i] to the buffer.
    //sleep(0.05);
    sem_wait(&full_sem);
    pthread_mutex_lock(&buffer_mutex);
    
    printf("Thread %d produced %c\n", thread_id, prod_str[i]);

    buffer.buf[buffer.tail] = prod_str[i];
//    int new_tail = buffer.tail;
    buffer.tail = (buffer.tail + 1) % BUF_SIZE;
//    buffer.tail = new_tail;
//    count++;

    pthread_mutex_unlock(&buffer_mutex);
    sem_post(&empty_sem);
//    sleep(0.05);
  }

//  pthread_mutex_unlock(&buffer_mutex);
  pthread_exit(0);
} /* producer() */

/*
 * Consume characters from the buffer. Stop after consuming the length of
 * prod_str, meaning that if an equal number of consumers and producers are
 * started, they will all exit. Signal producers appropriately of new free space
 * in the buffer. Receive and ID as an argument via an (int *).
 */

void* consumer(void *ptr) {
//  int thread_id = *((int *)ptr);
  int thread_id = atoi((char *)ptr);
//  free(ptr);
//  ptr = NULL;

  //pthread_mutex_lock(&buffer_mutex);

  printf("Consumer %d starting\n", thread_id);
  fflush(NULL);

  for (int i = 0; i < strlen(prod_str); i++) {
   // Add your code to wait on the semaphore and obtain the lock,
    // then consume prod_str[i] from the buffer, replacing
    // the following line.
//    sleep(0.05);
    sem_wait(&empty_sem);

    pthread_mutex_lock(&buffer_mutex);

    //char c = prod_str[i];

    printf("Thread %d consumed %c\n", thread_id, prod_str[i]);

    //buffer.head = (buffer.head + 1) % BUF_SIZE;
//    int new_head = buffer.head;
//    new_head = (new_head + 1) % BUF_SIZE;
    buffer.head = (buffer.head + 1) % BUF_SIZE;
//    count--;

    pthread_mutex_unlock(&buffer_mutex);
    sem_post(&full_sem);
//    sleep(0.05);
  }

  //pthread_mutex_unlock(&buffer_mutex);

  pthread_exit(0);
} /* consumer() */

/*
 * Start a number of producers indicated by the first argument, and a number of
 * consumers indicated by the second argument. Wait on all threads at the end to
 * prevent premature exit.
 */

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Please pass two arguments.");
    exit(1);
  }

  // Initialize the buffer_mutex and condition variables

  pthread_mutex_init(&buffer_mutex, NULL);

  // Since the buffer starts out with no characters, empty_sem should be 0 and
  // full_sem should be the full size of the buffer.

  sem_init(&empty_sem, 0, 0);
  sem_init(&full_sem, 0, BUF_SIZE);

  // Add your code to create the threads.
  // Make sure to allocate and pass the arguments correctly.
//  pthread_cond_init(&cons_cvar, NULL);
//  pthread_cond_init(&prod_cvar, NULL);
//  count = 0;
  buffer.head = 0;
  buffer.tail = 0;

  pthread_t producer_thread[atoi(argv[1])];
  pthread_t consumer_thread[atoi(argv[2])];
//  pthread_t producer_thread;
//  pthread_t consumer_thread;

//  pthread_detach(consumer_thread);
//  pthread_detach(producer_thread);
//  pthread_mutex_init(&consumer_thread, NULL);
//  pthread_mutex_init(&producer_thread, NULL);
//  pthread_create(&consumer_thread, NULL, consumer, (void *)buffer.buf);
//  pthread_create(&producer_thread, NULL, producer, (void *)buffer.buf);
//  pthread_create(&consumer_thread, NULL, consumer, (void *)buffer.buf);
  // Add your code to wait for the threads to finish.
  // Otherwise main might run to the end
  // and kill the entire process when it exits.
  int i;

  for (i = 0; i < atoi(argv[1]); i++) {
    char *index = (char *)malloc(sizeof(int));
    sprintf(index, "%d", i);
//    printf("producer current index: %s\n", index);
    pthread_create(&producer_thread[i], NULL, producer, (void *)index);
//    pthread_create(&producer_thread[i], NULL, producer, (void *)(&i));
//    free(index);
//    index = NULL;
//    pthread_join(producer_thread, NULL);
  }

  for (i = 0; i < atoi(argv[2]); i++) {
    char *index2 = (char *)malloc(sizeof(int));
    sprintf(index2, "%d", i);
//    printf("consumer current index: %s\n", index2);
    pthread_create(&consumer_thread[i], NULL, consumer, (void *)index2);
//    pthread_create(&consumer_thread[i], NULL, consumer, (void *)(&i));
//    free(index2);
//    index2 = NULL;
//    pthread_join(consumer_thread, NULL);
  }

  for (i = 0; i < atoi(argv[1]); i++) {
    pthread_join(producer_thread[i], NULL);
  }

  for (i = 0; i < atoi(argv[2]); i++) {
    pthread_join(consumer_thread[i], NULL);
  }

//  pthread_mutex_destroy(&buffer_mutex);
/*
  for (i = 0; i < atoi(argv[2]); i++) {
    pthread_detach(consumer_thread[i]);
  }
  for (i = 0; i < atoi(argv[1]); i++) {
    pthread_detach(producer_thread[i]);
  }
*/
  pthread_mutex_destroy(&buffer_mutex);

  return 0;
} /* main() */
