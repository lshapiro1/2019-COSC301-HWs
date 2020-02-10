#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "forgetful.h"

/*
 *
 * Each memory block has a 16 B header as follows:
 *
 * size (8 bytes)
 * magic (8 bytes) (if allocated) OR next (8 bytes) (if free)
 * ... user data follows ...
 *
 * this structure follows the OSTEP discussion in chapter 17 exactly
 * http://pages.cs.wisc.edu/~remzi/OSTEP/vm-freespace.pdf
 */

struct block_header {
    uint64_t size;
    struct block_header *next;
};


static const int MINIMUM_ALLOC = sizeof(struct block_header) * 2; 
static struct block_header *MAGIC = (struct block_header*)0xf00df00d;  // the heap is hungry?

#define XASSERT(cond) { if (!(cond)) { fprintf(stderr, "forgetful library check failed: line=%d\n", __LINE__); abort(); } }

/*
 * Given a pointer, return a pointer to the block header.
 * To do that, "rewind" sizeof(struct block_header) bytes
 * and return a pointer to that.
 * */
static struct block_header *get_header(void *ptr) {
    return (struct block_header*)((uint8_t*)ptr - sizeof(struct block_header));
}


/*
 * Here are the global references to free_list, beginning of the heap,
 * and end of the heap.
 */
static struct block_header *free_list = NULL;
static void *heap_begin = NULL;
static void *heap_end = NULL;


/*
 * Compute the number of bytes gap between two pointers
 */
size_t compute_offset(void *first, void *second) {
    return (uint8_t*)second - (uint8_t*)first;
}


/*
 * Update the free_list by removing block from it.  This is basically
 * a linked-list remove.
 *
 * As a side-effect, set next in the block to MAGIC.
 * Don't return anything.
 */
void remove_from_free_list(struct block_header *block) {
    // special case: block is at the head of the free list
    if (free_list == block) {
        free_list = block->next;
    } else {
        struct block_header *tmp = free_list;
        while (tmp->next != NULL && tmp->next != block) {
            tmp = tmp->next;
        }
        // invariant: tmp->next == block
        XASSERT(tmp->next == block);
        tmp->next = block->next; 
    }

    block->next = MAGIC;
}


/*
 * Search the free list to find a block large enough to satisfy
 * a request.  
 *
 * NOW implements *worst fit*.
 *
 * */
static struct block_header *find_free_block(size_t request_size) {
    if (free_list == NULL) {
        return NULL; 
    }
    // this function NOW implements *worst* fit.
    struct block_header *tmp = free_list;
    struct block_header *largest = NULL;
    while (tmp != NULL) {
        if (tmp->size >= request_size) {
            if (largest == NULL || tmp->size > largest->size){
                largest = tmp;
            }
        }
        tmp = tmp->next;
    }
    return largest;
}


/*
 * Allocate a new block.  
 * 1) get a free block from the free_list
 * 2) decide whether we need to split it; do so if needed
 * 3) update the free_list
 * 4) return a pointer to the *data* bytes (not the block header)
 */
static void *allocate_block(size_t request_size) {
    // 1) get a free block.  return NULL immediately if this fails.
    struct block_header *block = find_free_block(request_size);
    if (block == NULL) {
        return NULL;
    }
    
    // 2) decide whether to split the block.  we split if the 
    // left-over bytes (including another struct block_header!) 
    // are >= the MINIMUM_ALLOC.
    size_t remain_bytes = block->size - request_size;
    if (remain_bytes >= MINIMUM_ALLOC) {
        // Create new block to add to free list
        struct block_header * const toPlace = (struct block_header*)((uint8_t*)block+request_size);
        toPlace->size = remain_bytes - 16;
        toPlace->next = block->next;
        block->next = (struct block_header *)(block+request_size);
    }

    // 3) update the free_list
    remove_from_free_list(block);
    XASSERT(block->next == MAGIC);


    // 4) return a pointer to the data bytes
    uint8_t *p = (uint8_t*)block;
    return (p + sizeof(struct block_header));
}


/*
 * Compute the size that we will allocate, given a
 * user's request size.  
 * 
 * Constraints: (1) allocate at least sizeof(struct block_header),
 * (2) allocate a multiple of sizeof(void*) (8 bytes on 64 bit arch).
 *
 */
static size_t compute_request_size(size_t request_size) {
    // constraint (1)
    request_size = request_size < sizeof(struct block_header) ? sizeof(struct block_header) : request_size;

    // constraint (2)
    size_t leftover = request_size % sizeof(void *);
    if (leftover > 0) {
        request_size += (sizeof(void*) - leftover);
    }

    // verify constraints
    XASSERT((request_size >= sizeof(struct block_header) && request_size % sizeof(void*) == 0));

    return request_size;
}


/*
 * 
 * Extend the size of the heap to accommodate a new request.
 * 1) call sbrk to extend the heap; update heap_begin and heap_end as necessary
 * 2) for the newly allocated block, put it on the end of the free_list.
 *
 */
static void extend_heap(size_t amount) {
    // 1) extend the heap
    size_t amount_plus_header = amount + sizeof(struct block_header);

    void *ptr = sbrk(amount_plus_header);
    if (!heap_begin) {
        heap_begin = ptr;
        atexit(dump_memory_map);
    }
    heap_end = sbrk(0);

    // 2) add the newly allocated block to the end of the free_list
    struct block_header *hdr = (struct block_header *)ptr;
    hdr->size = amount;
    hdr->next = NULL;
    
    if (free_list == NULL) {
        free_list = hdr;
    } else {
        struct block_header *tmp = free_list;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = hdr;
    }
}


/*
 * Our malloc implementation, named xmalloc so it doesn't clash with built-in malloc.
 *
 * Returns a pointer to a newly allocated heap block, or NULL if no more memory.
 */
void *xmalloc(size_t request_size) {
    void *rv = NULL;

    request_size = compute_request_size(request_size);
    rv = allocate_block(request_size);

    // if no block, then extend the heap and try again.  NB: if this fails *again*
    // then we just return NULL (which is all we can do, sadly).
    if (rv == NULL) {
        extend_heap(request_size);
        rv = allocate_block(request_size);
    }
    return rv;
}

/* 
helper function to coalesce if necessary 
used in Xfree  
*/
void coalesce(){
    struct block_header *tmp = free_list;
    while (tmp!= NULL && tmp->next != NULL){
        while (compute_offset(tmp, get_header(tmp->next)) == tmp->size ){
            tmp->size += tmp->next->size + sizeof(struct block_header);
            tmp->next = tmp->next->next;
            if (tmp->next == NULL){
                break;
            }
        }
        tmp = tmp->next;
    }
    //dump_memory_map();
}

/* 
 * Our free implementation.
 * Links back into free list, then does any coalescence.
 */
void xfree(void *vptr) {
    //printf("BEGINING TO FREE\n");
    struct block_header *hdr = get_header(vptr);
    XASSERT(hdr->next == MAGIC);
    // 1) put the memory block back on to the freelist
    struct block_header *tmp = free_list;
    if (tmp == NULL){
        free_list = hdr;
        free_list->next = NULL;
        return;
    }
    else if (tmp > hdr){
        free_list = hdr;
        hdr->next = tmp;
    }
    else{
        while (tmp->next != NULL){
            if (tmp->next > hdr){ 
                break;
            }
            else{
                tmp = tmp->next;
            }
        }
        hdr->next = tmp->next;
        tmp->next = hdr;
    }
    
    //dump_memory_map();
    // 2) do any block coalescence necessary
    coalesce();
}



/*
 * Dump out a representation of the heap, including
 * free and allocated regions.
 *
 * This gets automatically called when a program exits.
 */
void dump_memory_map() {
    printf("\nHeap begin: %p\n", heap_begin);
    printf("Heap size: %ld\n", heap_end - heap_begin);
    printf(" ** Dumping free list ** \n"); 

    size_t alloc_size = 0;

    if (heap_begin < (void*)free_list) {
        alloc_size = compute_offset(heap_begin, free_list);
        printf("Block %p %ld bytes allocated\n", heap_begin, alloc_size);
    }

    if (free_list == NULL) {
        printf("\t(empty)\n");
    }

    struct block_header *curr = free_list;
    struct block_header *prev = NULL;
    while (curr != NULL) {
        XASSERT(curr->next != MAGIC); // shouldn't be MAGIC if on the free list

        if (prev) {
            alloc_size = compute_offset(prev, curr);
            // is there a hole in addition to free block?
            uint8_t *after_prev = (uint8_t*)prev + prev->size + sizeof(struct block_header);
            alloc_size = compute_offset(after_prev, curr);
            if (alloc_size > 0) {
                printf("Block %p %ld bytes allocated\n", after_prev, alloc_size);
            }
        }
        printf("Block %p %ld bytes free\n", curr, curr->size + sizeof(struct block_header));
        prev = curr;
        curr = curr->next;
    }

    if (prev) {
        uint8_t *after_prev = (uint8_t*)prev + prev->size + sizeof(struct block_header);
        alloc_size = compute_offset(after_prev, heap_end);
        if (alloc_size > 0) {
            printf("Block %p %ld bytes allocated\n", after_prev, alloc_size);
        }
    }
}

