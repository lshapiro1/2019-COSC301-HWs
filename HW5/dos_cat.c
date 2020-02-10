#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "bootsect.h"
#include "bpb.h"
#include "direntry.h"
#include "fat.h"
#include "dos.h"


uint16_t get_dirent(struct direntry *dirent, char *buffer)
{
    uint16_t followclust = 0;
    memset(buffer, 0, MAXFILENAME);

    int i;
    char name[9];
    char extension[4];
    uint16_t file_cluster;
    name[8] = ' ';
    extension[3] = ' ';
    memcpy(name, &(dirent->deName[0]), 8);
    memcpy(extension, dirent->deExtension, 3);
    if (name[0] == SLOT_EMPTY)
    {
	    return followclust;
    }

    /* skip over deleted entries */
    if (((uint8_t)name[0]) == SLOT_DELETED)
    {
	    return followclust;
    }

    if (((uint8_t)name[0]) == 0x2E)
    {
        // dot entry ("." or "..")
        // skip it
        return followclust;
    }

    /* names are space padded - remove the spaces */
    for (i = 8; i > 0; i--) 
    {
        if (name[i] == ' ') 
            name[i] = '\0';
        else 
            break;
    }

    /* remove the spaces from extensions */
    for (i = 3; i > 0; i--) 
    {
        if (extension[i] == ' ') 
            extension[i] = '\0';
        else 
            break;
    }

    if ((dirent->deAttributes & ATTR_WIN95LFN) == ATTR_WIN95LFN)
    {
        // ignore any long file name extension entries
        //
        // printf("Win95 long-filename entry seq 0x%0x\n", dirent->deName[0]);
    }
    else if ((dirent->deAttributes & ATTR_DIRECTORY) != 0) 
    {
        // don't deal with hidden directories; MacOS makes these
        // for trash directories and such; just ignore them.
	    if ((dirent->deAttributes & ATTR_HIDDEN) != ATTR_HIDDEN)
        {
            strcpy(buffer, name);
            file_cluster = getushort(dirent->deStartCluster);
            followclust = file_cluster;
        }
    }
    else 
    {
        /*
         * a "regular" file entry
         * print attributes, size, starting cluster, etc.
         */
        strcpy(buffer, name);
        if (strlen(extension))  
        {
            strcat(buffer, ".");
            strcat(buffer, extension);
        }
    }

    return followclust;
}


struct direntry *follow_dir(char *searchpath, uint16_t cluster, 
		            uint8_t *image_buf, struct bpb33* bpb)
{
    char *next_path_component = index(searchpath, '/');
    int entry_len = strlen(searchpath);
    if (next_path_component != NULL)
    {
        entry_len = next_path_component - searchpath;
        *next_path_component = '\0';
        next_path_component++;
    }

    struct direntry *rv = NULL;

    while (is_valid_cluster(cluster, bpb))
    {
        struct direntry *dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);

        int numDirEntries = (bpb->bpbBytesPerSec * bpb->bpbSecPerClust) / sizeof(struct direntry);
        int i = 0;
	for ( ; i < numDirEntries; i++)
	{
            char buffer[MAXFILENAME]; 
            uint16_t followclust = get_dirent(dirent, buffer);

            if (strncasecmp(searchpath, buffer, strlen(searchpath)) == 0)
            {
                if (next_path_component)
                {
                    if (followclust)
                        rv = follow_dir(buffer, followclust, image_buf, bpb);
                }
                else
                {
                    rv = dirent; 
                }
            }

            if (rv)
                break;

            dirent++;
	}

	cluster = get_fat_entry(cluster, image_buf, bpb);
    }

    return rv;
}


struct direntry *traverse_root(char *searchpath, uint8_t *image_buf, struct bpb33* bpb)
{
    uint16_t cluster = 0;
    struct direntry *rv = NULL;

    struct direntry *dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);

    char *next_path_component = index(searchpath, '/');
    int root_entry_len = strlen(searchpath);
    if (next_path_component != NULL)
    {
        root_entry_len = next_path_component - searchpath;
        *next_path_component = '\0';
        next_path_component++;
    }

    char buffer[MAXFILENAME];

    int i = 0;
    for ( ; i < bpb->bpbRootDirEnts; i++)
    {
        uint16_t followclust = get_dirent(dirent, buffer);

        if (strncasecmp(searchpath, buffer, strlen(searchpath)) == 0)
        {
            if (!next_path_component)
                rv = dirent;
            else if (is_valid_cluster(followclust, bpb))
                rv = follow_dir(next_path_component, followclust, image_buf, bpb);
        }

        if (rv)
            break;

        dirent++;
    }

    return rv;
}


struct direntry *find_file(char *searchpath, uint8_t *image_buf, struct bpb33 *bpb)
{
    /* strip any leading '/' from search path */
    while (*searchpath == '/' && *searchpath != '\0') searchpath++;
    return traverse_root(searchpath, image_buf, bpb);
}


void do_cat(struct direntry *dirent, uint8_t *image_buf, struct bpb33 *bpb)
{
    uint16_t cluster = getushort(dirent->deStartCluster);
    uint32_t bytes_remaining = getulong(dirent->deFileSize);
    uint16_t cluster_size = bpb->bpbBytesPerSec * bpb->bpbSecPerClust;

    char buffer[MAXFILENAME];
    get_dirent(dirent, buffer);

    fprintf(stderr, "doing cat for %s, size %d\n", buffer, bytes_remaining);

    while (is_valid_cluster(cluster, bpb))
    {
        /* map the cluster number to the data location */
        uint8_t *p = cluster_to_addr(cluster, image_buf, bpb);

        uint32_t nbytes = bytes_remaining > cluster_size ? cluster_size : bytes_remaining;

        fwrite(p, 1, nbytes, stdout);
        bytes_remaining -= nbytes;
    
        cluster = get_fat_entry(cluster, image_buf, bpb);
    }
}


void usage(char *progname)
{
    fprintf(stderr, "usage: %s <imagename> <filename>\n", progname);
    exit(1);
}


int main(int argc, char** argv)
{
    uint8_t *image_buf;
    int fd;
    struct bpb33* bpb;
    if (argc != 3)
    {
	    usage(argv[0]);
    }

    image_buf = mmap_file(argv[1], &fd);
    bpb = check_bootsector(image_buf);

    struct direntry *dirent = find_file(argv[2], image_buf, bpb);
    if (dirent)
        do_cat(dirent, image_buf, bpb);

    unmmap_file(image_buf, &fd);

    return 0;
}
