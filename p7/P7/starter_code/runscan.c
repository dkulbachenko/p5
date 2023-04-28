#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <errno.h>

#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BLOCK_SIZE 1024
#define MAX_FILENAME_LEN 256
#define MAX_BUFFER_SIZE 8192 // ulimit -s

// if it is a jpg file
int is_jpg(char buffer[BLOCK_SIZE])
{
    return (buffer[0] == (char)0xff &&
            buffer[1] == (char)0xd8 &&
            buffer[2] == (char)0xff &&
            (buffer[3] == (char)0xe0 ||
             buffer[3] == (char)0xe1 ||
             buffer[3] == (char)0xe8));
}

unsigned int inode_blocksize(int block_index)
{
    if (block_index < EXT2_NDIR_BLOCKS)
    {
        return 1024;
    }

    block_index -= EXT2_NDIR_BLOCKS;
    if (block_index < 256)
    {
        return 1024;
    }

    block_index -= 256;
    if (block_index < 256 * 256)
    {
        return 1024 * 256;
    }

    block_index -= 256 * 256;
    return 1024 * 256 * 256;
}

void copy_file(const char *from, const char *to) 
{
    //printf("reached 111");
    int src_fd = open(from, O_RDONLY);
    if (src_fd == -1)
    {
        perror("Error opening source file");
        exit(1);
    }

    int dst_fd = open(to, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (dst_fd == -1)
    {
        perror("Error opening destination file");
        exit(1);
    }

    char buffer[MAX_BUFFER_SIZE];
    ssize_t num_read;
    while ((num_read = read(src_fd, buffer, MAX_BUFFER_SIZE)) > 0)
    {
        if (write(dst_fd, buffer, num_read) != num_read)
        {
            perror("Error writing to destination file");
            exit(1);
        }
    }
    if (num_read == -1)
    {
        perror("Error reading from source file");
        exit(1);
    }

    if (close(src_fd) == -1)
    {
        perror("Error closing source file");
        exit(1);
    }

    if (close(dst_fd) == -1)
    {
        perror("Error closing destination file");
        exit(1);
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
    struct ext2_group_desc *group = malloc(sizeof(struct ext2_group_desc)*num_groups); // previously "struct ext2_group_desc group"

    // example read first the super-block and group-descriptor
    read_super_block(fd, &super);

    //read_group_descs(fd, &group, 1); 
    read_group_descs(fd, group, num_groups); // line 116
    // create output directory
    char *output_dir = argv[2];
    DIR *dir = opendir(output_dir);
    if (dir != NULL)
    {
        printf("Error: output directory already exists.\n");
        closedir(dir);
        exit(0);
    }
    else
    {
        mkdir(output_dir, 0700);
    }

    // part1
    int block_size = 1024 << super.s_log_block_size;
    char buffer[block_size];

    int num_groups = super.s_blocks_count / super.s_blocks_per_group;
    if (super.s_blocks_count % super.s_blocks_per_group > 0)
    {
        num_groups++;
    }

   // int inode_no = 0;

    int imgs[1024]; // target inode
    int imgs_num = 0;


    int i;
    for (i = 0; i < num_groups; i++)
    {
        int inode_table_offset = locate_inode_table(i, group); 
        // iterate the inode
        for (unsigned int j = 1; j <= super.s_inodes_per_group; j++)
        {


            // inode_id = 0;
            struct ext2_inode inode;
            read_inode(fd, inode_table_offset, j, &inode, super.s_inode_size);

            // check the inode represents file or dir

            if (S_ISREG(inode.i_mode))
            {

                lseek(fd, BLOCK_OFFSET(inode.i_block[0]), SEEK_SET);
                read(fd, buffer, block_size);


                // Check if it is a jpg file
            // buffer[0] == (char)0xff &&
            // buffer[1] == (char)0xd8 &&
            // buffer[2] == (char)0xff &&
            // (buffer[3] == (char)0xe0 ||
            //  buffer[3] == (char)0xe1 ||
            //  buffer[3] == (char)0xe8));
                if (is_jpg(buffer))
                {
                    imgs[imgs_num] = i * super.s_inodes_per_group + j; 
                    imgs_num++;

                    char filename[1028];
                    snprintf(filename, 1028, "%s/file-%d.JPG", output_dir, i * super.s_inodes_per_group + j); 
                    FILE *output_file = fopen(filename, "w");
                    if (output_file == NULL)
                    {
                        perror("fopen");
                        exit(EXIT_FAILURE);
                    }


                    // information stored in detail file
                    int links_count = inode.i_links_count;
                    int file_size = inode.i_size;
                    int user_id = inode.i_uid;

                   
                    char details_file_name[1024]; // copy one byte at one time
                    snprintf(details_file_name, 1024, "%s/file-%d-details.txt", output_dir, i * super.s_inodes_per_group + j); // line 178

                 
                    FILE *details_file = fopen(details_file_name, "w");
                    if (details_file == NULL)
                    {
                        perror("fopen");
                        exit(EXIT_FAILURE);
                    }

                    for (int b = 0; b < EXT2_N_BLOCKS; b++)
                    {
                        unsigned int indirect_buffer[256];
                        if(b < 12){
                            // Read data block
                            lseek(fd, BLOCK_OFFSET(inode.i_block[b]), SEEK_SET);
                            read(fd, buffer, block_size);
                            // if (is_jpg(buffer))
                            {

                                if (inode.i_size > 1024) // 1 byte
                                {
                                    fwrite(buffer, 1, 1024, output_file);
                                    inode.i_size -= 1024;
                                }
                                else
                                {
                                    fwrite(buffer, 1, inode.i_size, output_file);
                                    inode.i_size = 0;
                                    break;
                                }
                            }
                        }
                        if (b >= 12) // indirect
                        {
     
                            unsigned int block_size = inode_blocksize(b);
                            lseek(fd, BLOCK_OFFSET(inode.i_block[b]), SEEK_SET);
                            read(fd, indirect_buffer, 1024);

                            if (b == EXT2_IND_BLOCK) // 12
                            {
                                char transfer_buffer[1024];
                                for (int z = 0; z < 256; z++)
                                {
                                    lseek(fd, BLOCK_OFFSET(indirect_buffer[z]), SEEK_SET);
                                    read(fd, transfer_buffer, 1024);
                                    // if (is_jpg(IND_buffer))
                                    {
                                        // write to file
                                        if (inode.i_size > 1024)
                                        {
                                            fwrite(transfer_buffer, 1, 1024, output_file);
                                            inode.i_size -= 1024;
                                        }
                                        else
                                        {
                                            fwrite(transfer_buffer, 1, inode.i_size, output_file);
                                            inode.i_size = 0;
                                            break;
                                        }
                                    }
                                }
                            }

                            else if (b == EXT2_DIND_BLOCK) 
                            {
                                unsigned indirect_indirect_buffer[256];
                                for (int c = 0; c < 256; c++)
                                {
                                    lseek(fd, BLOCK_OFFSET(indirect_buffer[c]), SEEK_SET);
                                    read(fd, indirect_indirect_buffer, 1024);

                                    char transfer_buffer[1024];
                                    for (int x = 0; x < 256; x++)
                                    {
                                        lseek(fd, BLOCK_OFFSET((unsigned int)indirect_indirect_buffer[x]), SEEK_SET);
                                        read(fd, transfer_buffer, 1024);
                                        {
                                            if (inode.i_size > 1024)
                                            {
                                                fwrite(transfer_buffer, 1, 1024, output_file);
                                                inode.i_size -= 1024;
                                            }
                                            else
                                            {
                                                fwrite(transfer_buffer, 1, inode.i_size, output_file);
                                                inode.i_size = 0;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    

                    }
                    fclose(output_file);
                 
                    fprintf(details_file, "%d\n", links_count);
                    fprintf(details_file, "%d\n", file_size);
                    fprintf(details_file, "%d", user_id);
                    fclose(details_file);
                }
            }
        }
    }

    ////////// part 2

    int inodes_per_block = block_size / sizeof(struct ext2_inode); 

    off_t start_inode_table; 
    int inode_num = 0;

    for (int n = 0; n < num_groups; n++)
    {
        for (int i = 1; i <= inodes_per_block; i++)
        {

            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            start_inode_table = locate_inode_table(0, group);   
            read_inode(fd, start_inode_table, inode_num, inode, super.s_inode_size);
            if (!S_ISDIR(inode->i_mode))
            {
                free(inode);
                inode_num++;
                continue;
            }
            char dir_buffer[1024];
            lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET); 
            read(fd, dir_buffer, 1024);

            int start = 0;
            while (start < 1024)
            {
                struct ext2_dir_entry *entry = (struct ext2_dir_entry *)&(dir_buffer[start]);
                if (entry->inode == 0 || entry->rec_len == 0 || entry->name_len == 0)
                { 
                    break;
                }

                int is_jpg = 0;
                for (int j = 0; j < imgs_num; j++)
                {
                    if ((int)entry->inode == imgs[j])
                    {
                        is_jpg = 1;
                        break;
                    }
                }

                if (is_jpg == 1)
                {
                    char old_file_name[MAX_FILENAME_LEN] = "";
                    char second_file_name[MAX_FILENAME_LEN] = "";
                    snprintf(old_file_name, 256, "%s/file-%d.JPG", output_dir, entry->inode);
                     char name[EXT2_NAME_LEN];
                     int name_len = entry->name_len & 0xFF;
                     strncpy(name, entry->name, name_len);
                     name[name_len] = '\0';
                    
                    snprintf(second_file_name, 256, "%s/%s", output_dir, name);
     

                    copy_file(old_file_name, second_file_name);
                }

                int round = entry->name_len;
                if (entry->name_len % 4 != 0)
                {
                    int extra = 4 - (entry->name_len % 4);
                    round = entry->name_len + extra;
                }

                start = start + 8 + round;
            }
            free(inode);
            inode_num++;
        }
    }
    close(fd);

    return 0;
}
