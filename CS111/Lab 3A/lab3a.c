//NAME: Darren Lu
//EMAIL: darrenlu3@ucla.edu
//ID: 205394473

#include <stdio.h>
#include "ext2_fs.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define SUPERBLOCK_OFFSET 1024

struct ext2_super_block superblock;
int extfd;
unsigned int inodes_count = 0, blocks_count = 0, log_block_size = 0, inode_size = 0, blocks_per_group = 0, inodes_per_group = 0, first_unreserved = 0;
int block_size = 0;

void read_free_blocks(unsigned int groupnum, unsigned int bitmap){
    char* bitmap_chars = (char*) malloc(block_size*sizeof(char));
    unsigned long offset = SUPERBLOCK_OFFSET + (bitmap-1)*block_size;
    unsigned int position = superblock.s_first_data_block + (groupnum * blocks_per_group);
    
    if (pread(extfd, bitmap_chars, block_size, offset) == -1){
        fprintf(stderr,"Error reading free blocks. Reason:%s\n",strerror(errno));
        _exit(2);
    };
    
    int i, j;
    for (i = 0; i < block_size; i++){
        char used = bitmap_chars[i];
        int offset = 0;
        for (j = 0; j < 8; j++){
            if(!(used&(1<<offset))){
                printf("BFREE,%d\n",position);
            }
            position++;
            offset++;
        }
    }
    
    //fprintf(stderr, "%s\n", bitmap_chars);
    free(bitmap_chars);
}

void read_free_inodes(unsigned int groupnum, unsigned int bitmap){
    char* bitmap_chars = (char*) malloc(block_size*sizeof(char));
    unsigned long offset = SUPERBLOCK_OFFSET + (bitmap-1)*block_size;
    unsigned int position = superblock.s_first_data_block + (groupnum * blocks_per_group);
    
    if (pread(extfd, bitmap_chars, block_size, offset) == -1){
        fprintf(stderr,"Error reading free inodes. Reason:%s\n",strerror(errno));
        _exit(2);
    };
    
    int i, j;
    for (i = 0; i < block_size; i++){
        char used = bitmap_chars[i];
        int offset = 0;
        for (j = 0; j < 8; j++){
            if(!(used&(1<<offset))){
                printf("IFREE,%d\n",position);
            }
            position++;
            offset++;
        }
    }
    
    free(bitmap_chars);
}

void gettime(const time_t time, char* output){
    struct tm result = *gmtime(&time);
    strftime(output, 80, "%m/%d/%y %H:%M:%S",&result); //from https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
    return;
}

void enter_dir(unsigned int position, unsigned int iblock){
    struct ext2_dir_entry entry;
    unsigned long offset = SUPERBLOCK_OFFSET + (iblock-1)*block_size;
    int dir_bytes = 0;
    
    while(dir_bytes < block_size){
        memset(entry.name, 0, 256);
        if (pread(extfd, &entry, sizeof(entry), offset) == -1){
            fprintf(stderr,"Error reading inode dir entry %d at position %d. Reason:%s\n",iblock, position,strerror(errno));
            _exit(2);
        };
        if (entry.inode != 0){
            memset(&entry.name[entry.name_len], 0, 256 - entry.name_len);
            printf("DIRENT,%d,%d,%d,%d,%d,\'%s\'\n",position,dir_bytes,entry.inode,entry.rec_len,entry.name_len,entry.name);
        }
        dir_bytes += entry.rec_len;
        offset += entry.rec_len;
    }
//    while(dir_bytes < inode.i_size){
//        char file_name[EXT2_NAME_LEN+1];
//        if (pread(extfd, &entry, sizeof(entry), offset) == -1){
//            fprintf(stderr,"Error reading inode dir entry %d at position %d. Reason:%s\n",iblock, position,strerror(errno));
//            _exit(2);
//        };
//        memcpy(file_name, entry.name,entry.name_len);
//        file_name[entry.name_len]=0;
//        printf("DIRENT,%d,%d,%d,%d,%d,\'%s\'\n",position,dir_bytes,entry.inode,entry.rec_len,entry.name_len,entry.name);
//        dir_bytes += entry.rec_len;
//        offset += dir_bytes;
//    }
    
}

void access_inode(unsigned int table, unsigned int relative_position, unsigned int position){
    struct ext2_inode inode;
    unsigned long offset = SUPERBLOCK_OFFSET + (table-1)*block_size + relative_position*sizeof(inode);
    
    if (pread(extfd, &inode, sizeof(inode), offset) == -1){
        fprintf(stderr,"Error reading inode at relative relative position %d. Reason:%s\n", relative_position,strerror(errno));
        _exit(2);
    };
    
    if (inode.i_links_count == 0 || inode.i_mode == 0) return;
    
    char type = '?';
    if (S_ISDIR(inode.i_mode)) type = 'd';
    if (S_ISREG(inode.i_mode)) type = 'f';
    if (S_ISLNK(inode.i_mode)) type = 's';

    char lastchange[25],modification[25],lastaccess[25];
    gettime(inode.i_ctime, lastchange);
    gettime(inode.i_mtime, modification);
    gettime(inode.i_atime, lastaccess);
    
    int blocks = 2*(inode.i_blocks/(2<<log_block_size));
    
    printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",position,type,inode.i_mode&0x0FFF,inode.i_uid,inode.i_gid,inode.i_links_count,lastchange,modification,lastaccess,inode.i_size,blocks);
    
    int i;
    for (i = 0; i < 15; i++){
        printf(",%d",inode.i_block[i]);
    }
    printf("\n");
    
    //directory direct entry case
    if(type == 'd'){
        for (i = 0; i < 12; i++){
            if (inode.i_block[i] != 0) enter_dir(position, inode.i_block[i]);
        }
    }
    
    //first level indirect entry
    if(inode.i_block[12] != 0){
        unsigned int *blocks = malloc(block_size);
        unsigned int offset_one = SUPERBLOCK_OFFSET + (inode.i_block[12]-1) * block_size;
        if (pread(extfd, blocks, block_size, offset_one) == -1){
            fprintf(stderr,"Error reading used inodes. Reason:%s\n",strerror(errno));
            _exit(2);
        };
        unsigned long j;
        for (j = 0; j < block_size/sizeof(uint32_t); j++){
            if(blocks[j]!=0){
                if (type == 'd'){
                    enter_dir(position, inode.i_block[12]);
                }
                printf("INDIRECT,%d,1,%lu,%d,%d\n",position,12+j,inode.i_block[12],blocks[j]);
            }
        }
    }
    
    //second level indirect entry repeats first level with different offsets
    if(inode.i_block[13] != 0){
        unsigned int *blocks = malloc(block_size);
        unsigned int offset_one = SUPERBLOCK_OFFSET + (inode.i_block[13]-1) * block_size;
        if (pread(extfd, blocks, block_size, offset_one) == -1){
            fprintf(stderr,"Error reading indirect directory. Reason:%s\n",strerror(errno));
            _exit(2);
        };
        unsigned long j;
        for (j = 0; j < block_size/sizeof(uint32_t); j++){
            if(blocks[j]!=0){
                printf("INDIRECT,%d,2,%lu,%d,%d\n",position,256+12+j,inode.i_block[13],blocks[j]);
                
                //repeat the algorithm
                unsigned int *blocks2 = malloc(block_size);
                unsigned int offset_two = SUPERBLOCK_OFFSET + (blocks[j]-1) * block_size;
                if (pread(extfd, blocks2, block_size, offset_two) == -1){
                    fprintf(stderr,"Error reading used indirect directory. Reason:%s\n",strerror(errno));
                    _exit(2);
                };
                unsigned long k;
                for (k = 0; k < block_size/sizeof(uint32_t); k++){
                    if(blocks2[k]!=0){
                        if (type == 'd'){
                            enter_dir(position, blocks2[k]);
                        }
                        printf("INDIRECT,%d,1,%lu,%d,%d\n",position,256+12+k,blocks[j],blocks2[k]);
                    }
                }
            }
        }
    }
    
    //third level indirect entry repeats second level with different offsets
    if(inode.i_block[14] != 0){
        unsigned int *blocks = malloc(block_size);
        unsigned int offset_one = SUPERBLOCK_OFFSET + (inode.i_block[14]-1) * block_size;
        if (pread(extfd, blocks, block_size, offset_one) == -1){
            fprintf(stderr,"Error reading indirect directory. Reason:%s\n",strerror(errno));
            _exit(2);
        };
        unsigned long j;
        for (j = 0; j < block_size/sizeof(uint32_t); j++){
            if(blocks[j]!=0){
                printf("INDIRECT,%d,3,%lu,%d,%d\n",position,65536+256+12+j,inode.i_block[14],blocks[j]);
                
                //repeat the algorithm
                unsigned int *blocks2 = malloc(block_size);
                unsigned int offset_two = SUPERBLOCK_OFFSET + (blocks[j]-1) * block_size;
                if (pread(extfd, blocks2, block_size, offset_two) == -1){
                    fprintf(stderr,"Error reading used indirect directory. Reason:%s\n",strerror(errno));
                    _exit(2);
                };
                unsigned long k;
                for (k = 0; k < block_size/sizeof(uint32_t); k++){
                    if(blocks2[k]!=0){
                        printf("INDIRECT,%d,2,%lu,%d,%d\n",position,65536+256+12+k,blocks[j],blocks2[k]);
                        
                        //repeat the algorithm
                        unsigned int *blocks3 = malloc(block_size);
                        unsigned int offset_three = SUPERBLOCK_OFFSET + (blocks2[k]-1) * block_size;
                        if (pread(extfd, blocks3, block_size, offset_three) == -1){
                            fprintf(stderr,"Error reading used indirect directory. Reason:%s\n",strerror(errno));
                            _exit(2);
                        };
                        unsigned long l;
                        for (l = 0; l < block_size/sizeof(uint32_t); l++){
                            if(blocks3[l]!=0){
                                printf("INDIRECT,%d,1,%lu,%d,%d\n",position,65536+256+12+l,blocks2[k],blocks3[l]);
                            }
                        }
                    }
                }
            }
        }
    }
}

void read_inodes(unsigned int groupnum, unsigned int bitmap, unsigned int table){
    char* bitmap_chars = (char*) malloc(block_size*sizeof(char));
    unsigned long offset = SUPERBLOCK_OFFSET + (bitmap-1)*block_size;
    unsigned int position = superblock.s_first_data_block + (groupnum * blocks_per_group);
    unsigned int starting_position = position;
    int inode_bytes = inodes_per_group/8;
    
    if (pread(extfd, bitmap_chars, block_size, offset) == -1){
        fprintf(stderr,"Error reading used inodes. Reason:%s\n",strerror(errno));
        _exit(2);
    };
    
    int i, j;
    for (i = 0; i < inode_bytes; i++){
        char used = bitmap_chars[i];
        int offset = 0;
        for (j = 0; j < 8; j++){
            if(used&(1<<offset)){
                access_inode(table, position - starting_position, position);
                //printf("IFREE,%d\n",position);
            }
            position++;
            offset++;
        }
    }
    
    free(bitmap_chars);
}

void read_group(unsigned int groupnum){
    //check group descriptor
    unsigned int startblock = 0;
    if (block_size == 1024) {startblock = 2;} //start at block 2
    else if (block_size > 1024) {startblock = 1;} //start at block 1
    else{
        fprintf(stderr,"Block size is less than 1024, exiting.");
        _exit(2);
    }
    //set offset based on block size
    unsigned long offset = block_size * startblock;
    struct ext2_group_desc group_descriptor;
    if (pread(extfd, &group_descriptor, sizeof(group_descriptor), offset) == -1){
        fprintf(stderr,"Error reading group descriptor block. Reason:%s\n",strerror(errno));
        _exit(2);
    };
    
    
    //group_descriptor now contains info on group
    //since there is only one group, and this is the last group, the number of blocks in the group is what is remaining of the rest of the blocks
    unsigned int blocks_in_group = superblock.s_blocks_count;
    //get info in position of block bitmap and inode bitmap and first block of inodes
    unsigned int block_bitmap = group_descriptor.bg_block_bitmap;
    unsigned int inode_bitmap = group_descriptor.bg_inode_bitmap;
    unsigned int inode_table = group_descriptor.bg_inode_table;
    unsigned int free_blocks = group_descriptor.bg_free_blocks_count;
    printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",groupnum,blocks_in_group,inodes_per_group,free_blocks,group_descriptor.bg_free_inodes_count,block_bitmap,inode_bitmap,inode_table);
    
    //checking free blocks
    read_free_blocks(groupnum, block_bitmap);
    //checking free inodes
    read_free_inodes(groupnum, inode_bitmap);
    
    //getting inode summaries
    read_inodes(groupnum, inode_bitmap, inode_table);
}

void read_superblock(){
    //reading the superblock
    if (pread(extfd, &superblock, sizeof(superblock), SUPERBLOCK_OFFSET) == -1){
        fprintf(stderr,"Error reading superblock. Reason:%s\n",strerror(errno));
        _exit(2);
    };
    inodes_count = superblock.s_inodes_count;
    blocks_count = superblock.s_blocks_count;
    block_size = 1024 << superblock.s_log_block_size;
    inode_size = superblock.s_inode_size;
    blocks_per_group = superblock.s_blocks_per_group;
    inodes_per_group = superblock.s_inodes_per_group;
    first_unreserved = superblock.s_first_ino;
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",blocks_count,inodes_count,block_size,inode_size,blocks_per_group,inodes_per_group,first_unreserved);
}

int main(int argc, char* argv[]){
    if (argc != 2){
        fprintf(stderr, "Incorrect arguments given! Format: ./lab3a (file system image path)\n");
        _exit(1);
    }
    if ((extfd = open(argv[1], O_RDONLY)) == -1){
        fprintf(stderr, "Could not open %s\n)", argv[1]);
        _exit(1);
    }
    
    read_superblock();
    
    //checking block group
    read_group(0);
    
    
    return 0;
}

