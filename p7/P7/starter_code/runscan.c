#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("expected usage: ./runscan inputfile outputfile\n");
        exit(0);
    }

    /* This is some boilerplate code to help you get started, feel free to modify
       as needed! */

    int fd;
    fd = open(argv[1], O_RDONLY); /* open disk image */

    ext2_read_init(fd);

    struct ext2_super_block super;
    ext2_group_desc group;

    int num_groups = super.s_blocks_count / super.s_blocks_per_group;

    ext2_group_desc *groups = malloc(sizeof(ext2_group_desc *) * num_groups);

    if (groups == NULL)
    {
        printf("groups malloc failed\n");
        exit(1);
    }

    // example read first the super-block and group-descriptor
    read_super_block(fd, &super);
    read_group_descs(fd, &groups, num_groups);
    
    int inode_size = super.s_inode_size;   // inode size
    int inode_total = super.s_inodes_count; // total number of inodes in the file system.
    off_t inode_starter = locate_inode_table(0, &group);
    off_t inode_table = lseek(fd, inode_starter, SEEK_SET);
    
    // if the dir does not exist then create it 
    char output_dir[] = "output";
    struct stat st = {0};
    if (stat(output_dir, &st) == -1 ) mkdir(output_dir, 0700);
    for (int i = 0; i < inode_total; i ++){
        off_t inode_offset = inode_table + (inode_size * i);
        lseek(fd, inode_offset, SEEK_SET);
        struct ext2_inode inode;
        read(fd, &inode, sizeof(inode));
        uint32_t block_index = inode.i_block[0];
        if (block_index != 0){
            off_t offset = block_index * block_size;
            char buffer[block_size];
            pread(fd, buffer, block_size, offset);
            int is_jpg = 0; 
            if (buffer[0] == (char)0xff &&
                buffer[1] == (char)0xd8 &&
                buffer[2] == (char)0xff &&
                (buffer[3] == (char)0xe0 ||
                buffer[3] == (char)0xe1 ||
                buffer[3] == (char)0xe8)) 
            {
                is_jpg = 1;
            }
        
            if (is_jpg == 1){
                char output_filename[100];
                sprintf(output_filename, "output/file-%d.jpg", i + 1);
                FILE *output_file = fopen(output_filename, "w");
                


            }
        }
    }
    
    return 0;
}