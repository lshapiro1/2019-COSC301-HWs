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

    xfree(p2);
    XASSERT(p1hdr->size == 352);
    XASSERT(p1hdr->next == NULL);

    printf("Got to the end --- all tests in this file have passed!\n");
    printf("The heap size should be 456 bytes and the freelist below should be:\n");
    printf("\t368 bytes free\n");
    printf("\t88 bytes allocated\n");
    return 0;
}
