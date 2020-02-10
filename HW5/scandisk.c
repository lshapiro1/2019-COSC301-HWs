#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h> 


#include "bootsect.h"
#include "bpb.h"
#include "direntry.h"
#include "fat.h"
#include "dos.h"

void usage(char *progname) {
    fprintf(stderr, "usage: %s <imagename>\n", progname);
    exit(1);
}

//write the values into a directory entry
// straight from dos_cp
void erase_dirent(struct direntry *dirent){
    memset(dirent, 0, sizeof(struct direntry));
    memset(dirent->deName, SLOT_DELETED, 8);

}
//write the values into a directory entry
// straight from dos_cp
void write_dirent(struct direntry *dirent, char *filename, uint16_t start_cluster, uint32_t size){
    char *p, *p2;
    char *uppername;
    int len, i;
    memset(dirent, 0, sizeof(struct direntry));
    uppername = strdup(filename);
    p2 = uppername;
    for (i = 0; i < strlen(filename); i++) {
        if (p2[i] == '/' || p2[i] == '\\') {
            uppername = p2+i+1;
        }
    }
    for (i = 0; i < strlen(uppername); i++) {
	    uppername[i] = toupper(uppername[i]);
    }
    memset(dirent->deName, ' ', 8);
    p = strchr(uppername, '.');
    memcpy(dirent->deExtension, "___", 3);
    if (p == NULL) {
	    fprintf(stderr, "No filename extension given - defaulting to .___\n");
    }
    else {
        *p = '\0';
        p++;
        len = strlen(p);
        if (len > 3) len = 3;{
            memcpy(dirent->deExtension, p, len);
        }
    }
    if (strlen(uppername)>8) {
        uppername[8]='\0';
    }
    memcpy(dirent->deName, uppername, strlen(uppername));
    free(p2);
    dirent->deAttributes = ATTR_NORMAL;
    putushort(dirent->deStartCluster, start_cluster);
    putulong(dirent->deFileSize, size);
}
// creates directory entry
void create_dirent(char *filename, uint16_t start_cluster, uint32_t size,uint8_t *image_buf, struct bpb33* bpb){
    struct direntry *dirent = (struct direntry *) root_dir_addr(image_buf,bpb);
    while (1) {
        if (dirent->deName[0] == SLOT_EMPTY) {
            write_dirent(dirent, filename, start_cluster, size);
            dirent++;
            memset((uint8_t*)dirent, 0, sizeof(struct direntry));
            dirent->deName[0] = SLOT_EMPTY;
            return;
        }
        if (dirent->deName[0] == SLOT_DELETED){
            /* we found a deleted entry - we can just overwrite it */
            write_dirent(dirent, filename, start_cluster, size);
            return;
        }
        dirent++;
    }
}
struct cluster_refs{ 
    // will be used to keep track of chains in the FAT and the slots that are not used
    // if an entry is used in the directory slots but not fat slots, indicates start of FAT chain
    int *fat_unused_slots; //will keep track of slots allocated in FAT
    int *direntry_unused_slots; // vs the ones that we access as we go through directory so we can find ones that are not used by directory entries
};

void clean_bad_chain(struct direntry *dirent,uint8_t *image_buf, struct bpb33* bpb,struct cluster_refs* ref_count){
    // will free all clusters in bad chain, delete directory entry that refrences chain
    uint16_t file_cluster, next_cluster;
    file_cluster = getushort(dirent->deStartCluster);
    while (!is_end_of_file(file_cluster)){
        next_cluster = get_fat_entry(file_cluster,image_buf,bpb);
        //printf("At cluster: %4i  Next cluster: %4i",file_cluster,next_cluster);
        set_fat_entry(file_cluster, (FAT12_MASK&CLUST_FREE), image_buf,bpb);
        if (next_cluster ==  (FAT12_MASK&CLUST_BAD)){
            //printf("\n--Found bad cluster at:  %i \n",file_cluster);
            set_fat_entry(file_cluster, (FAT12_MASK&CLUST_FREE), image_buf,bpb);
            if (get_fat_entry(file_cluster+1,image_buf,bpb) != (FAT12_MASK&CLUST_FREE) && ref_count->direntry_unused_slots[next_cluster+1] == 0){
                next_cluster = file_cluster + 1;
                //printf("---Chain continues at cluster: %i\n",next_cluster);
            }
            else{
                //printf("---Must search for remainder of chain\n");
            }
        }
        ref_count->direntry_unused_slots[file_cluster] = 0;
        ref_count->fat_unused_slots[file_cluster] = 0;
        file_cluster = next_cluster;
    }
    set_fat_entry(file_cluster, (FAT12_MASK&CLUST_FREE), image_buf,bpb);
    ref_count->direntry_unused_slots[file_cluster] = 0;
    ref_count->fat_unused_slots[file_cluster] = 0;
    memset((uint8_t*)dirent, 0, sizeof(struct direntry));
    memset(dirent->deName, SLOT_DELETED, 8);
    return;
}

void clean_up_fat(uint8_t *image_buf, struct bpb33* bpb,struct cluster_refs* ref_count){
    //will go through FAT and make that no clusters are referenced more than once, clears any that reference a block already
    // referenced and also not listed in directories
    int cluster_count = bpb->bpbSectors+2;
    int refrenced_cluster;
    for (int i = 2; i < cluster_count; i++){ // data clusters start at 
        refrenced_cluster = get_fat_entry(i,image_buf,bpb);
        if (refrenced_cluster != (FAT12_MASK&CLUST_FREE) && is_valid_cluster(refrenced_cluster,bpb)){
            if (ref_count->fat_unused_slots[refrenced_cluster] > 1){
                if (ref_count->direntry_unused_slots[i] == 0){
                    set_fat_entry(i,CLUST_FREE,image_buf,bpb);
                    ref_count->fat_unused_slots[refrenced_cluster]--;
                    ref_count->fat_unused_slots[i] = 0;
                }
            }
        }
    }
}

uint16_t check_dirent(struct direntry *dirent,uint8_t *image_buf, struct bpb33* bpb,struct cluster_refs* ref_count){
    //1. Size in directory entry matches file size in FAT
    //int freed_count = 0; FOR TESTING COMMENT OUT LATER
    uint16_t followclust = 0;
    char name[9];
    char extension[4];
    uint32_t dir_size;
    uint32_t fat_size = 0;
    uint16_t file_cluster;
    name[8] = ' ';
    extension[3] = ' ';
    memcpy(name, &(dirent->deName[0]), 8);
    memcpy(extension, dirent->deExtension, 3);
    //skip slots that are empty, deleted, parent directory, itself or a volume label 
    if (name[0] == SLOT_EMPTY || ((uint8_t)name[0]) == SLOT_DELETED ||((uint8_t)name[0]) == 0x2E ||(dirent->deAttributes & ATTR_VOLUME) != 0){ 
	    return followclust;
    }
    //check if slot is a directory
    else if ((dirent->deAttributes & ATTR_DIRECTORY) != 0) {
        file_cluster = getushort(dirent->deStartCluster);
        //make sure that not a hidden directory, we will not deal with those
	    if ((dirent->deAttributes & ATTR_HIDDEN) != ATTR_HIDDEN){
            followclust = file_cluster;
        }
        ref_count->direntry_unused_slots[file_cluster] = -7; //special value to indicate directory 
    }
    //found a "regular" file entry, 
    else {
        uint16_t prev_cluster;
        file_cluster = getushort(dirent->deStartCluster);
        dir_size = getulong(dirent->deFileSize); //get size listed in directory IN BYTES
        //compare w/ the FAT cluster chain using provided functions
        //iterate from starting cluster until we hit the end of file, then compare
        while (!is_end_of_file(file_cluster)){
            if (file_cluster == (FAT12_MASK&CLUST_BAD)){
                printf("-Deleting file: %s --> Bad cluster found in cluster chain\n",name);
                clean_bad_chain(dirent,image_buf,bpb,ref_count);
                return followclust;
            }
            ref_count->direntry_unused_slots[file_cluster]++;
            fat_size = fat_size + 512;
            prev_cluster = file_cluster;
            file_cluster = get_fat_entry(file_cluster,image_buf,bpb);
        }
        ref_count->direntry_unused_slots[file_cluster]++;
        //have gotten size of file in bytes
        int offset = fat_size - dir_size;
        //Now must modify FAT in 2 cases:
            // 1: If clusters exist beyond end of file, free those clusters (set_fat_entry() for each)
            // 2: If free or bad cluster in FAT chain, must adjust size in file (modify direntry)
        if (offset > 512){ //cluster chain longer than file
            file_cluster = getushort(dirent->deStartCluster); 
            int file_cluster_count;
            if (dir_size % 512 != 0){file_cluster_count = (dir_size / 512)+1;} //account for integer divison
            else{file_cluster_count = dir_size / 512;}
            int fat_cluster_count = fat_size / 512;
            printf("-Size issue found in file: %s\n--File size in DIRENTRY:  %6i Bytes\n--File size in FAT:       %6i Bytes  \n---Current FAT Chain Length: %i clusters\n---Needed  FAT Chain Length: %i clusters\n", name,dir_size,fat_size,fat_cluster_count,file_cluster_count);
            for (int i = 1; i <= fat_cluster_count; i++){
                if (i == file_cluster_count){
                    set_fat_entry(file_cluster,FAT12_MASK&CLUST_EOFE,image_buf,bpb);
                    //printf("SET CLUSTER: %i to EOFE\n",i);
                }
                else if(i > file_cluster_count){
                    set_fat_entry(file_cluster,FAT12_MASK&CLUST_FREE,image_buf,bpb);
                    //printf("SET CLUSTER: %i to FREE\n",i);
                }
                file_cluster = get_fat_entry(file_cluster,image_buf,bpb);
            }
            //printf("In File: %s Clusters Freed:%i\n",name,freed_count);
        }
        else if (offset < 0){ //file is longer than current chain
            int file_cluster_count;
            if (dir_size % 512 != 0){file_cluster_count = (dir_size / 512)+1;} //account for integer divison
            else{file_cluster_count = dir_size / 512;}
            int fat_cluster_count = fat_size / 512;
            printf("-Size issue found in file: %s\n--File size in DIRENTRY:  %6i Bytes\n--File size in FAT:       %6i Bytes  \n---Current FAT Chain Length: %i clusters\n---Needed  FAT Chain Length: %i clusters\n", name,dir_size,fat_size,fat_cluster_count,file_cluster_count);
            //rewrite size
            //all other clean up handled by fix_orphans
            putulong(dirent->deFileSize, fat_size);
        }
    }
    return followclust;
}

void fix_orphans(uint8_t *image_buf, struct bpb33* bpb,struct cluster_refs* ref_count){
    // will use the struct cluster_refs to see if any blocks are not apart of cluster chains but
    // are not listed in free within the FAT
    int found = FALSE;
    int print_count = 0;
    for (int i = 2; i < bpb->bpbSectors+2; i++){
        if ((ref_count->direntry_unused_slots[i] == 0) && (get_fat_entry(i,image_buf,bpb)!=(FAT12_MASK&CLUST_FREE))){
            if (!found){
                found = TRUE;
                printf("Orphan(s) found at Cluster Number(s):\n");
            }
            printf("-%4i", i);
            print_count++;
            if (print_count % 4 == 0){
                printf("\n");
            }
            else{
                printf("     ");
            }
            // get file size -> assume max use of cluster
            uint32_t file_size;
            int file_cluster = i;
            file_size = 512;
            ref_count->direntry_unused_slots[file_cluster]++;
            file_cluster = get_fat_entry(file_cluster,image_buf,bpb);
            while (!is_end_of_file(file_cluster)){
                ref_count->direntry_unused_slots[file_cluster]++;
                file_size = file_size + 512;
                file_cluster = get_fat_entry(file_cluster,image_buf,bpb);
            }
            // Now add orphan to root directory
            // create new name
            char name[9] = "found";
            char num[4];
            char ext[4] = ".dat";
            sprintf(num,"%d",print_count);
            strcat(name,num);
            strcat(name,ext);
            //add new entry intoo address at open slot in root directory
            create_dirent(name,i,file_size,image_buf,bpb);
        }
    }
    if (print_count == 0){
        printf("--No Orphans found!\n\n");
    }else if(print_count % 4 != 0){
                printf("\n");
            }
}


void follow_dir(uint16_t cluster,uint8_t *image_buf, struct bpb33* bpb,struct cluster_refs* ref_count){
    while (is_valid_cluster(cluster, bpb)){
        struct direntry *dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);
        int numDirEntries = (bpb->bpbBytesPerSec * bpb->bpbSecPerClust) / sizeof(struct direntry);
        for (int i = 0; i < numDirEntries; i++){
            uint16_t followclust = check_dirent(dirent,image_buf,bpb,ref_count); 
            if (followclust)
                follow_dir(followclust,image_buf, bpb,ref_count);
            dirent++;
	    }
	    cluster = get_fat_entry(cluster, image_buf, bpb);
        /* get_fat_entry returns the value from the FAT entry for
           clusternum. */
    }
}



void traverse_root(uint8_t *image_buf, struct bpb33* bpb, struct cluster_refs* ref_count){   
    //printf("ENTERED TRANVERSE_ROOT\n");
    uint16_t cluster = 0; // cluster number

    struct direntry *dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);
    /* cluster_to_addr returns the memory location where the memory mapped
       cluster actually starts */
    // direntry is a DOS directory entry
    
    for (int i = 0; i < bpb->bpbRootDirEnts; i++)
    {
        uint16_t followclust = check_dirent(dirent,image_buf,bpb,ref_count);
        if (is_valid_cluster(followclust, bpb))
            follow_dir(followclust, image_buf, bpb, ref_count);
        dirent++;
    }
}

int * fill_fat_ref_count(uint8_t *image_buf, struct bpb33* bpb){
    //iterates through all clusters in FAT, checks if they refrence any other clusters
    // when we compare to the clusters accessed via the directory, we can find which clusters are used but not accessable.
    int cluster_count = bpb->bpbSectors+2;
    int refrenced_cluster;
    int *cluster_refrence_counts = calloc(cluster_count, sizeof(int)); //create array to hold referecence counts of clusters, initalize w/ all 0's
    for (int i = 2; i < cluster_count; i++){ // data clusters start at 
        refrenced_cluster = get_fat_entry(i,image_buf,bpb);
        if (refrenced_cluster == i){
            set_fat_entry(i,(FAT12_MASK&CLUST_EOFE),image_buf,bpb);
        }else if (refrenced_cluster != (FAT12_MASK&CLUST_FREE)){
            if (cluster_refrence_counts[refrenced_cluster] == -7){
                cluster_refrence_counts[refrenced_cluster] = 0;
            }
            cluster_refrence_counts[refrenced_cluster]++;
            if (cluster_refrence_counts[i] == 0){
                cluster_refrence_counts[i] = -7;
                 // if nothing points to parent but parent points to valid cluster, it must be head of chain or after bad cluster 
                 // --> set special refrence (-7)
            }
        }
    }
    return cluster_refrence_counts;
}

//FOLLOWING METHODS ARE FOR TEST PURPOSES ONLY
// COMMENT OUT CALLS IN MAIN BEFORE FINAL PRINT
//method to test print the filled ref_count
void test_print_refs(struct cluster_refs* ref_counts,int cluster_count){ 
    int print_count = 0;
    printf("\nPRINTING REF COUNTS:\n");
    for (int i = 2; i < cluster_count; i++){
        if (ref_counts->direntry_unused_slots[i] != ref_counts->fat_unused_slots[i]){
            printf("Cluster %4i: DERC:%4i  FATRC:%4i",i,ref_counts->direntry_unused_slots[i],ref_counts->fat_unused_slots[i]);
            print_count++;
            if (print_count % 2 == 0){
                printf("\n");
            }
            else{
                printf("     ");
            }
        }
    }
    printf("\n");
}

void test_print_FAT(uint8_t *image_buf, struct bpb33* bpb){
    printf("\nPRINTING FAT:\n");
    int cluster_count = bpb->bpbSectors+2;
    int ref_cluster1;
    int print_count = 0;
    for (int i = 2; i < cluster_count; i++){ 
        ref_cluster1 = get_fat_entry(i,image_buf,bpb);
        if (ref_cluster1 > 0){
            if (ref_cluster1 == (FAT12_MASK&CLUST_EOFE)){
                printf("Slot %4i:   EOFE",i);
            }else if (ref_cluster1 == (FAT12_MASK&CLUST_BAD)){
                printf("Slot %4i:    BAD",i);
            }else if (ref_cluster1 == (FAT12_MASK&CLUST_EOFS)){
                printf("Slot %4i:   EOFS",i);
            }else if (ref_cluster1 == (FAT12_MASK&CLUST_LAST)){
                printf("Slot %4i:   LAST",i);
            }else if (ref_cluster1 == (FAT12_MASK&CLUST_RSRVDS)){
                printf("Slot %4i: RSRVDS",i);
            }else if (ref_cluster1 == (FAT12_MASK&CLUST_RSRVDE)){
                printf("Slot %4i: RSRVDE",i);
            }else if (ref_cluster1 == (FAT12_MASK&CLUST_FREE)){
                printf("Slot %4i:   FREE",i);
            }
            else{
                printf("Slot %4i: %6i",i,ref_cluster1);
            }
            print_count++;
            if (print_count % 4 == 0){
                printf("\n");
            }
            else{
                printf("     ");
            }
        }
    }
    printf("\n");
}


// Scandisk is a filesystem checker for DOS filesystems.
// It can determine whether a filesystem is internally consistent,
// and correct any errors found.
// (Consistency in states!)
int main(int argc, char** argv) {
    uint8_t *image_buf;
    int fd; 
    struct bpb33* bpb; 
    if (argc < 2)
    {
	    usage(argv[0]);
    }
    
    image_buf = mmap_file(argv[1], &fd);
    bpb = check_bootsector(image_buf);
    struct cluster_refs* ref_count = malloc(sizeof(struct cluster_refs));
    // your code should start here... 
    ref_count->fat_unused_slots = fill_fat_ref_count(image_buf,bpb);
    // must fill FAT cluster ref here
    int cluster_count = bpb->bpbSectors+2;
    ref_count->direntry_unused_slots = calloc(cluster_count, sizeof(int));

    // go through every directory entry, check that size is consistent w/ cluster chain and vice versa
    printf("Checking for size consistency\n");
    traverse_root(image_buf, bpb, ref_count);
    // check for orphaned data
    printf("\nChecking for FAT data consistency\n");
    fix_orphans(image_buf,bpb,ref_count); 
    clean_up_fat(image_buf,bpb,ref_count);
    //clean up memory at end
    free(ref_count->direntry_unused_slots);
    free(ref_count->fat_unused_slots);
    free(ref_count);
    free(bpb);
    unmmap_file(image_buf, &fd); // close it
    return 0;
}
