#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

//rotate/flip a quadrant appropriately
void rot(unsigned long n, 
	 unsigned long *x, 
	 unsigned long *y, 
	 unsigned long rx,
	 unsigned long ry) 
{
  if (ry == 0) {
    if (rx == 1) {
      *x = n-1 - *x;
      *y = n-1 - *y;
    }

    //Swap x and y
    unsigned long t  = *x;
    *x = *y;
    *y = t;
  }
}

//convert (x,y) to d
unsigned long xy2d (unsigned long n, unsigned long x, unsigned long y) 
{
  unsigned long rx, ry, s, d=0;
  for (s=n/2; s>0; s/=2) {
    rx = (x & s) > 0;
    ry = (y & s) > 0;
    d += s*s*((3 * rx) ^ ry);
    rot(s, &x, &y, rx, ry);
  }
  return d;
}

//convert d to (x,y)
void d2xy(unsigned long n, unsigned long d, unsigned long *x, unsigned long *y) 
{
  unsigned long rx, ry, s, t=d;
  *x = *y = 0;
  for (s=1; s<n; s*=2) {
    rx = 1 & (t/2);
    ry = 1 & (t ^ rx);
    rot(s, x, y, rx, ry);
    *x += s * rx;
    *y += s * ry;
    t /= 4;
  }
}

unsigned int *perm;

void setup_permutation(unsigned int n)
{
  perm[0] = 0;
  unsigned int i, tmp;
  for(i=1;i<n;i++) {
    perm[i] = i;
    unsigned int swap = lrand48() % (i + 1);
    if(swap != i) {
      tmp = perm[swap];
      perm[swap] = perm[i];
      perm[i] = tmp;
    }
  }
}


void hilbert_encode(unsigned char *buf, 
		    unsigned long size,
		    unsigned long n)
{
  unsigned long offset = 0;
  unsigned int old_x, old_y;
  unsigned long x, y;
  while(offset < size) {
    old_x = *(unsigned int *)(buf + offset);
    old_y = *(unsigned int *)(buf + offset + 4);
    if(old_x < (1U << 31)) {
      old_x = perm[old_x];
    }
    if(old_y < (1U << 31)) {
      old_y = perm[old_y];
    }
    *(unsigned long *)(buf + offset) = xy2d(n, (unsigned long)old_x, (unsigned long)old_y); 
    d2xy(n, *(unsigned long*)(buf + offset), &x, &y);
    if(old_x != (unsigned int)x) {
      printf("reverse check x failed %u != %u\n", old_x, (unsigned int)x);
    }
    if(old_y != (unsigned int)y) {
      printf("reverse check y failed %u != %u\n", old_y, (unsigned int)y);
    }
    offset += 8;
  }
}

unsigned long count_jumps(unsigned long *buf, unsigned long entries)
{
  unsigned long jmps = 0;
  unsigned long last = buf[0];
  unsigned long i;
  for(i=1;i<entries;i++) {
    if(buf[i] < last) 
      jmps++;
    last = buf[i];
  }
}

total_delta(unsigned long *buf, 
	    unsigned long entries,
	    unsigned long *f2p,
	    unsigned long *f4p,
	    unsigned long *f8p,
	    unsigned long *f16p)
{
  unsigned long last = buf[0];
  unsigned long i, diff;
  for(i=1;i<entries;i++) {
    if(last > buf[i]) {
      printf("sort failed !\n");
      exit(-1);
    }
    diff = buf[i] - last;
    if(diff < 4)
      (*f2p)++;
    else if(diff < 16)
      (*f4p)++;
    else if(diff  < 256)
      (*f8p)++;
    else if(diff < 655356)
      (*f16p)++;
    last = buf[i];
  }
}

int compar(const void * ap, const void *bp)
{
  unsigned long a = *(const unsigned long *)ap;
  unsigned long b = *(const unsigned long *)bp;
  if(a < b) {
    return -1;
  }
  else if(a > b) {
    return 1;
  }
  else {
    return 0;
  }
}


main()
{
  int fd = open("sample", O_RDWR);
  if(fd == -1) {
    printf("cnt open file\n");
    exit(-1);
  }
  char *buf = mmap(NULL, 1422253056, PROT_READ|PROT_WRITE,
		   MAP_SHARED, fd, 0);

  if(buf == MAP_FAILED) {
    printf("ap failed\n");
    exit(-1);
  }


  perm = (unsigned int *)mmap(NULL, 4*(1UL<<31), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  if(perm == MAP_FAILED) {
    printf("couldn't map perm:%s\n", strerror(errno));
    exit(-1);
  }
  unsigned long edges = 1422253056/8;
  unsigned long n = (1UL << 32);



  setup_permutation((1U << 31));
  hilbert_encode(buf, 1422253056, n);
  printf("encoding complete\n");
  qsort(buf, edges, 8, compar);
  printf("sorting done\n");
  unsigned long f2=0,f4=0,f8=0,f16=0;
  total_delta((unsigned long *)buf, edges, &f2, &f4, &f8, &f16);
  printf("frac_2_bit = %lf\n",  ((double)f2)/(edges - 1));
  printf("frac_4_bit = %lf\n",  ((double)f4)/(edges - 1));
  printf("frac_8_bit = %lf\n",  ((double)f8)/(edges - 1));
  printf("frac_16_bit = %lf\n",  ((double)f16)/(edges - 1));
}
