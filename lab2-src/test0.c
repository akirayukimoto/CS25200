#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "MyMalloc.h"

size_t size = 699000;

/*
 * Runs a simple test to allocate memory using your allocated.
 * Modify this file or use it as a base to create your own more complex tests.
 */
int main(int argc, char **argv) {
  printf("\n---- Running test0 ---\n");

  char *mem1 = (char *)malloc(699000);
  memset(mem1, 'a', 699000);
  char *mem2 = (char *)malloc(699000);
  memset(mem2, 'b', 699000);
  char *mem3 = (char *)malloc(699000);
  memset(mem3, 'c', 699000);

  print_list();

  char *mem4 = (char *)malloc(699000);
  memset(mem4, 'a', size);
  char *mem5 = (char *)malloc(size);
  memset(mem5, 'a', size);
  char *mem6 = (char *)malloc(size);
  memset(mem6, 'a', size);

  char *mem7 = (char *)malloc(2097152);
  memset(mem7, 'a', 2097152);
  char *mem8 = (char *)malloc(size);
  memset(mem8, 'a', size);
  char *mem9 = (char *)malloc(size);
  memset(mem9, 'a', size);
  char *mem10 = (char *)malloc(size);
  memset(mem10, 'a', size);

  printf("\n---- After Allocation ---\n");
  print_list();

  printf("\n---- After Free ---\n");
  free(mem2);
  free(mem4);
  free(mem6);
  free(mem8);
  free(mem10);

  free(mem7);
  
  print_list();


  exit(0);
} /* main() */
