#ifndef __DOS_H__
#define __DOS_H__

#define MAXPATHLEN 255
#define MAXFILENAME 13

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

/* prototypes for functions in dos.c */

#include <stdint.h>

uint8_t *mmap_file(char *, int *);
void unmmap_file(uint8_t *, int *);

struct bpb33* check_bootsector(uint8_t *);

uint16_t get_fat_entry(uint16_t, uint8_t *, struct bpb33 *);

void set_fat_entry(uint16_t, uint16_t, uint8_t *, struct bpb33 *);

int is_end_of_file(uint16_t);
int is_valid_cluster(uint16_t, struct bpb33 *);

uint8_t *root_dir_addr(uint8_t *, struct bpb33 *);

uint8_t *cluster_to_addr(uint16_t, uint8_t *, struct bpb33 *);

#endif // __DOS_H__
