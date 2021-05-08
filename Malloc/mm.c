/******************************************************************************
 * 
 * Malloc lab header
 * 
 * Author: Mark Gameng 
 * Email:  mgameng1@iit.edu
 * AID:    A20419026
 * Date:   05/03/21
 * 
 * By signing above, I pledge on my honor that I neither gave nor received any
 * unauthorized assistance on the code contained in this repository.
 * 
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "mm.h"
#include "memlib.h"

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define MAX(x, y) ((x) > (y)? (x) : (y))

// pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// read and write a word at address p
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/*
8/16
trace  valid  util     ops      secs  Kops
 0       yes   88%    5694  0.000532 10699
 1       yes   91%    5848  0.000443 13204
 2       yes   94%    6648  0.000680  9775
 3       yes   96%    5380  0.000525 10253
 4       yes   66%   14400  0.000779 18490
 5       yes   88%    4800  0.000806  5952
 6       yes   85%    4800  0.000905  5304
 7       yes   54%   12000  0.004482  2677
 8       yes   47%   24000  0.006866  3495
 9       yes   92%   14401  0.000598 24086
10       yes   62%   14401  0.000551 26150
Total          79%  112372  0.017167  6546

Perf index = 47 (util) + 26 (thru) = 73/100

16/32
trace  valid  util     ops      secs  Kops
 0       yes   88%    5694  0.000484 11764
 1       yes   90%    5848  0.000420 13911
 2       yes   93%    6648  0.000653 10176
 3       yes   95%    5380  0.000499 10788
 4       yes   66%   14400  0.000777 18542
 5       yes   88%    4800  0.000790  6076
 6       yes   85%    4800  0.000868  5532
 7       yes   51%   12000  0.004469  2685
 8       yes   64%   24000  0.001126 21318
 9       yes   93%   14401  0.000605 23784
10       yes   68%   14401  0.000530 27182
Total          80%  112372  0.011221 10015

Perf index = 48 (util) + 40 (thru) = 88/100
*/

#define WSIZE 16 // word and header/footer size bytes
#define DSIZE 32 // double word size bytes
#define CHUNKSIZE (1 << 12) // extend heap by this amount of bytes

// read the size and allocated fields from address p
#define GET_SIZE(p) (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p) (GET(p) & 0x1)

// given block ptr bp, compute address of its header and footer
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define NEXT_PTR(bp) (*(char **)(bp + WSIZE))
#define PREV_PTR(bp) (*(char **)(bp))

// funcs
int mm_check(void);
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void freeListInsert(void *bp);
static void freeListRemove(void *bp);

// globes
static char *heap_listp = 0;
static char *free_listp = 0;

/* ---------------------- */

int mm_init(void) // p883
{
  /*
 * Perform any necessary initializations -- allocating initial heap area
 * Retrun -1 if there was a problem in performing the initialization, 0 otherwise */
  
  // create initial empty heap
  if ((heap_listp = mem_sbrk(8 * WSIZE)) == (void *) - 1) // 8 = 84 ; 16, 32 = 83
    return -1;

  PUT(heap_listp, 0); // alignment padding
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // prologue header
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // prologue footer
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); // epilogue header
  //heap_listp += (2 * WSIZE);
  free_listp = heap_listp + 2 * WSIZE;

  // extend empty heap with a free block of chunksize bytes
  if (extend_heap(4) == NULL) // 4 = 84% util ; CHUNKSIZE / WSIZE = 80% ; CHUNKSIZE = 69% ; WSIZE = 81% ; DSIZE = 82%
    return -1;
  //mm_check();
  return 0;
}

void *mm_malloc(size_t size) // p886
{
  /*
 * Return point to an allocated block payload of at least size bytes
 * The block should lie within the heap region and not overlap with any other allocated chunk
 * retrun 8 byte aligned pointers */

  /*
  int blk_size = ALIGN(size + SIZE_T_SIZE);
  size_t *header = mem_sbrk(blk_size);
  if (header == (void *)-1) {
    return NULL;
  } else {
    *header = blk_size | 1;
    return (char *)header + SIZE_T_SIZE;
  }
  */
  
  size_t asize; // adjusted block size
  size_t extendsize; // amount to extend heap if no fit
  void *bp;
  
  // ignore spurious requests
  if (size == 0)
    return NULL;
  
  // adjust block size to include overhead and alignment reqs
  if (size <= DSIZE)
    asize = 2 * DSIZE;
  else
    asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  
  // search the free list ofr a fit
  if ((bp = find_fit(asize)) != NULL){
    place(bp, asize);
    return bp;
  }

  // no fit found, get more memory and place the block
  extendsize = MAX(asize, CHUNKSIZE); // CHUNKSIZE = 10k ; WSIZE = 2k; DSIZE = 2k ; CHUNKSIZE * 2 = 12k but lower util
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL) // extendsize / WSIZE = 84 ; extendsize = 62
    return NULL;
  place(bp, asize);
  //mm_check();
  return bp;
}

void mm_free(void *ptr) // p885
{
  /*
 * Frees the block pointed to by ptr
 * only guaranteed to work when ptr was returned by an earlier call to mm_malloc or mm_realloc and has not yet been freed */

  /*
  // given
  size_t *header = ptr - SIZE_T_SIZE;
  *header &= ~1L;
  */
  //mm_check();
  
  // do i need to check if ptr is valid?
  /*Not really used
  if(ptr == NULL){
    printf("here");
    return;
  }
  if(ptr == 0 || heap_listp == 0){
    //printf("here");
    return;
  }*/
  size_t size = GET_SIZE(HDRP(ptr));
  
  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);
  //mm_check();
}

void *mm_realloc(void *ptr, size_t size)
{
  /*
 * returns a pointer to an allocated region of at least size bytes
 * if ptr null, call is equivalent to mm_malloc(size)
 * if size is 0, call is equivalent to mm_free(ptr)
 * if ptr is not null, it must have been returned by an earlier call to mm_malloc or realloc
 *  call to realloc changes the size of the memory block pointed to by ptr to size bytes and returns new address of the new
 *  block. Contents of the new block are the same as the hold ptr block, up to the minimum of the old and new sizes.
 *  Everything else is uninitialized. */

  /*
  // prof
  size_t *header = ptr - SIZE_T_SIZE;

  if((char *)header + (*header & ~1L) > (char *)mem_heap_hi()){
    // this block is at end of heap
    size_t blk_size = ALIGN(size + SIZE_T_SIZE);
    mem_sbrk(blk_size - (*header & ~1L));
    *header = blk_size | 1;
    return ptr
  }else{
    // given
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
  */ 
  
  // not used
  if(size == 0){
    //printf("1");
    mm_free(ptr);
    return NULL;
  }else if(size < 0){
    //printf("2");
    return NULL;
  }
  
  // not even used
  if(ptr == NULL){
    //printf("nice");
    mm_malloc(size);
    return NULL;
  }
  
  size_t oldSize = GET_SIZE(HDRP(ptr));
  size_t newSize = size + 2 * WSIZE;

  if(newSize <= oldSize)
    return ptr;
  
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
  size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
  size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
  size_t prevSize = GET_SIZE(HDRP(PREV_BLKP(ptr)));  
   
  if(!next_alloc && (oldSize + nextSize >= newSize)){ // next block unallocated -- combine
    freeListRemove(NEXT_BLKP(ptr));
    PUT(HDRP(ptr), PACK(oldSize + nextSize, 1));
    PUT(FTRP(ptr), PACK(oldSize + nextSize, 1));
    /*freeListRemove(NEXT_BLKP(ptr));
    PUT(HDRP(ptr), PACK(newSize, 1));
    PUT(FTRP(ptr), PACK(newSize, 1));
    void *bp = NEXT_BLKP(ptr);
    PUT(HDRP(bp), PACK(oldSize + nextSize - newSize, 1));
    PUT(FTRP(bp), PACK(oldSize + nextSize - newSize, 1));
    mm_free(bp);*/
    return ptr;
  }else if(!prev_alloc && (oldSize + prevSize >= newSize)){ // prev block unallocated 
    //-- used alot results in 93/68 util ; 24k/27k kops vs without of 97/40 ; 24k/25k
    //printf("prev \n");
    void *new_ptr = PREV_BLKP(ptr);
    freeListRemove(new_ptr);
    PUT(HDRP(new_ptr), PACK(oldSize + prevSize, 1));
    memcpy(new_ptr, ptr, GET_SIZE(HDRP(ptr)));
    PUT(FTRP(new_ptr), PACK(oldSize + prevSize, 1));
    //mm_check();
    return new_ptr;
  }
  /*else if(!prev_alloc && !next_alloc && (prevSize + oldSize + nextSize >= newSize)){ // next and prev unallocated -- not used in traces
    //printf("prev + next \n");
    freeListRemove(PREV_BLKP(ptr));
    freeListRemove(NEXT_BLKP(ptr));
    void *tbp = PREV_BLKP(ptr);
    PUT(HDRP(tbp), PACK(oldSize + prevSize + nextSize, 1));
    PUT(FTRP(tbp), PACK(oldSize + prevSize + nextSize, 1));
    return tbp;
  }*/

  void *new_ptr = mm_malloc(newSize);
  place(new_ptr, newSize);
  memcpy(new_ptr, ptr, newSize);
  mm_free(ptr);

  //mm_check();
  return new_ptr;
}

static void freeListInsert(void *bp){
  NEXT_PTR(bp) = free_listp;
  PREV_PTR(free_listp) = bp;
  PREV_PTR(bp) = NULL;
  free_listp = bp;
}

static void freeListRemove(void *bp){
  void* prev = PREV_PTR(bp);
  void* next = NEXT_PTR(bp);
  if(prev){
    NEXT_PTR(prev) = next;
  }else{
    free_listp = next;
  }
  PREV_PTR(next) = prev;
}

static void *extend_heap(size_t words){ // p883
  char *bp;
  size_t size;
 
  // allocate an even number of words to maintain alighment
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

  if ((long)(bp = mem_sbrk(size)) == -1)
    return NULL;
  
  // initialize free block header/footer and the epilogue header
  PUT(HDRP(bp), PACK(size, 0)); // free block header
  PUT(FTRP(bp), PACK(size, 0)); // free block footer
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epilogue header

  // coalesce if the previous block was free
  //mm_check();
  return coalesce(bp);
}

static void *coalesce(void *bp){ // p885

  size_t prev_alloc;
  if(GET_ALLOC(FTRP(PREV_BLKP(bp)))){
    prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  }else{
    prev_alloc = PREV_BLKP(bp) == bp;
  }
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if(prev_alloc && next_alloc){ // case 1
    /*freeListInsert(bp);
    return bp;*/
  } else if(prev_alloc && !next_alloc){ // case 2
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    freeListRemove(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  } else if(!prev_alloc && next_alloc){ // case 3
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    /*
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
    */
    bp = PREV_BLKP(bp);
    freeListRemove(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }else{ // case 4
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    /*
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
    */
    freeListRemove(PREV_BLKP(bp));
    freeListRemove(NEXT_BLKP(bp));
    bp = PREV_BLKP(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }
  freeListInsert(bp);
  //mm_check();
  return bp;
}

static void *find_fit(size_t asize){ // p909
  // first fit search
  void *bp;

  // optimize this 
  for(bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_PTR(bp)){
    if(asize <= (size_t)GET_SIZE(HDRP(bp))){
      return bp;
    }
  }
   
  return NULL; // no fit
}

static void place(void *bp, size_t asize){ // p909
  size_t csize = GET_SIZE(HDRP(bp));

  if((csize - asize) >= (2 * DSIZE)){ // 2 = 84/10k; 4 = 82/8k;
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    freeListRemove(bp);
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, 0));
    PUT(FTRP(bp), PACK(csize - asize, 0));
    coalesce(bp);
  }else{
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
    freeListRemove(bp);
  }
  //mm_check();
}

int mm_check(void){
  /*
 * Is every block in the free list marked as free
 * Are there any contiguous free blocks that smoehow escaped coalescing
 * Is every free block actually in the free list
 * Do the pointers in the free list point to valid free blocks
 * Do any allocated blocks overlap
 * Do the poniters in a heap block point to a valid heap address */
  
  //printf("Heap: %p \n", heap_listp);
  //printf("Free: %p \n", free_listp);
  
  void *bp;
  for(bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_PTR(bp)){
    if(GET_ALLOC(HDRP(bp))){
      printf("free list aint so free");
      return -1;
    }
    if(bp < mem_heap_lo() || bp > mem_heap_hi()){
      printf("Block outside of heap");
      return -1;
    }
  }
        
  //printf("Better not be checking rn \n");
  //printf("BotHeap: %p ", mem_heap_hi());
  //printf("TopHeap: %p ", mem_heap_lo()); 
  //printf("HeapSize: %lu \n", mem_heapsize());
  
  return 0;
}
