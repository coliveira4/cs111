#include <stdio.h>
#include "ext2_fs.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>


typedef __uint32_t	__u32;
typedef __uint16_t	__u16;
typedef unsigned char	__u8;
typedef __int32_t	__s32;
typedef __int16_t	__s16;

void getFreeInodesInfo(__u32 block_size, __u32 inode_bitmap);
void getInodeSummaryInfo(__u32 inode_table_location);
void getDirentries(int parent_inode, struct ext2_inode dir_inode);
void getIndirectBlockInfo(__u32 level_of_directness, __u32 parent_num, __u32 block_id, __u32 logical_block_offset);



int my_filesystem; // File descriptor 

struct ext2_super_block my_superblock; //To store information about the superblock :: data about the entire system 
__u32 block_size;
__u16 inode_size;
__u32 inodes_per_group;


struct ext2_group_desc group_desc;
__u32 first_inode_block;



void getSuperBlockInfo()
{
/*
superblock summary
A single new-line terminated line, comprised of eight comma-separated fields (with no white-space), summarizing the key file system parameters:

1.SUPERBLOCK
2.total number of blocks (decimal)
3.total number of i-nodes (decimal)
4.block size (in bytes, decimal)
5.i-node size (in bytes, decimal)
6.blocks per group (decimal)
7.i-nodes per group (decimal)
8.first non-reserved i-node (decimal)
*/

//Puts superblock form the file system into the superblock data structure 
pread(my_filesystem, &my_superblock, sizeof(my_superblock), 1024); //"Is located at an offset of 1024 bytes from the start ofthe device"

__u32 total_number_of_blocks;
__u32 total_number_of_inodes;
__u32 blocks_per_group;
__u32 first_non_reserved_inode; 

total_number_of_blocks = my_superblock.s_blocks_count; 
total_number_of_inodes = my_superblock.s_inodes_count; 
block_size = EXT2_MIN_BLOCK_SIZE << my_superblock.s_log_block_size; //From header file 
inode_size = my_superblock.s_inode_size; 
blocks_per_group = my_superblock.s_blocks_per_group; 
inodes_per_group = my_superblock.s_inodes_per_group; 
first_non_reserved_inode = my_superblock.s_first_ino; 

printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", total_number_of_blocks, total_number_of_inodes, block_size, inode_size, blocks_per_group, inodes_per_group, first_non_reserved_inode);


}

void getGroupSummaryInfo(){
/*
group summary
Scan each of the groups in the file system. For each group, produce a new-line terminated line for each group, each comprised of nine comma-separated fields (with no white space), summarizing its contents.

GROUP
group number (decimal, starting from zero)
total number of blocks in this group (decimal)
total number of i-nodes in this group (decimal)
number of free blocks (decimal)
number of free i-nodes (decimal)
block number of free block bitmap for this group (decimal)
block number of free i-node bitmap for this group (decimal)
block number of first block of i-nodes in this group (decimal)
*/



	__u32 group_bsizes;
	__u32 group_isizes;
	__u32 num_free_blocks;
	__u32 num_free_inodes;
	__u32 free_block_bitmap;
	__u32 free_inode_bitmap;

	pread(my_filesystem, &group_desc, sizeof(group_desc), sizeof(struct ext2_super_block) + 1024);

	group_bsizes= my_superblock.s_blocks_count;
	group_isizes = my_superblock.s_inodes_per_group;
  //  printf("%d, %d", my_superblock.s_blocks_per_group, my_superblock.s_inodes_per_group);
	num_free_blocks = group_desc.bg_free_blocks_count;
	num_free_inodes = group_desc.bg_free_inodes_count;
	free_block_bitmap =  group_desc.bg_block_bitmap;
	free_inode_bitmap = group_desc.bg_inode_bitmap;
	first_inode_block = group_desc.bg_inode_table;


	fprintf(stdout, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
		0, group_bsizes, group_isizes,
		num_free_blocks, num_free_inodes,
		free_block_bitmap, free_inode_bitmap,
		first_inode_block);

	getFreeInodesInfo(block_size, free_inode_bitmap);


}
/*
free block entries
Scan the free block bitmap for each group. For each free block, produce a new-line terminated line, with two comma-separated fields (with no white space).

BFREE
number of the free block (decimal)
*/

void read_freeblocks(int index) { // Only reads one byte at time, but included with getFreeInodes which goes over the entire bitmap 
  char byte;
  int bit_c = 1;
  int leastsb_mask = 0x01;
  //__u32 block_size = EXT2_MIN_BLOCK_SIZE << my_superblock.s_log_block_size;

   pread(my_filesystem, &byte, 1, index + (group_desc.bg_block_bitmap * block_size) );
   int k = 0;
      while (k < 8) {
    int bit = byte & leastsb_mask;
    if (!bit) {
        fprintf(stdout, "BFREE,%d\n", bit_c + (8*index));
    }
    byte = byte >> 1;
    bit_c++;
    k++;
     }
}

/*
free inodes summary
1. IFREE
2. number of the free I-node (decimal)

There is only a single group, so it only needs to be used once. 
No need to iterate over all the groups. 
*/
void getFreeInodesInfo(__u32 block_size, __u32 inode_bitmap)
{

char one_byte; 

//A bitmap's size must fit in one block.
for (__u32 i = 0; i < block_size; i++) //Iterate over every byte in the bitmap
{
	read_freeblocks(i);
	//Read one byte of info 
	pread(my_filesystem, &one_byte, 1, i + (block_size * inode_bitmap));

	//Check if every bit in the byte is free. 
	//If not free, call the function to get its summary (different output)
	if ((1 & one_byte) == 0)
	{
		printf("IFREE,%d\n", 1 + (8*i)); //Add one because first inode is #1
	}
/*	else if (i < inodes_per_group)
	{
		getInodeSummaryInfo(1 + (8*i), inode_table);
	}*/
	if ((2 & one_byte) == 0)
	{
		printf("IFREE,%d\n", 2 + (8*i));
	}

	if ((4 & one_byte) == 0)
	{
		printf("IFREE,%d\n", 3 + (8*i));
	}

	if ((8 & one_byte) == 0)
	{
		printf("IFREE,%d\n", 4 + (8*i));
	}

	if ((16 & one_byte) == 0)
	{
		printf("IFREE,%d\n", 5 + (8*i));
	}

	if ((32 & one_byte) == 0)
	{
		printf("IFREE,%d\n", 6 + (8*i));
	}

	if ((64 & one_byte) == 0)
	{
		printf("IFREE,%d\n", 7 + (8*i));
	}

	if ((128 & one_byte) == 0)
	{
		printf("IFREE,%d\n", 8 + (8*i));
	}




}

}
/*

I-node summary
Scan the I-nodes for each group. For each allocated (non-zero mode and non-zero link count) I-node,
 produce a new-line terminated line, with up to 27 comma-separated fields (with no white space). 
 The first twelve fields are i-node attributes:

1.INODE
2.inode number (decimal)
3.file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
4.mode (low order 12-bits, octal ... suggested format "%o")
5.owner (decimal)
6.group (decimal)
7.link count (decimal)
8.time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
9.modification time (mm/dd/yy hh:mm:ss, GMT)
10.time of last access (mm/dd/yy hh:mm:ss, GMT)
11.file size (decimal)
12.number of (512 byte) blocks of disk space (decimal) taken up by this file
+ more
*/

void getInodeSummaryInfo(__u32 inode_table_location) //# of inode, which starts at 1
{

struct ext2_inode my_inode; 

//Inode number * inode_size should get the offset of the specific inode from the start of the inode table

	for (__u32 i = 0; i < inodes_per_group; i ++)
	{

	pread(my_filesystem, &my_inode, inode_size, (i)*inode_size + inode_table_location*block_size);
	
	if (my_inode.i_links_count == 0 && my_inode.i_mode == 0)
	{
		continue; 
	}

//Get file type
//"file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)"
//Look at i_mode value of each inode (First 2 bytes of each inode)
/*
0x8000	regular file
0x4000	directory
0xA000	symbolic link
*/ 
char file_type = '?';
switch ( (my_inode.i_mode >> 12) << 12 ){
	case 0x8000: file_type = 'f';
	break;
	case 0x4000: file_type = 'd';
	break;
	case 0xA000: file_type = 's';
	break;
	default: file_type = '?'; 
	}

int lower_twelve_bits = 4095 & my_inode.i_mode; //12 bits for "mode" output, which is different than i_mode

__u16 owner = my_inode.i_uid; 
__u16 group = my_inode.i_gid; 
__u16 link_count = my_inode.i_links_count;
__u32 file_size = my_inode.i_size;
__u32 number_of_512_blocks = my_inode.i_blocks;

time_t creation_time = my_inode.i_ctime; 
struct tm creat_t_struct = * gmtime(&creation_time);

time_t modification_time = my_inode.i_mtime; 
struct tm mod_t_struct = * gmtime(&modification_time);

time_t access_time = my_inode.i_atime; 
struct tm acc_t_struct = * gmtime(&access_time);
// Using https://www.tutorialspoint.com/c_standard_library/c_function_gmtime.htm
char * time_of_last_inode_change = malloc(18);
char * time_of_last_modification = malloc(18);
char * time_of_last_inode_access = malloc(18);

strftime(time_of_last_inode_change, 18, "%m/%d/%y %H:%M:%S", &creat_t_struct);
strftime(time_of_last_modification, 18, "%m/%d/%y %H:%M:%S", &mod_t_struct);
strftime(time_of_last_inode_access, 18, "%m/%d/%y %H:%M:%S", &acc_t_struct);

__u32 true_inode_num = i + 1; //Since inodes start at 1

printf("INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u", true_inode_num,file_type,lower_twelve_bits,owner,group,link_count,time_of_last_inode_change,time_of_last_modification,time_of_last_inode_access,file_size,number_of_512_blocks);

//For ordiary files (type 'f') and directories (type 'd') the next fifteen fields are block addresses (decimal, 12 direct, one indirect, one double indirect, one triple indirect).
if (file_type == 'f' || file_type == 'd')
{
for (int m = 0; m <=14; m++)
{
printf(",%u", my_inode.i_block[m]);

}
printf("\n");
}

if (file_type == 'd')
{
	getDirentries(true_inode_num, my_inode);
}


/*
Symbolic links may be a little more complicated.
If the file length is less than or equal to the size of the block pointers (60 bytes) 
the file will contain zero data blocks, and the name is stored in the space normally occupied 
by the block pointers. If this is the case, the fifteen block pointers should not be printed. 
If, however, the file length is greater than 60 bytes, print out the fifteen block nunmbes as
for ordinary files and directories.*/
if (file_type == 's') 
{

	if (file_size > 60)
	{
		for (int m = 0; m <=14; m++)
			{
			printf(",%u", my_inode.i_block[m]);
			}
		printf("\n");
	}




}

//The 13th entry in this array is the block number of the first indirect block; which is a block containing an array of block ID containing the data.
//The 14th entry in this array is the block number of the first doubly-indirect block
//The 15th entry in this array is the block number of the triply-indirect block

/*
Block 1 to 12 are direct
■ Block 13 to 268 are indirect
■ How many double indirect blocks? (256)*(256) = 65536
■ How many triple indirect blocks? (256)*(256)*(256) = 16777216
*/

if (my_inode.i_block[12] && (file_type == 'f' || file_type =='d')) 
{
getIndirectBlockInfo(1, true_inode_num, my_inode.i_block[12], 12);
}

if (my_inode.i_block[13]&& (file_type == 'f' || file_type =='d')) 
{
getIndirectBlockInfo(2, true_inode_num, my_inode.i_block[13], 268);

}

if (my_inode.i_block[14] && (file_type == 'f' || file_type =='d')) 
{
getIndirectBlockInfo(3, true_inode_num, my_inode.i_block[14], 65536+268);

}


	}//End of for loop




}

/*

For each directory I-node, scan every data block. For each valid (non-zero I-node number) directory entry, produce a new-line terminated line, with seven comma-separated fields (no white space).

DIRENT
parent inode number (decimal) ... the I-node number of the directory that contains this entry
logical byte offset (decimal) of this entry within the directory
inode number of the referenced file (decimal)
entry length (decimal)
name length (decimal)
name (string, surrounded by single-quotes). Don't worry about escaping, we promise there will be no single-quotes or commas in any of the file names.

*/

void getDirentries(int parent_inode, struct ext2_inode dir_inode) {
  struct ext2_dir_entry dir;
  int blocks_used = dir_inode.i_blocks/(2<<my_superblock.s_log_block_size);
  int offset = 0;
  int i = 0;
  while (i < blocks_used) {
    while (offset < 1024) {
      lseek(my_filesystem, offset + dir_inode.i_block[i]*block_size, SEEK_SET); 
      read (my_filesystem, &dir, sizeof(struct ext2_dir_entry));
      if (dir.inode)
  fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n", parent_inode, offset, dir.inode, dir.rec_len, dir.name_len, dir.name);
      offset += dir.rec_len;
    }
    offset = 0;
    i++;
  }
}

/*
The I-node summary contains a list of all 12 blocks, and the primary single, double, and triple indirect blocks.
We also need to know about the blocks that are pointed to by those indirect blocks.
For each file or directory I-node, scan the single indirect blocks and (recursively) 
the double and triple indirect blocks. For each non-zero block pointer you find, 
produce a new-line terminated line with six comma-separated fields (no white space).

1. INDIRECT
2. I-node number of the owning file (decimal)
3. (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
4. logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
5. block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
6. block number of the referenced block (decimal)


*/

/*15 x 32bit block numbers
 pointing to the blocks containing the data for this inode*/
void getIndirectBlockInfo(__u32 level_of_directness, __u32 parent_num, __u32 block_id, __u32 logical_block_offset )
{

//which is a block containing an array of block ID containing the data. 
__u32 * block_of_array = malloc(block_size); //iterate over this block
__u32 num_of_block_ids = block_size/32; //block id is 32 bits

__u32 block_location = (block_id-1)*block_size + 1024; //To account for superblock 

//Read in the whole block containing arrays // either single, doubly, or triply
pread(my_filesystem, block_of_array, block_size, block_location);


for (__u32 b = 0; b < num_of_block_ids; b++)
{

	if (block_of_array[b])
	{

if(level_of_directness != 1){
	__u32 parents_inode_num = parent_num;	
	__u32 referenced_block_num = block_of_array[b];
	printf("INDIRECT,%u,%u,%u,%u,%u\n", parents_inode_num, level_of_directness, logical_block_offset, block_id, referenced_block_num);
	
}


	switch (level_of_directness)
	{
		case 1: logical_block_offset = logical_block_offset + b; 
		__u32 parents_inode_num = parent_num;	
		__u32 referenced_block_num = block_of_array[b];
		printf("INDIRECT,%u,%u,%u,%u,%u\n", parents_inode_num, level_of_directness, logical_block_offset, block_id, referenced_block_num);
		break;
		case 2: logical_block_offset = logical_block_offset + b;  
		getIndirectBlockInfo(level_of_directness-1, parent_num, block_of_array[b], logical_block_offset);
		break; 
		case 3: logical_block_offset = logical_block_offset + b;
		getIndirectBlockInfo(level_of_directness-1, parent_num, block_of_array[b], logical_block_offset);
		break;
	}



	}


}




}


int main (int argc, char **argv)
{

int number_of_args = argc; // To stop warnings
number_of_args = number_of_args + 1 - 1; //To stop warnings

// Get name of file system to analyze and open it
char * file_system_argument = argv[1]; 
my_filesystem = open(file_system_argument, O_RDONLY);

getSuperBlockInfo();
getGroupSummaryInfo();
getInodeSummaryInfo(first_inode_block); 


return 0;

}