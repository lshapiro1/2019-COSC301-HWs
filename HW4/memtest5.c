#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "forgetful.h"

/*
 * A re-definition of the block header here, since
 * we are not "inside" the forgetful library and
 * don't have access to that definition.
 */

struct block_header {
    uint64_t size;
    struct block_header *next;
};


static const int SMALLEST_SIZE = sizeof(struct block_header);
static struct block_header *MAGIC = (struct block_header *)0xf00df00d;  // the heap is hungry?


#define XASSERT(cond) { if (!(cond)) { printf("Test failed on line=%d in file=%s)\n", __LINE__, __FILE__); abort(); } }


/*
 * Given a pointer, return a pointer to the block header.
 * To do that, "rewind" sizeof(struct block_header) bytes
 * and return a pointer to that.
 * */
struct block_header *get_header(void *ptr) {
    return (struct block_header*)((uint8_t*)ptr - sizeof(struct block_header));
}

int main(int argc, char **argv) {
    struct block_header *hdr = NULL;

    void *p1 = xmalloc(8);
    hdr = get_header(p1);
    XASSERT(hdr->size == SMALLEST_SIZE);
    XASSERT(hdr->next == MAGIC);

    void *p2 = xmalloc(100);
    hdr = get_header(p2);
    XASSERT(hdr->size == 104);
    XASSERT(hdr->next == MAGIC);

    void *p3 = xmalloc(200);
    hdr = get_header(p3);
    XASSERT(hdr->size == 200);
    XASSERT(hdr->next == MAGIC);

    void *p4 = xmalloc(72);
    hdr = get_header(p4);
    XASSERT(hdr->size == 72);
    XASSERT(hdr->next == MAGIC);

    void *p5 = xmalloc(1024);
    hdr = get_header(p5);
    XASSERT(hdr->size == 1024);
    XASSERT(hdr->next == MAGIC);

    //
    // don't try this at home.  we are doing some "use after free" to verify that
    // the freelist gets properly coalesced.
    //
    xfree(p1);
    struct block_header *p1hdr;
    p1hdr = get_header(p1);
    XASSERT(p1hdr->size == 16);
    XASSERT(p1hdr->next == NULL);

    xfree(p3);
    struct block_header *p3hdr;
    p3hdr = get_header(p3); 
    XASSERT(p1hdr->size == 16);
    XASSERT(p1hdr->next == p3hdr);
    XASSERT(p3hdr->size == 200);
    XASSERT(p3hdr->next == NULL);

    xfree(p5);
    struct block_header *p5hdr;
    p5hdr = get_header(p5); 
    XASSERT(p5hdr->size == 1024);
    XASSERT(p5hdr->next == NULL);
    XASSERT(p3hdr->size == 200);
    XASSERT(p3hdr->next == p5hdr);

    void *p6 = xmalloc(202); // should cause a split of free block of 1040 bytes
    hdr = get_header(p6);
    XASSERT(hdr->size == 208);
    XASSERT(hdr->next == MAGIC);

    XASSERT(p1hdr->size == 16);
    XASSERT(p1hdr->next == p3hdr);
    XASSERT(p3hdr->size == 200);
    XASSERT(p3hdr->next != NULL);
    XASSERT(p3hdr->next->size == 800); // last block should have been split
    XASSERT(p3hdr->next->next == NULL);

    void *p7 = xmalloc(4); // should cause another split of last block 
    hdr = get_header(p7);
    XASSERT(hdr->size == 16);
    XASSERT(hdr->next == MAGIC);

    XASSERT(p3hdr->size == 200);
    XASSERT(p3hdr->next != NULL);
    XASSERT(p3hdr->next->size == 768); // last block should have been split
    XASSERT(p3hdr->next->next == NULL);

    xfree(p2);

    printf("Got to the end --- all tests in this file have passed!\n");
    printf("The heap size should be 1496 bytes and the freelist below should be:\n");
    printf("\t368 bytes free\n");
    printf("\t344 bytes allocated\n");
    printf("\t784 bytes free\n");
    return 0;
}
