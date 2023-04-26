#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

ext2_group_desc *groups;
void read_directory_data_blocks(int fd, struct ext2_inode inode, struct ext2_super_block super)
{
    off_t inode_starter = locate_inode_table(0, groups);
    struct ext2_inode root;
    read_inode(fd, inode_starter, EXT2_ROOT_INO, &inode, super.s_inode_size);
    int num_blocks = (inode.i_size + block_size - 1) / block_size;
    for (int i = 0; i < EXT2_N_BLOCKS; i++) // ext2_n_block is max number of block in inode
    {
        if (inode.i_block[i] != 0)
        {
            int offset = 0;
            while (offset < block_size && num_blocks > 0)
            {
                char buffer[block_size];

                int read_size = block_size;
                pread(fd, buffer, read_size, inode.i_block[i] * block_size + offset);
                struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)buffer;
                while ((char *)entry < buffer + read_size)
                {
                    
                    char name[EXT2_NAME_LEN + 1];

                    // perhaps get the name from the entry
                }
            }
        }
    }
}



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
    groups = malloc(sizeof(ext2_group_desc) * num_groups);
    read_super_block(fd, &super);
    read_group_descs(fd, groups, num_groups);

    // int num_groups = super.s_blocks_count / super.s_blocks_per_group;
    int num_groups = (super.s_blocks_count + blocks_per_group - 1) / blocks_per_group; // this was in other starter file
    printf("num of groups in line 29: %d\n", num_groups);
    printf("num of blocks in line 30: %d\n", super.s_blocks_count);
    printf("num of blocks per group in line 31: %d\n", blocks_per_group);

    if (groups == NULL)
    {
        printf("groups malloc failed\n");
        exit(1);
    }

    // example read first the super-block and group-descriptor

    // int inode_size = super.s_inode_size; // inode size
    // int inode_total = super.s_inodes_count; // total number of inodes in the file system.
    off_t inode_starter;
    // off_t inode_starter = locate_inode_table(0, &group);    // change to first element of groups?
    // off_t inode_starter = locate_inode_table(0, &groups); // change to first element of groups?
    // off_t inode_table;

    // if the dir does not exist then create it
    // char output_dir[] = "output";

    struct stat st = {0};
    if (stat(argv[2], &st) == -1)
        mkdir(argv[2], 0700);

    // if (stat(output_dir, &st) == -1)
    //     mkdir(output_dir, 0700);

    // ext2_group_desc group;

    // off_t inode_offset;

    // int file_name_number = 1;
    int file_name_number = 1;
    // int i_num = 1; // track which inode num this is
    int count = 0; // for test case
    // printf("hello world 60\n");
    // printf("num of groups: %d\n", num_groups);

    // FILE *output_file;

    // char output_filename[100] = ;
    printf("new file name: %s", "test");

    for (int i = 0; i < num_groups; i++)
    {
        inode_starter = locate_inode_table(i, groups);

        for (int j = 0; j < (int)super.s_inodes_per_group; j++)
        {
            // inode_offset = inode_table + (inode_size * j);
            struct ext2_inode inode;
            read_inode(fd, inode_starter, j + 1, &inode, super.s_inode_size);

            uint32_t block_index = inode.i_block[0];
            if (block_index != 0)
            {
                // printf("hello world\n");
                off_t offset = block_index * block_size;
                char buffer[block_size];
                pread(fd, buffer, block_size, offset);
                //  printf("%s\n", buffer);
                int is_jpg = 0;
                if (buffer[0] == (char)0xff &&
                    buffer[1] == (char)0xd8 &&
                    buffer[2] == (char)0xff &&
                    (buffer[3] == (char)0xe0 ||
                     buffer[3] == (char)0xe1 ||
                     buffer[3] == (char)0xe8))
                {
                    // printf("hello world\n");
                    is_jpg = 1;
                }

                if (is_jpg == 1)
                {
                    FILE *output_file;
                    printf("found jpg with inode num %d\n", file_name_number);
                    count++;
                    char output_filename[100];
                    sprintf(output_filename, "%s/file-%d.jpg", argv[2], file_name_number);
                    // file_name_number++;
                    output_file = fopen(output_filename, "w");
                    if (output_file == NULL)
                        printf("the output file is null\n");

                    printf("output file name is: %s\n", output_filename);
                    int file_size = inode.i_size;
                    printf("file size is: %d bytes\n", file_size);

                    int blocks_required = (file_size + file_size % block_size) / block_size;
                    printf("blocks required: %d\n", blocks_required);

                    for (int b = 0; b < blocks_required; b++)
                    {
                        if (b <= 11)
                        {
                            block_index = inode.i_block[b];
                            printf("block index: %d\n", block_index);
                        }
                        else
                        {
                            // need to read indirect block
                            off_t indirect_off = inode.i_block[12] * block_size;
                            pread(fd, buffer, block_size, indirect_off);
                            block_index = *(int *)(buffer + (4 * (b - 12)));
                        }

                        // now that we have block number, copy that block into output file

                        offset = block_index * block_size;
                        pread(fd, buffer, block_size, offset);
                        fwrite(buffer, sizeof(char), block_size, output_file);
                    }
                    fclose(output_file);
                }
            }

            file_name_number++;
        }
    }
    printf("count is: %d\n", count);

    {
        // off_t inode_offset;
        // for (int i = 0; i < inode_total; i++)
        // {
        //     inode_offset = inode_table + (inode_size * i);
        //     lseek(fd, inode_offset, SEEK_SET);
        //     struct ext2_inode inode;
        //     read(fd, &inode, sizeof(inode));
        //     uint32_t block_index = inode.i_block[0];
        //     if (block_index != 0)
        //     {
        //         off_t offset = block_index * block_size;
        //         char buffer[block_size];
        //         pread(fd, buffer, block_size, offset);
        //         int is_jpg = 0;
        //         if (buffer[0] == (char)0xff &&
        //             buffer[1] == (char)0xd8 &&
        //             buffer[2] == (char)0xff &&
        //             (buffer[3] == (char)0xe0 ||
        //              buffer[3] == (char)0xe1 ||
        //              buffer[3] == (char)0xe8))
        //         {
        //             is_jpg = 1;
        //         }

        //         if (is_jpg == 1)
        //         {
        //             char output_filename[100];
        //             sprintf(output_filename, "output/file-%d.jpg", i + 1);
        //             FILE *output_file = fopen(output_filename, "w");
        //         }
        //     }
        // }
    }

    return 0;
}