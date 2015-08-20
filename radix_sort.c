#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/time.h>

#define BYTES 1
#define SHIFT 8*BYTES
#define MASK ((1U << (SHIFT)) - 1)
#define RADIX ((MASK) + 1)


double tdiff(struct timeval *before, struct timeval*after)
{
  unsigned long usecbefore =
    ((unsigned long)before->tv_sec)*1000000 +
    (unsigned long)before->tv_usec;
  unsigned long usecafter =
    ((unsigned long)after->tv_sec)*1000000 +
    (unsigned long)after->tv_usec;
  return (((double)usecafter) - usecbefore)/1000000.0;
}

void radix_sort(unsigned int *input,
		unsigned int *aux,
		unsigned int *counts,
		unsigned int keys,
		unsigned int max_key)
{
  unsigned int *src = input;
  unsigned int *dst = aux;
  unsigned int *tmp;

  unsigned int shift_amt = 0;

  // Determine needed passes - do at least 1
  int passes   = 1;
  int possible_max_key = RADIX;
  while(1) {
    if((possible_max_key - 1) >= (max_key - 1)) {
      break;
    }
    possible_max_key = possible_max_key * RADIX;
    passes++;
  }

  struct timeval t_before;
  struct timeval t_after;

  gettimeofday(&t_before, NULL);
    
  for(unsigned int i=0;i<passes;i++) {
    memset(counts, 0, RADIX*sizeof(unsigned int));
    // Count
    for(unsigned int j=0;j<keys;j++) {
      unsigned int key = src[j];
      key = (key >> shift_amt) & MASK;
      counts[key]++;
    }
    // Compute positions
    unsigned int marker = 0;
    for(unsigned int j=0;j<RADIX;j++) {
      unsigned int tmp2 = counts[j];
      counts[j] = marker;
      marker += tmp2;
    }
    // Copy
    for(unsigned int j=0;j<keys;j++) {
      unsigned int key = src[j];
      key = (key >> shift_amt) & MASK;
      dst[counts[key]] = src[j];
      counts[key]++;
    }
    tmp = src;
    src = dst;
    dst = tmp;
    shift_amt += SHIFT;
  }

  gettimeofday(&t_after, NULL);

  printf("MAX_KEY=%d PASSES=%d TIME=%lf\n",
	 max_key,
	 passes,
	 tdiff(&t_before, &t_after));
  // Test sort
  for(unsigned int i=1;i<keys;i++) {
    if(src[i-1] > src[i]) {
      fprintf(stderr, 
	      "FAILED [%d]=%d [%d]=%d\n",
	      i-1, src[i-1], i, src[i]);
      exit(-1);
    }
  }
  fprintf(stderr, "PASSED\n");
}

int main(int argc, char *argv[])
{
  unsigned int keys = (1 << 29);
  if(argc < 2) {
    fprintf(stderr, "Usage: %s max_key\n", argv[0]);
    exit(-1);
  }
  unsigned int max_key = atoi(argv[1]);
  unsigned int *counts = (unsigned int *)malloc(RADIX*sizeof(unsigned int));
  unsigned int *data = (unsigned int *)malloc(keys*sizeof(unsigned int));
  unsigned int *aux  = (unsigned int *)malloc(keys*sizeof(unsigned int));
  if(counts == NULL || data == NULL || aux == NULL) {
    fprintf(stderr, "Malloc failed\n");
  }
  // Deterministic random input
  srand48(0xdeadbeef);
  for(unsigned int i=0;i<keys;i++) {
    data[i] = ((int)mrand48()) % max_key;
  }
  radix_sort(data, aux, counts, keys, max_key);
  free(counts);
  free(data);
  free(aux);
  return 0;
}

