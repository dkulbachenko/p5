#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

ext2_group_desc *groups;
int imgs[100];
int num_images = 0;
char* dir_name;
// helper method read the data into the buffer
void read_block(int fd, char* buffer, uint32_t block_num, int block_size){
    off_t offset = block_num * block_size;
    pread(fd, buffer, block_size, offset);
}


void read_directory_data_blocks(int fd, struct ext2_super_block super)
{
    off_t inode_starter = locate_inode_table(0, groups);
    struct ext2_inode root;
    read_inode(fd, inode_starter, EXT2_ROOT_INO, &root, super.s_inode_size);
    uint32_t block_index = root.i_block[0];

    struct ext2_dir_entry_2 dir_entry;
    int offset = 0;
    int block_size = super.s_log_block_size;
    while(block_index != 0){
        char block_buffer[block_size];
        read_block(fd, block_buffer, block_index, block_size);

        while(offset < block_size){
            // all entries in block_buffer, block_buffer + offset represent each entry
            memcpy(&dir_entry, block_buffer + offset, sizeof(struct ext2_dir_entry_2));
            offset += dir_entry.rec_len;
            int name_len = dir_entry.name_len & 0xFF;
            char name [EXT2_NAME_LEN];
            strncpy(name, dir_entry.name, name_len);
            name[name_len] = '\0';
            if (dir_entry.file_type == 2){
                
            }else{
                for (int i = 0; i < num_images; i ++){
                    if (dir_entry.inode == imgs[i]){
                        char file_read_from[100];
                        sprintf(file_read_from, "%s/file-%d.jpg", dir_name, imgs[i]);
                        char output_file_name[100];
                        sprintf(output_file_name, "%s/%s", dir_name, dir_entry.name);
                        FILE* input_file = fopen(file_read_from, "r");
                        FILE * output_file = fopen(output_file_name, "w");
                        char data[100];
                        fscanf(input_file, "%s", data);
                        fprintf(output_file, "%s", data);
                        fclose(input_file);
                        fclose(output_file);
                        // int group_id = (dir_entry.inode + dir_entry.inode % super.s_inodes_per_group) / (super.s_inodes_per_group);
                        // off_t inode_starter = locate_inode_table(group_id, groups);
                        // int offset = dir_entry.inode % super.s_inodes_per_group;
                        // struct ext2_inode inode;
                        // read_inode(fd, inode_starter, offset + 1, &inode, super.s_inode_size);
                    
                        
                        
                    }
                }
            }

        }

    }
    void read_subdirectory(int fd, struct ext2_super_block super, struct ext2_inode inode)


//     for (int i = 0; i < EXT2_N_BLOCKS; i++) // ext2_n_block is max number of block in inode
//     {
//         if (inode.i_block[i] != 0)
//         {
//             int offset = 0;

//             while (offset < block_size && num_blocks > 0)
//             {
//                 char buffer[block_size];

//                 int read_size = block_size;
//                 pread(fd, buffer, read_size, inode.i_block[i] * block_size + offset);
//                 struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)buffer;
//                 while ((char *)entry < buffer + read_size)
//                 {
//                     if (entry->file_type == 2)
//                     {

//                         // recursive call
//                     }
//                     else
//                     {
//                         // array of file names
//                         char *names = entry->name;
//                     }

//                     // perhaps get the name from the entry
//                 }
//             }
//         }
//     }
// }

// void write_indirect(int fd, int block_num, FILE *output_file)
// {
//     for (int i = 0; i < 256; i++)
//     {
//         char buffer[block_size];
//         off_t indirect_off = block_num * block_size;
//         pread(fd, buffer, block_size, indirect_off);
//         uint32_t block_index = *(int *)(buffer + (4 * (i)));
//         if (block_index == 0)
//             continue;
//         off_t offset = block_index * block_size;
//         pread(fd, buffer, block_size, offset);
//         fwrite(buffer, sizeof(char), block_size, output_file);
//     }
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
    // printf("num of groups in line 29: %d\n", num_groups);
    // printf("num of blocks in line 30: %d\n", super.s_blocks_count);
    // printf("num of blocks per group in line 31: %d\n", blocks_per_group);

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
    dir_name = argv[2];
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
    // printf("new file name: %s", "test");

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

                    // add file_name_num to array
                    imgs[num_images] = file_name_number;
                    // update amount of images
                    num_images++;

                    for (int b = 0; b < 15; b++)
                    {
                        if (b <= 11)
                        {
                            block_index = inode.i_block[b];
                            if (block_index == 0)
                                continue;
                            printf("b: %d\n", b);
                            printf("block index: %d\n", block_index);
                            offset = block_index * block_size;
                            pread(fd, buffer, block_size, offset);
                            fwrite(buffer, sizeof(char), block_size, output_file);
                        }
                        else if (b == 12)
                        {
                            // single indirect
                            
                            write_indirect(fd, inode.i_block[b], output_file);
                        }
                        else if (b == 13)
                        {
                            
                            // double indirect
                            int block_num = inode.i_block[b];
                            off_t indirect_off = block_num * block_size;
                            pread(fd, buffer, block_size, indirect_off);
                            // int c = 0;
                            // while (*(int *)(buffer + (4 * (c))) != 0)
                            for (int c = 0; c < 256; c++)
                            {
                                block_num = *(int *)(buffer + (4 * (c)));
                                if (block_num != 0)
                                    write_indirect(fd, block_num, output_file);
                            }
                        }
                        else
                        {
                            int block_num = inode.i_block[b];
                            if (block_num != 0)
                            { // need to triple indirect
                                printf("didn't think triple indirect would be necessary\n");
                            }
                        }
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