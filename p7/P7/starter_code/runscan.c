#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#define BLOCK_SIZE 1024
#define MAX_FILENAME_LEN 256
#define MAX_BUFFER_SIZE 8192 // ulimit -s
ext2_group_desc *groups;
int imgs[1024];
int num_images = 0;
char* dir_name;
unsigned int block_size_for_inode(int block_index)
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

// helper method read the data into the buffer
void copy_file(const char *src_file_path, const char *dst_file_path) // copy file from src to dst
{
    //printf("reached 111");
    int src_fd = open(src_file_path, O_RDONLY);
    if (src_fd == -1)
    {
        perror("Error opening source file");
        exit(1);
    }

    int dst_fd = open(dst_file_path, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
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
    groups = malloc(sizeof(ext2_group_desc) * num_groups);
    read_super_block(fd, &super);
    read_group_descs(fd, groups, num_groups);

    // int num_groups = super.s_blocks_count / super.s_blocks_per_group;
   // int num_groups = (super.s_blocks_count + blocks_per_group - 1) / blocks_per_group; // this was in other starter file
    int num_groups = super.s_blocks_count / super.s_blocks_per_group;
    if (super.s_blocks_count % super.s_blocks_per_group > 0)
    {
        num_groups++;
    }

    if (groups == NULL)
    {
        printf("groups malloc failed\n");
        exit(1);
    }

    // example read first the super-block and group-descriptor

    // int inode_size = super.s_inode_size; // inode size
    // int inode_total = super.s_inodes_count; // total number of inodes in the file system.
    off_t inode_starter;

    // if the dir does not exist then create it
    // char output_dir[] = "output";

    struct stat st = {0};
    if (stat(argv[2], &st) == -1)
        mkdir(argv[2], 0700);
    dir_name = argv[2];
    //int file_name_number = 1;
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

        for (int j = 1; j <= (int)super.s_inodes_per_group; j++)
        {
            // inode_offset = inode_table + (inode_size * j);
            struct ext2_inode inode;
            read_inode(fd, inode_starter, j , &inode, super.s_inode_size);

            uint32_t block_index = inode.i_block[0];
            // if (block_index != 0)4
            if (S_ISREG(inode.i_mode)) // check if it is a regular file
            {
                lseek(fd, BLOCK_OFFSET(inode.i_block[0]), SEEK_SET);                // printf("hello world\n");
               // off_t offset = block_index * block_size;
                char buffer[block_size];
                read(fd, buffer, block_size);
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
                    imgs[num_images] = i * super.s_inodes_per_group + j;
                    // update amount of images
                    num_images++;
                    FILE *output_file;
                    printf("found jpg with inode num %d\n", i * super.s_inodes_per_group + j);
                    count++;
                    char output_filename[100];
                    sprintf(output_filename, "%s/file-%d.jpg",dir_name , i * super.s_inodes_per_group + j);
                    // file_name_number++;
                    output_file = fopen(output_filename, "w");
                    if (output_file == NULL)
                        printf("the output file is null\n");

                    int links_count = inode.i_links_count;
                    int file_size = inode.i_size;
                    int owner_user_id = inode.i_uid;

                    // create the file name for the details
                    char detail_file_name[1024];
                    sprintf(detail_file_name, "%s/file-%d-details.txt", dir_name,  i * super.s_inodes_per_group + j);
                    FILE *details_file = fopen(detail_file_name, "w");
                    if (details_file == NULL)
                    {
                        perror("fopen");
                        exit(EXIT_FAILURE);
                    }



                    printf("output file name is: %s\n", output_filename);
                    printf("file size is: %d bytes\n", file_size);
                    int blocks_required = (file_size + file_size % block_size) / block_size;
                    printf("blocks required: %d\n", blocks_required);

                    // add file_name_num to array


                    for (int b = 0; b < 15; b++)
                    {
                        unsigned int add_buffer[256];
                        if (b <= 11)
                        {
                            block_index = inode.i_block[b];
                            lseek(fd, BLOCK_OFFSET(block_index), SEEK_SET);
                            read(fd, buffer, block_size);
                            if (block_index == 0)
                                continue;
                            printf("b: %d\n", b);
                            printf("block index: %d\n", block_index);
                            //offset = block_index * block_size;
                            fwrite(buffer, sizeof(char), block_size, output_file);
                        }
                        else{
                            unsigned int block_size = block_size_for_inode(b);
                            lseek(fd, BLOCK_OFFSET(inode.i_block[b]), SEEK_SET);
                            read(fd, add_buffer, 1024);
                        
                         if (b == 12)
                        {
                            char indir_buffer[1024];
                            for (int k = 0; k < 256; k ++)
                            {
                                lseek(fd, BLOCK_OFFSET(add_buffer[k]), SEEK_SET);
                                read(fd, indir_buffer, block_size);
                                //fwrite(indir_buffer, sizeof(char), inode.i_size, output_file);
                                if (inode.i_size > 1024){
                                    fwrite(indir_buffer, 1, block_size, output_file);
                                    inode.i_size -= 1024;
                                }
                                else{
                                    fwrite(indir_buffer, 1, inode.i_size, output_file);
                                    inode.i_size = 0;
                                    break;
                                }


                            }

                            // single indirect
                            
                            //write_indirect(fd, inode.i_block[b], output_file);
                        }
                        else if (b == 13)
                        {
                            
                            // double indirect

                            unsigned indir_add_buffer[256];
                            for (int y = 0; y < 256; y ++){
                                lseek(fd, BLOCK_OFFSET(add_buffer[y]), SEEK_SET);
                                read(fd, indir_add_buffer, block_size);
                                char ind_buffer[block_size];
                                for (int x = 0; x < 256; x ++){
                                    lseek(fd, BLOCK_OFFSET((unsigned int) indir_add_buffer[x]), SEEK_SET);
                                    read(fd, ind_buffer, block_size);
                                            if (inode.i_size > 1024)
                                            {
                                                fwrite(ind_buffer, 1, 1024, output_file);
                                                inode.i_size -= 1024;
                                            }
                                            else
                                            {
                                                fwrite(ind_buffer, 1, inode.i_size, output_file);
                                                inode.i_size = 0;
                                                break;
                                            }                                
                                        }
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
                    }
                    fclose(output_file);
                    fprintf(details_file, "%d\n", links_count);
                    fprintf(details_file, "%d\n", file_size);
                    fprintf(details_file, "%d", owner_user_id);
                    fclose(details_file);
                }
            }

            //file_name_number++;
            
        }
    }
    printf("count is: %d\n", count);



    /// part2
    int new_fd = open(argv[1], O_RDONLY);
    ext2_read_init(new_fd);
    struct ext2_super_block new_super;
    struct ext2_group_desc new_group; //?????

    read_super_block(new_fd, &new_super);
    read_group_descs(fd, &new_group, num_groups);
    int inodes_per_block = block_size / sizeof(struct ext2_inode); // number of inodes/block

    off_t start_inode_table; // start of inode table for this group
    int new_inode_no = 0;
        int group_count = super.s_blocks_count / super.s_blocks_per_group;
     for (int n = 0; n < group_count; n++)
    {
        // printf("inode/block:%d\n", inodes_per_block);
        for (int i = 1; i <= inodes_per_block; i++)
        {

            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            // read_inode(new_fd, start_inode_table + (new_inode_no - 1) * super.s_inode_size, new_inode_no, inode, super.s_inode_size);
            start_inode_table = locate_inode_table(0, &new_group);   // ?????????
            read_inode(new_fd, start_inode_table, new_inode_no, inode, super.s_inode_size);
            // printf("A");
            if (!S_ISDIR(inode->i_mode))
            {
                free(inode);
                new_inode_no++;
                continue;
            }
            char dir_buffer[1024];
            lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET); // block_count * 1024 +
            read(fd, dir_buffer, 1024);

            int start = 0;
            while (start < 1024)
            {
                struct ext2_dir_entry *dentry = (struct ext2_dir_entry *)&(dir_buffer[start]);
                if (dentry->inode == 0 || dentry->rec_len == 0 || dentry->name_len == 0)
                { // rec_len, Directory entry length
                    break;
                }

                int is_jpg = 0;
                for (int j = 0; j < num_images; j++)
                {
                    if ((int)dentry->inode == imgs[j])
                    {
                        is_jpg = 1;
                        //printf("jpginode: %d\n", jpg_inodes[j]);
                        break;
                    }
                }

                if (is_jpg == 1)
                {
                    char old_file_name[MAX_FILENAME_LEN] = "";
                    char second_file_name[MAX_FILENAME_LEN] = "";
                    snprintf(old_file_name, 256, "%s/file-%d.jpg", dir_name, dentry->inode);

                     char name[EXT2_NAME_LEN];
                     int name_len = dentry->name_len & 0xFF;
                     strncpy(name, dentry->name, name_len);
                     name[name_len] = '\0';
                    // strcat(second_file_name, name);
                    snprintf(second_file_name, 256, "%s/%s", dir_name, name);
                    //printf("new file name: %s\n", name);

                    copy_file(old_file_name, second_file_name);
                }

                int name_len_rounded = dentry->name_len;
                if (dentry->name_len % 4 != 0)
                {
                    int extra = 4 - (dentry->name_len % 4);
                    name_len_rounded = dentry->name_len + extra;
                }

                start = start + 8 + name_len_rounded;
            }
            free(inode);
            new_inode_no++;
        }
    }
    close(new_fd);
    return 0;
}