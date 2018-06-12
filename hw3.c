/*******************************************************************************/
// Muhammed Furkan YAĞBASAN
// 2099505
/*******************************************************************************/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ext2.h"
#include <string.h>

#define BASE_OFFSET 1024
#define EXT2_BLOCK_SIZE 1024
#define IMAGE "image.img"

typedef unsigned char bmap;
#define __NBITS (8 * (int) sizeof (bmap))
#define __BMELT(d) ((d) / __NBITS)
#define __BMMASK(d) ((bmap) 1 << ((d) % __NBITS))
#define BM_SET(d, set) ((set[__BMELT (d)] |= __BMMASK (d)))
#define BM_CLR(d, set) ((set[__BMELT (d)] &= ~__BMMASK (d)))
#define BM_ISSET(d, set) ((set[__BMELT (d)] & __BMMASK (d)) != 0)

unsigned int block_size = 0;
#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1)*block_size)


struct ext2_dir_entry_2 {
        unsigned int   inode;     /* Inode number */
        unsigned short rec_len;   /* Directory entry length */
        unsigned char  name_len;  /* Name length */
        unsigned char  file_type;
        char           name[10];    /* File name, up to EXT2_NAME_LEN */
};

/*******************************************************************************/
// Check if inode blocks is not overwritten
//
int isRecoverable(int fd, struct ext2_group_desc group, struct ext2_inode inode){
  bmap *bitmap;
  bitmap = malloc(block_size);

  lseek(fd, BLOCK_OFFSET(group.bg_block_bitmap), SEEK_SET);
  read(fd, bitmap, block_size);

  for(int i=0; i<15; i++){
    if(inode.i_block[i] != 0){

      if(BM_ISSET(inode.i_block[i]-1,bitmap)){
        free(bitmap);
        return 0;
      }

      unsigned int *block;
      block = malloc(block_size);

      unsigned int *block1;
      block1 = malloc(block_size);

      unsigned int *block2;
      block2 = malloc(block_size);

      if(i==12){
        lseek(fd, BLOCK_OFFSET(inode.i_block[i]), SEEK_SET);
        read(fd, block, block_size);

        for(int k=0; k<block_size/sizeof(unsigned int); k++){
          if(block[k] != 0){
            if(BM_ISSET(block[k]-1,bitmap)){
              free(bitmap);free(block);free(block1);free(block2);
              return 0;
            }
          }
        }
      }

      else if(i==13){
        lseek(fd, BLOCK_OFFSET(inode.i_block[i]), SEEK_SET);
        read(fd, block, block_size);

        for(int k=0; k<block_size/sizeof(unsigned int); k++){
          if(block[k] != 0){
            if(BM_ISSET(block[k]-1,bitmap)){
              free(bitmap);free(block);free(block1);free(block2);
              return 0;
            }
            lseek(fd, BLOCK_OFFSET(block[k]), SEEK_SET);
            read(fd, block1, block_size);
            for(int h=0; h<block_size/sizeof(unsigned int); h++){
              if(block1[h] != 0){
                if(BM_ISSET(block1[h]-1,bitmap)){
                  free(bitmap);free(block);free(block1);free(block2);
                  return 0;
                }
              }
            }
          }
        }
      }

      else if(i==14){
        lseek(fd, BLOCK_OFFSET(inode.i_block[i]), SEEK_SET);
        read(fd, block, block_size);

        for(int k=0; k<block_size/sizeof(unsigned int); k++){
          if(block[k] != 0){
            if(BM_ISSET(block[k]-1,bitmap)){
              free(bitmap);free(block);free(block1);free(block2);
              return 0;
            }
            lseek(fd, BLOCK_OFFSET(block[k]), SEEK_SET);
            read(fd, block1, block_size);
            for(int h=0; h<block_size/sizeof(unsigned int); h++){
              if(block1[h] != 0){
                if(BM_ISSET(block1[h]-1,bitmap)){
                  free(bitmap);free(block);free(block1);free(block2);
                  return 0;
                }
                lseek(fd, BLOCK_OFFSET(block1[h]), SEEK_SET);
                read(fd, block2, block_size);
                for(int t=0; t<block_size/sizeof(unsigned int); t++){
                  if(block2[t] != 0){
                    if(BM_ISSET(block2[t]-1,bitmap)){
                      free(bitmap);free(block);free(block1);free(block2);
                      return 0;
                    }
                  }
                }
              }
            }
          }
        }
      }

      free(block);
      free(block1);
      free(block2);
    }
  }

  free(bitmap);
  return 1;
}

/*******************************************************************************/
// Set the bits of block bitmap for the file that will be recovered
//
void takeBlockBitmapBack(int fd, struct ext2_group_desc group, struct ext2_inode inode){

  bmap *bitmap;
  bitmap = malloc(block_size);

  lseek(fd, BLOCK_OFFSET(group.bg_block_bitmap), SEEK_SET);
  read(fd, bitmap, block_size);

  unsigned int *block;
  block = malloc(block_size);

  unsigned int *block1;
  block1 = malloc(block_size);

  unsigned int *block2;
  block2 = malloc(block_size);

  for(int i=0; i<15; i++){
    if(inode.i_block[i] != 0){

      BM_SET(inode.i_block[i]-1,bitmap);

      if(i==12){
        lseek(fd, BLOCK_OFFSET(inode.i_block[i]), SEEK_SET);
        read(fd, block, block_size);

        for(int k=0; k<block_size/sizeof(unsigned int); k++){
          if(block[k] != 0){
            BM_SET(block[k]-1,bitmap);
          }
        }
      }

      else if(i==13){
        lseek(fd, BLOCK_OFFSET(inode.i_block[i]), SEEK_SET);
        read(fd, block, block_size);

        for(int k=0; k<block_size/sizeof(unsigned int); k++){
          if(block[k] != 0){
            BM_SET(block[k]-1,bitmap);
            lseek(fd, BLOCK_OFFSET(block[k]), SEEK_SET);
            read(fd, block1, block_size);
            for(int h=0; h<block_size/sizeof(unsigned int); h++){
              if(block1[h] != 0){
                BM_SET(block1[h]-1,bitmap);
              }
            }
          }
        }
      }

      else if(i==14){
        lseek(fd, BLOCK_OFFSET(inode.i_block[i]), SEEK_SET);
        read(fd, block, block_size);

        for(int k=0; k<block_size/sizeof(unsigned int); k++){
          if(block[k] != 0){
            BM_SET(block[k]-1,bitmap);
            lseek(fd, BLOCK_OFFSET(block[k]), SEEK_SET);
            read(fd, block1, block_size);
            for(int h=0; h<block_size/sizeof(unsigned int); h++){
              if(block1[h] != 0){
                BM_SET(block1[h]-1,bitmap);
                lseek(fd, BLOCK_OFFSET(block1[h]), SEEK_SET);
                read(fd, block2, block_size);
                for(int t=0; t<block_size/sizeof(unsigned int); t++){
                  if(block2[t] != 0){
                    BM_SET(block2[t]-1,bitmap);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  lseek(fd, BLOCK_OFFSET(group.bg_block_bitmap), SEEK_SET);
  write(fd, bitmap, block_size);
  free(block);
  free(block1);
  free(block2);
  free(bitmap);

}

/*******************************************************************************/
// MAIN FUNCTION
//
int main(int argc, char **argv)
{
  struct ext2_super_block super;
  struct ext2_group_desc group;
  int fd;

  if ((fd = open(argv[1], O_RDWR)) < 0) {
    perror(IMAGE);
    exit(1);
  }

  // read super-block
  lseek(fd, BASE_OFFSET, SEEK_SET);
  read(fd, &super, sizeof(super));
  if (super.s_magic != EXT2_SUPER_MAGIC) {
    fprintf(stderr, "Not a Ext2 filesystem\n");
    exit(1);
  }

  //block size
  block_size = 1024 << super.s_log_block_size;

  // read group descriptor
  lseek(fd, BASE_OFFSET + block_size, SEEK_SET);
  read(fd, &group, sizeof(group));

  unsigned int fileNum =0;  // deleted files
  unsigned int recovedFileNum =0; // deleted and will be recovered files
  struct ext2_inode inode;

  unsigned int recovereds[100];
  unsigned int idx = 0;

  unsigned int recovery_offset;
  unsigned int recovery_block;
  
  // Traverse inodes
  for(int i = 0; i< super.s_inodes_count; i++){

    lseek(fd, BLOCK_OFFSET(group.bg_inode_table)+i*sizeof(struct ext2_inode), SEEK_SET);
    read(fd, &inode, sizeof(struct ext2_inode));

    if(inode.i_dtime != 0) {
      printf("file%02d ",++fileNum);
      printf("%u ",inode.i_dtime);
      printf("%u\n",inode.i_blocks);
      
      if(isRecoverable(fd, group, inode) == 0){
        //inode.i_dtime = 0;
        continue;
      }
      inode.i_mode = 0x81b2;
      inode.i_dtime = 0;
      inode.i_links_count++;

      lseek(fd, BLOCK_OFFSET(group.bg_inode_table)+i*sizeof(struct ext2_inode), SEEK_SET);
      write(fd, &inode, sizeof(struct ext2_inode));

      super.s_free_inodes_count--;
      lseek(fd, BASE_OFFSET, SEEK_SET);
      write(fd, &super, sizeof(super));

      bmap *bitmap;
      bitmap = malloc(block_size);
      lseek(fd, BLOCK_OFFSET(group.bg_inode_bitmap), SEEK_SET);
      read(fd, bitmap, block_size);

      BM_SET(i,bitmap);

      lseek(fd, BLOCK_OFFSET(group.bg_inode_bitmap), SEEK_SET);
      write(fd, bitmap, block_size);

      free(bitmap);
     
      takeBlockBitmapBack(fd, group, inode);

      ////////////////////////////////////
      struct ext2_inode inodeLost;
      unsigned char *entry;
      entry = malloc(block_size);
      lseek(fd, BLOCK_OFFSET(group.bg_inode_table)+10*sizeof(struct ext2_inode), SEEK_SET);
      read(fd, &inodeLost, sizeof(struct ext2_inode));

      if(24+(recovedFileNum)*20 <= block_size){
        recovery_offset = 24;
        recovery_block = 0;
      } else {
        recovery_offset = 0;
        recovery_block = 1;
        recovedFileNum = recovedFileNum%((block_size-24)/20)-1;
      }
      lseek(fd, BLOCK_OFFSET(inodeLost.i_block[recovery_block]), SEEK_SET);
      read(fd, entry, block_size); 

      if(recovedFileNum==0){
        entry[16]=0x0c;       // arrage rec_len for .. directory
        entry[17]=0x00;
      }
      else if(recovedFileNum > 0){
        entry[recovery_offset+(recovedFileNum-1)*20+4]=0x14;       // rearrange rec_len for last recovered file
        entry[recovery_offset+(recovedFileNum-1)*20+4+1]=0x00;
      }

      lseek(fd, BLOCK_OFFSET(inodeLost.i_block[recovery_block]), SEEK_SET);
      write(fd, entry, block_size);

      free(entry);

      struct ext2_dir_entry_2 loadEntry;
      loadEntry.inode = i+1;
      loadEntry.rec_len = block_size-recovery_offset-(++recovedFileNum-1)*20;   // rec_len of the last coming recovered file
      loadEntry.name_len = 6;
      loadEntry.file_type = EXT2_FT_REG_FILE;
      loadEntry.name[0] = 'f';
      loadEntry.name[1] = 'i';
      loadEntry.name[2] = 'l';
      loadEntry.name[3] = 'e';
      loadEntry.name[4] = fileNum/10 +48;
      loadEntry.name[5] = fileNum%10 +48;
      loadEntry.name[6] = 0;
      loadEntry.name[7] = 0;
      loadEntry.name[8] = 0;
      loadEntry.name[9] = 0;



      recovereds[idx++] = fileNum;

      lseek(fd, BLOCK_OFFSET(inodeLost.i_block[recovery_block])+recovery_offset+(recovedFileNum-1)*20, SEEK_SET);
      write(fd, &loadEntry, 20); 
    }
  }

  printf("###\n");
  for(int k=0; k<idx; k++){
    printf("file%02d\n", recovereds[k]);
  }

  close(fd);
  return 0;
}
