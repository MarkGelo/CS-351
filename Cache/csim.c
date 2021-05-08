/******************************************************************************
 *
 * Cache lab header
 *
 * Author: Mark Gameng
 * Email:  mgameng1@hawk.iit.edu
 * AID:    A20419026
 * Date:   04/10/21
 *
 * By signing above, I pledge on my honor that I neither gave nor received any
 * unauthorized assistance on the code contained in this repository.
 *
 *****************************************************************************/

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>

void show_help(){ // same help from csim-ref
  printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
   	"Options:\n"
  	"-h         Print this help message.\n"
  	"-v         Optional verbose flag.\n"
  	"-s <num>   Number of set index bits.\n"
  	"-E <num>   Number of lines per set.\n"
 	"-b <num>   Number of block offset bits.\n"
  	"-t <file>  Trace file.\n\n"

	"Examples:\n"
  	"linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
  	"linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

int s = -1; // init as neg first since will never be negative
int E = -1; // just to test if user specified
int b = -1;
int verbose = 0;
int hits = 0;
int misses = 0;
int evictions = 0;

// structs for easier readability

typedef struct{
    int tag;
    int valid;
    int lru_c;
} line;

typedef struct{
    line *lines;
} set;

typedef struct{
    set *sets;
} cache;

void caching(cache CACHE, unsigned long long address){
    unsigned long long tag, idx;
    int empty_idx = 0;
    int full = 1;

    tag = address >> (s + b);
    idx = (address << (64  - (s + b))) >> (64 - s); 
    set cur_set = CACHE.sets[idx];
    //return;

    for(int i = 0; i < E; i++){
      // line search
      //printf("%d", cur_set.lines[i].tag);
      //continue;
      if(cur_set.lines[i].valid && cur_set.lines[i].tag == tag){
        hits += 1;
        cur_set.lines[i].lru_c += 1;
        if(verbose){
          printf(" hit");
        }
        return;
      }else if(!cur_set.lines[i].valid && full){ // check if full and get empty line
        full = 0;
        empty_idx = i;
      }
    }

    misses += 1;
    if(verbose){
      printf(" miss");
    }

    // lru 
    int least = cur_set.lines[0].lru_c;
    int most = cur_set.lines[0].lru_c;
    int evict = 0;
    
    for(int i = 0; i < E; i++){
      if(cur_set.lines[i].lru_c < least){
        least = cur_set.lines[i].lru_c;
        evict = i;
      }
      if(cur_set.lines[i].lru_c > most){
        most = cur_set.lines[i].lru_c;
      }
    }
   
    if(full){
      // evict lru
      evictions += 1;
      if(verbose){
        printf(" eviction");
      }
      cur_set.lines[evict].tag = tag;
      cur_set.lines[evict].lru_c = most + 1;
    }else{ // there are empties, so no need to evict
      cur_set.lines[empty_idx].tag = tag;
      cur_set.lines[empty_idx].valid = 1;
      cur_set.lines[empty_idx].lru_c = most + 1;
    }    
}

void cache_sim(char* trace){
    //open file and init cache
    FILE *fp = fopen(trace, "r");
    if(fp == NULL){
      printf("Trace file invalid");
      exit(0);
    }
    
    cache CACHE;
    int n_sets;
    n_sets = 1 << s;
    CACHE.sets = (set *) malloc(sizeof(set) * n_sets);
    
    for(int i = 0; i < n_sets; i++){
      set cur_set;
      cur_set.lines = (line *) malloc(sizeof(line) * E);
      CACHE.sets[i] = cur_set;
      for(int j = 0; j < E; j++){
        line cur_line;
        cur_line.tag = 0;
        cur_line.valid = 0;
        cur_line.lru_c = 0;
        cur_set.lines[j] = cur_line;
      }
    }

    //simulate cache
    char operation;
    long long int address;
    int size;
    while(fscanf(fp, " %c %llx,%d", &operation, &address, &size) == 3){
      switch(operation){
        case 'I': // instruction load ignore
          break;
        case 'L': // load
          if(verbose){
            printf("L %llx,%d", address, size);
          }
          caching(CACHE, address);
          if(verbose){
            printf("\n");
          }
          break;
        case 'S': // store
          if(verbose){
            printf("S %llx,%d", address, size);
          }
          caching(CACHE, address);
          if(verbose){
            printf("\n");
          }
          break;
        case 'M': // modify = data load + data store
          if(verbose){
            printf("M %llx,%d", address, size);
          }
          caching(CACHE, address);
          caching(CACHE, address);
          if(verbose){
            printf("\n");
          }
          break;
        default:
          break;
      }
    }
   
    // should free everything
    for(int i = 0; i < n_sets; i ++){
      free(CACHE.sets[i].lines);
    }
    free(CACHE.sets);
}

int main(int argc, char *argv[])
{
 // -h -v -s <s> -E <E> -b <b> -t <tracefile>
 // - LRU (least-recently used) replacement policy
 // - dynamically allocate storage for simulators data structure using malloc
 // - ignore all instruction cache accesses (lines starting with I)
 // Valgrind always put I in the first column (with no preceding space) and
 // M, L, S, in the second column (with a preceding space)
 // - assume memory accesses are aligned properly, such that a single memory
 //   access never crosses block boundaries. By making this assumption, you can
 //   ignore the request sizes in the valgrind traces
 

    char* file;
    int opt;

    // parse
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1){
      switch(opt){
        case 'h':
          show_help();
          return 0;
          break;
        case 'v':
          verbose = 1;
          break;
        case 's':
          s = atoi(optarg);
          break;
        case 'E':
          E = atoi(optarg);
          break;
        case 'b':
          b = atoi(optarg);
          break;
        case 't':
          file = optarg;
          break;
        case '?':
          printf("The following parameters are required -s <num> -E <num> -b <num> -t <file>\n");
          return 0;
      }
    }
    if(s == -1 || E == -1 || b == -1){ // check if user didnt specify s, E, b
      printf("The following parameters are required -s <num> -E <num> -b <num> -t <file>\n");
      return 0;
    }
    //printf("%d %d %d %s %d\n", s, E, b, file, verbose);
    // get hits, misses, evicts
    cache_sim(file);
    
    printSummary(hits, misses, evictions);
    //printf("done");
    return 0;

}
