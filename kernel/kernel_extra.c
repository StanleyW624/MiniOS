/*
 * kernel_extra.c - Project 2 Extra Credit, CMPSC 473
 * Copyright 2023 Ruslan Nikolaev <rnikola@psu.edu>
 * Distribution, modification, or usage without explicit author's permission
 * is not allowed.
 */

#include <malloc.h>
#include <types.h>
#include <string.h>
#include <printf.h>

// Your mm_init(), malloc(), free() code from mm.c here
// You can only use mem_sbrk(), mem_heap_lo(), mem_heap_hi() and
// Project 2's kernel headers provided in 'include' such
// as memset and memcpy.
// No other files from Project 1 are allowed!

#define ALIGNMENT 16

static void *find_fit(size_t);

// Block structure: header size, next node address, prev node address
typedef struct Block {
    size_t size_node;
    struct Block* next_node;
    struct Block* prev_node;
} mem;

// Free block list
struct Block *first;

static void addblock(mem *p, size_t size);

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}


bool mm_init()
{
    void* value = mem_sbrk(16+4096); 

    size_t* end = mem_heap_hi()+1;

    // Set freespace header block
    first = value;
    if (value == (void *)-1) {
        return false;
    }
    *((size_t*) value)= 1;
    mem *mod = value+ 8;
    mod -> size_node = 4096; // Set header size
    mod -> size_node ^= 2; // For dummy footer
    mod -> next_node = NULL; // Set next node
    mod -> prev_node = NULL; // Set prev node
    first = mod; // Set freespace header block to the first free block
    *(end-2)= 4096; // Set freespace footer
    *(end-1)=1; // Set dummy header to be allocated
    return true;

}

void *malloc(size_t size)
{
    size_t* new_heap_end;
    size_t n_size;
    
    if (size == 0) {
    	return NULL;
    }

    // Align with header
    n_size = align(size+8); 

    // Corner case: If aligned less than 32
    if (n_size < 32) {
        n_size = 32;
    }
    
    // Finds the block that "first fits" the size
    mem *pt = find_fit(n_size);
    if (pt!= NULL) {        
        size_t full_size = (pt -> size_node) & (~3); // Gets the full size of the block from find fit
        size_t cut_size = full_size - n_size; // Checks whether or not we need to split the block
        
        // Check for edge condition, if free space is same size or too small
        if ((cut_size <= 32) || (full_size == n_size)) {
            // Updates header (case 1: if prev block is allocated, case 2: if prev block is not allocated)
            if (((pt->size_node) & 2) != 0) {
                pt -> size_node = full_size | 1; // Case 1
                pt -> size_node ^= 2;
            } else {
                pt -> size_node = full_size | 1; // Case 2
            }

            // Updates next block second bit
            *((size_t*)pt+(size_t)(full_size/(sizeof(size_t)))) ^= 2; 

            // Deletes free block from free list
            if ((pt->prev_node) != NULL) {
                (pt -> prev_node) -> next_node = pt -> next_node;
            } else {
                first = pt -> next_node;
            } 

            if ((pt -> next_node)!= NULL) {
                (pt -> next_node) -> prev_node = pt -> prev_node;
            }

            return (size_t*)pt +1;
        }
        
        // Add block, and split if too large
        addblock(pt,n_size); 
        
        return (size_t*)pt + 1 + (size_t)((cut_size/(sizeof(size_t))));
    } else {
        // Extend if does not fit size
        void* newblk = mem_sbrk(n_size);
        if (newblk == (void *)-1) {
            return NULL;
        }

        new_heap_end = mem_heap_hi()+1;
        // Change the old dummy header to new header of the extended space
        mem* extend_blk = (mem*)((char*)newblk-8);

        // Updates header (case 1: if prev block is allocated, case 2: if prev block is not allocated)
        if (((extend_blk->size_node) & 2) != 0) {
            extend_blk -> size_node = n_size | 1; // Case 1
            extend_blk -> size_node ^= 2;
        } else {
            extend_blk -> size_node = n_size | 1; // Case 2
        }
 
        *(new_heap_end-1)=3; // Set new dummy header

        return newblk;
    }
    return NULL;
}

/*
 * addblock: Adds a block and also splits it
 */
static void addblock(mem *p, size_t size)
{
    size_t block_size = (p -> size_node) & (~3);
    size_t extra_size = block_size - size;

    // Updates header (case 1: if prev block is allocated, case 2: if prev block is not allocated)
    if (((p->size_node) & 2) != 0) {
        p -> size_node = extra_size;  // Case 1
        p -> size_node ^= 2;
    } else {
        p -> size_node = extra_size;  // Case 2
    }

    // Changes free block footer
    *((size_t*)p+(size_t)(extra_size/(sizeof(size_t)))-1) = extra_size;

    // Changes allocated block header and the next block's previous block indicator
    *((size_t*)p+(size_t)(extra_size/(sizeof(size_t)))) = size | 1;
    *((size_t*)p+(size_t)(block_size/(sizeof(size_t)))) ^= 2;

}

/*
 * find_fit: Finds a location that fits after passing a size
 */
static void *find_fit(size_t size) 
{
    // Loop begins at the start of free list
    mem *p = first;

    // Find valid free space
    while(p) {
        
        if (((p -> size_node) & (~3)) >= size) {
            return p;
        } 

        p = p -> next_node;
    }
    return NULL;
}

void free(void *ptr)
{
   if (ptr == NULL) {
      return;
    }
    
    // Get header block of ptr and set bit value as not allocated
    mem* freeing = (mem*) ((char*)(ptr)-8);
    freeing->size_node = (freeing->size_node) & -2;

    // Set free block footer
    *((size_t*)freeing + (size_t)(((freeing->size_node) & (~3))/sizeof(size_t))-1) = (freeing -> size_node) & (~3);

    // Update next block's current block information
    *((size_t*)freeing + (size_t)(((freeing->size_node) & (~3))/sizeof(size_t))) = *((size_t*)freeing + (size_t)(((freeing->size_node) & (~3))/sizeof(size_t))) & (~2);
    
    // Add block to front of free list
    freeing -> prev_node = NULL;
    freeing -> next_node = first;
    if (first != NULL) {
        first -> prev_node = freeing;
    }
    first = freeing;

}
