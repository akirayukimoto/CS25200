//
// CS252: MyMalloc Project
//
// The current implementation gets memory from the OS
// every time memory is requested and never frees memory.
//
// You will implement the allocator as indicated in the handout.
//
// Also you will need to add the necessary locking mechanisms to
// support multi-threaded programs.
//

#include "MyMalloc.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>


// This mutex must be held whenever modifying internal allocator state,
// or referencing state that is subject to change. The skeleton should
// take care of this automatically, unless you make modifications to the
// C interface (malloc(), calloc(), realloc(), and free()).

static pthread_mutex_t mutex;

// The size of block to get from the OS (2MB).

#define ARENA_SIZE ((size_t) 2097152)


// STATE VARIABLES

// Sum total size of heap

static size_t heap_size;

// Start of memory pool

static void *mem_start;

// Number of chunks requested from OS so far

static int num_chunks;

// Verbose mode enabled via environment variable
// (See initialize())

static int verbose;

// Keep track of the number of calls to each function

static int malloc_calls;
static int free_calls;
static int realloc_calls;
static int calloc_calls;

// The free list is a doubly-linked list, with a constant sentinel.

static object_header free_list_sentinel;
static object_header *free_list;


/*
 * Increments the number of calls to malloc().
 */

void increase_malloc_calls() {
  malloc_calls++;
} /* increase_malloc_calls() */

/*
 * Increase the number of calls to realloc().
 */

void increase_realloc_calls() {
  realloc_calls++;
} /* increase_realloc_calls() */

/*
 * Increase the number of calls to calloc().
 */

void increase_calloc_calls() {
  calloc_calls++;
} /* increase_calloc_calls() */

/*
 * Increase the number of calls to free().
 */

void increase_free_calls() {
  free_calls++;
} /* increase_free_calls() */

/*
 * externed version of at_exit_handler(), which can be passed to atexit().
 * See atexit(3).
 */

extern void at_exit_handler_in_c() {
  at_exit_handler();
} /* at_exit_handler_in_c() */

/*
 * Initialize the allocator by setting initial state
 * and making the first allocation.
 */

void initialize() {
  // Set this environment variable to the specified value
  // to disable verbose logging.

#define VERBOSE_ENV_VAR "MALLOCVERBOSE"
#define VERBOSE_DISABLE_STRING "NO"

  pthread_mutex_init(&mutex, NULL);

  // We default to verbose mode, but if it has been disabled in
  // the environment, disable it correctly.

  const char *env_verbose = getenv(VERBOSE_ENV_VAR);
  verbose = (!env_verbose || strcmp(env_verbose, VERBOSE_DISABLE_STRING));

  // Disable printf's buffer, so that it won't call malloc and make
  // debugging even more difficult

  setvbuf(stdout, NULL, _IONBF, 0);

  // Get initial memory block from OS

  void *new_block = get_memory_from_os(ARENA_SIZE +
                                       (2 * sizeof(object_header)) +
                                       (2 * sizeof(object_footer)));

  // In verbose mode register function to print statistics at exit

  atexit(at_exit_handler_in_c);

  // Establish memory locations for objects within the new block

  object_footer *start_fencepost = (object_footer *) new_block;
  object_header *current_header =
    (object_header *) ((char *) start_fencepost +
                              sizeof(object_footer));
  object_footer *current_footer =
    (object_footer *) ((char *) current_header +
                              ARENA_SIZE +
                              sizeof(object_header));
  object_header *end_fencepost =
    (object_header *) ((char *) current_footer +
                              sizeof(object_footer));

  // Establish fenceposts
  // We set fencepost size to 0 as an arbitrary value which would
  // be impossible as a value for a valid memory block

  start_fencepost->status = ALLOCATED;
  start_fencepost->object_size = 0;

  end_fencepost->status = ALLOCATED;
  end_fencepost->object_size = 0;
  end_fencepost->next = NULL;
  end_fencepost->prev = NULL;

  // Establish main free object

  current_header->status = UNALLOCATED;
  current_header->object_size = ARENA_SIZE +
                                sizeof(object_header) +
                                sizeof(object_footer);

  current_footer->status = UNALLOCATED;
  current_footer->object_size = current_header->object_size;

  // Initialize free list and add the new first object

  free_list = &free_list_sentinel;
  free_list->prev = current_header;
  free_list->next = current_header;
  current_header->next = free_list;
  current_header->prev = free_list;

  // Mark sentinel as such. Do not coalesce the sentinel.

  free_list->status = SENTINEL;
  free_list->object_size = 0;

  // Set start of memory pool

  mem_start = (char *) current_header;
} /* initialize() */

/*
 * Allocate an object of size size. Ideally, we can allocate from the free list,
 * but if we don't have a free object large enough, go get more memory from the
 * OS. Return a pointer to the newly allocated memory.
 *
 * TODO: Add your code to allocate memory from the free list, instead of getting
 * it from the OS every time.
 */

void *allocate_object(size_t size) {
  // SIZE_PRECISION determines how to round.
  // By default, round up to nearest 8 bytes.
  // It must be a power of 2.
  // MINIMUM_SIZE is the minimum size that can be requested, not including
  // header and footer. Smaller requests are rounded up to this minimum.

#define SIZE_PRECISION (8)
#define MINIMUM_SIZE (8)

  if (size < MINIMUM_SIZE) {
    size = MINIMUM_SIZE;
  }

  // Add the object_header/Footer to the size and round the total size
  // up to a multiple of 8 bytes for alignment.
  // Bitwise-and with ~(SIZE_PRECISION - 1) will set the last x bits to 0,
  // if SIZE_PRECISION = 2**x.

  size_t rounded_size = (size +
                         sizeof(object_header) +
                         sizeof(object_footer) +
                         (SIZE_PRECISION - 1)) & ~(SIZE_PRECISION - 1);

  // For now, naively get the memory from the OS every time.
  // (You need to change this.)
  size_t max_size = ARENA_SIZE + sizeof(object_header) + sizeof(object_footer);
  if (rounded_size > max_size) {
    return NULL;
  }
 
  object_header *current = free_list->next;
  // Go through the free list and find the first-fit block to use

  // Also need to check where the fencepost is
  while (current != &free_list_sentinel) {
    // 1. When the current block should be split
    // 2. When the current block can directly return
    // 3. When the current block is not enough
    void *temp = (void *)((char *)current + sizeof(object_header));
    if (object_size(temp) >= rounded_size) {
      size_t remainder = object_size(temp) - rounded_size;
      size_t min_size = MINIMUM_SIZE + 
                        sizeof(object_header) + 
                        sizeof(object_footer);
      // Directly return: when the remain size is not enough
      if (remainder < min_size) {
        current->status = ALLOCATED;
        object_footer *cur_footer =
          (object_footer *)((char *)current +
                             object_size(temp) -
                             sizeof(object_footer));
        cur_footer->status = ALLOCATED;

        current->prev->next = current->next;
        current->next->prev = current->prev;
        current->prev = NULL;
        current->next = NULL;

        return (void *)(object_header *)(current + 1);
      }
      
      // Split: when the current block is large enough
      else if (remainder >= min_size) {
        object_header *new_header = (object_header *)((char *)current + 
                                                      rounded_size);
        object_footer *cur_footer = 
          (object_footer *)((char *)current + 
                            rounded_size - 
                            sizeof(object_footer));
        object_footer *new_footer = 
          (object_footer *)((char *)current + 
                            current->object_size -
                            sizeof(object_footer));

        new_header->object_size = current->object_size - rounded_size;
        new_footer->object_size = new_header->object_size;
        new_header->status = UNALLOCATED;
        new_footer->status = UNALLOCATED;

        current->object_size = rounded_size;
        current->status = ALLOCATED;

        cur_footer->object_size = current->object_size;
        cur_footer->status = ALLOCATED;

        current->prev->next = new_header;
        new_header->prev = current->prev;
        new_header->next = current->next;
        current->next->prev = new_header;
        current->prev = NULL;
        current->next = NULL;

        return (void *)(object_header *)(current + 1);
        
      }
    }
    else {
      current = current->next;
    }
  }
  // Since there is no fit block, we request a new object 
  // and receive what we need

  // Return a pointer to usable memory

  // When the program arrives here, it means there is no fit block in the free
  // list, so we required a new chunk for use.

  void *new_block = get_memory_from_os(ARENA_SIZE +
                                       2 * sizeof(object_header) +
                                       2 * sizeof(object_footer));

  object_footer *start_fencepost = (object_footer *)new_block;
  object_header *new_object = 
    (object_header *)((char *)start_fencepost +
                              sizeof(object_footer));
  object_footer *new_footer = 
    (object_footer *)((char *)new_object +
                             rounded_size -
                             sizeof(object_footer));
  object_header *new_next_header = 
    (object_header *)((char *)new_object + rounded_size);
  object_footer *new_next_footer =
    (object_footer *)((char *)new_object + sizeof(object_header) + ARENA_SIZE);
  object_header *end_fencepost = 
    (object_header *)((char *)new_next_footer + sizeof(object_footer));

  start_fencepost->status = ALLOCATED;
  start_fencepost->object_size = 0;

  end_fencepost->status = ALLOCATED;
  end_fencepost->object_size = 0;
  end_fencepost->next = NULL;
  end_fencepost->prev = NULL;

  // After grab new chunk and marked its fenceposts
  // Check whether the block should be split or not

  size_t real_size = ARENA_SIZE + 
                     sizeof(object_header) + 
                     sizeof(object_footer);
  // When the required size is less than the new chunk
  // Split the new chunk
  if (rounded_size < real_size) {
    new_next_header->status = UNALLOCATED;
    new_next_header->object_size = real_size - rounded_size;
    new_next_footer->object_size = new_next_header->object_size;
    new_next_footer->status = UNALLOCATED;

    new_next_header->prev = free_list->prev;
    free_list->prev = new_next_header;
    new_next_header->prev->next = new_next_header;
    new_next_header->next = free_list;

    new_object->status = ALLOCATED;
    new_object->object_size = rounded_size;
    new_object->prev = NULL;
    new_object->next = NULL;
    new_footer->object_size = new_object->object_size;
    new_footer->status = ALLOCATED;
  }
  // When the required size is equal to the new chunk
  // Use the new chunk directly
  else if (rounded_size == real_size) {
    new_object->status = ALLOCATED;
    new_footer->status = ALLOCATED;
    new_object->object_size = rounded_size;
    new_footer->object_size = rounded_size;
    new_object->prev = NULL;
    new_object->next = NULL;
  }
  
  return (void *) (new_object + 1);
} /* allocate_object() */

/*
 * Free an object. ptr is a pointer to the usable block of memory in
 * the object. If possible, coalesce the object, then add to the free list.
 *
 * TODO: Add your code to free the object
 */

void free_object(void *ptr) {
  // Add your code here
  object_header *current = 
    (object_header *)((char *)ptr - sizeof(object_header));
  object_footer *current_footer = 
    (object_footer *)((char *)current + 
                      current->object_size - 
                      sizeof(object_footer));
  object_footer *left_footer = 
    (object_footer *)((char *)current - sizeof(object_footer));
  object_header *right_header =
    (object_header *)((char *)current + current->object_size);

  // There are four cases for free

  // 1. neither the left nor right neighbors are free
  if ((left_footer->status != UNALLOCATED) &&
  (right_header->status != UNALLOCATED)) {
    current->status = UNALLOCATED;
    current_footer->status = UNALLOCATED;
    // Find the closest free block
    object_header *temp = &free_list_sentinel;
    while (temp->next != &free_list_sentinel) {
      if (temp < current && temp->next > current) {
        break;
      }
      else {
        temp = temp->next;
      }
    }
    temp->next->prev = current;
    current->next = temp->next;
    temp->next = current;
    current->prev = temp;
  }
  // 2. if the left neighbor is free
  else if ((left_footer->status == UNALLOCATED) &&
  (right_header->status == ALLOCATED)) {
    current->status = UNALLOCATED;
    current_footer->status = UNALLOCATED;

    object_header *left_header =
      (object_header *)((char *)left_footer -
                        left_footer->object_size +
                        sizeof(object_footer));

    left_header->object_size = 
      left_header->object_size + current->object_size;
    current_footer->object_size = left_header->object_size;
  }
  // 3. if the right neighbor is free
  else if ((right_header->status == UNALLOCATED) &&
  (left_footer->status == ALLOCATED)) {
    current->status = UNALLOCATED;
    current_footer->status = UNALLOCATED;
    current->object_size =
      current->object_size + right_header->object_size;

    object_footer *right_footer =
      (object_footer *)((char *)right_header +
                        right_header->object_size -
                        sizeof(object_footer));
    right_footer->object_size = current->object_size;

    current->prev = right_header->prev;
    current->next = right_header->next;
    right_header->prev->next = current;
    right_header->next->prev = current;
    right_header->prev = NULL;
    right_header->next = NULL;
  }
  // 4. both the left and right neighbors are free
  else {
    object_header *left_header = 
      (object_header *)((char *)left_footer -
                        left_footer->object_size +
                        sizeof(object_footer));
    object_footer *right_footer = 
      (object_footer *)((char *)right_header +
                        right_header->object_size -
                        sizeof(object_footer));
    size_t new_size = left_footer->object_size + 
                      current->object_size +
                      right_header->object_size;
    left_header->object_size = new_size;
    right_footer->object_size = new_size;

    left_header->next = right_header->next;
    right_header->next->prev = left_header;
    right_header->prev = NULL;
    right_header->next = NULL;
  }


} /* free_object() */

/*
 * Return the size of the object pointed by ptr. We assume that ptr points to
 * usable memory in a valid obejct.
 */

size_t object_size(void *ptr) {
  // ptr will point at the end of the header, so subtract the size of the
  // header to get the start of the header.

  object_header *object =
    (object_header *) ((char *) ptr - sizeof(object_header));

  return object->object_size;
} /* object_size() */

/*
 * Print statistics on heap size and
 * how many times each function has been called.
 */

void print_stats() {
  printf("\n-------------------\n");

  printf("HeapSize:\t%zd bytes\n", heap_size);
  printf("# mallocs:\t%d\n", malloc_calls);
  printf("# reallocs:\t%d\n", realloc_calls);
  printf("# callocs:\t%d\n", calloc_calls);
  printf("# frees:\t%d\n", free_calls);

  printf("\n-------------------\n");
} /* print_stats() */

/*
 * Print a representation of the current free list.
 * For each object in the free list, show the offset (distance in memory from
 * the start of the memory pool, mem_start) and the size of the object.
 */

void print_list() {
  pthread_mutex_lock(&mutex);

  printf("FreeList: ");

  object_header *ptr = free_list->next;

  while (ptr != free_list) {
    long offset = (long) ptr - (long) mem_start;
    printf("[offset:%ld,size:%zd]", offset, ptr->object_size);
    ptr = ptr->next;
    if (ptr != free_list) {
      printf("->");
    }
  }
  printf("\n");

  pthread_mutex_unlock(&mutex);
} /* print_list() */

/*
 * Use sbrk() to get the memory from the OS. See sbrk(2).
 */

void *get_memory_from_os(size_t size) {
  heap_size += size;

  void *new_block = sbrk(size);

  num_chunks++;

  return new_block;
} /* get_memory_from_os() */

/*
 * Run when the program exists, and prints final statistics about the allocator.
 */

void at_exit_handler() {
  if (verbose) {
    print_stats();
  }
} /* at_exit_handler() */



//
// C interface
//


/*
 * Allocates size bytes of memory and returns the pointer to the
 * newly-allocated memory. See malloc(3).
 */

extern void *malloc(size_t size) {
  pthread_mutex_lock(&mutex);
  increase_malloc_calls();

  void *memory = allocate_object(size);

  pthread_mutex_unlock(&mutex);

  return memory;
} /* malloc() */

/*
 * Frees a block of memory allocated by malloc(), calloc(), or realloc().
 * See malloc(3).
 */

extern void free(void *ptr) {
  pthread_mutex_lock(&mutex);
  increase_free_calls();

  if (ptr != NULL) {
    free_object(ptr);
  }

  pthread_mutex_unlock(&mutex);
} /* free() */

/*
 * Resizes the block of memory at ptr, which was allocated by
 * malloc(), realloc(), or calloc(), and returns a pointer to
 * the new resized block. See malloc(3).
 */

extern void *realloc(void *ptr, size_t size) {
  pthread_mutex_lock(&mutex);
  increase_realloc_calls();

  void *new_ptr = allocate_object(size);

  pthread_mutex_unlock(&mutex);

  // Copy old object only if ptr is non-null

  if (ptr != NULL) {
    // Copy everything from the old ptr.
    // We don't need to hold the mutex here because it is undefined behavior
    // (a double free) for the calling program to free() or realloc() this
    // memory once realloc() has already been called.

    size_t size_to_copy =  object_size(ptr);
    if (size_to_copy > size) {
      // If we are shrinking, don't write past the end of the new block

      size_to_copy = size;
    }

    memcpy(new_ptr, ptr, size_to_copy);

    pthread_mutex_lock(&mutex);
    free_object(ptr);
    pthread_mutex_unlock(&mutex);
  }

  return new_ptr;
} /* realloc() */

/*
 * Allocates contiguous memory large enough to fit num_elems elements
 * of size elem_size. Initialize the memory to 0. Return a pointer to
 * the beginning of the newly-allocated memory. See malloc(3).
 */

extern void *calloc(size_t num_elems, size_t elem_size) {
  pthread_mutex_lock(&mutex);
  increase_calloc_calls();

  // Find total size needed

  size_t size = num_elems * elem_size;

  void *ptr = allocate_object(size);

  pthread_mutex_unlock(&mutex);

  if (ptr) {
    // No error, so initialize chunk with 0s

    memset(ptr, 0, size);
  }

  return ptr;
} /*doku.php?id calloc() */
