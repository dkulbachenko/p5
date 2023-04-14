#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

typedef struct sort_args
{
    int len;
    void ***subarr;
} sort_args;

typedef struct merge_args
{
    int len1;
    int len2;
    void ***subarr1;
    void ***subarr2;
} merge_args;

// merges the two input arrays
void **merge(void **arr1, void **arr2, int len1, int len2)
{
    void **ret = malloc(sizeof(void *) * (len1 + len2));
    if (ret == NULL)
    {
        printf("Failed malloc in merge\n");
    }

    // merge arr1 and arr 2
    int index = 0;
    // while both arrs have values
    while (len1 && len2)
    {
        // compare keys
        if (*(int *)(*arr1) < *(int *)(*arr2))
        {
            *(ret + index) = *arr1;
            arr1++;
            len1--;
        }
        else
        {
            *(ret + index) = *arr2;
            arr2++;
            len2--;
        }
        index++;
    }

    // add leftovers
    while (len1)
    {
        *(ret + index) = *arr1;
        arr1++;
        len1--;
        index++;
    }
    while (len2)
    {
        *(ret + index) = *arr2;
        arr1++;
        len2--;
        index++;
    }

    // free arr 1 and 2 here?

    return ret;
}

// returns arr in ascending order using merge sort
void **sort_helper(void **arr, int length)
{

    if (length == 1)
        return arr;

    int len1 = length / 2;
    int len2 = length - len1;
    // call recrusive
    void **arr1 = sort_helper(arr, len1);
    void **arr2 = sort_helper(arr + len1, len2);

    return merge(arr1, arr2, len1, len2);
}

void *sort_worker(void *input)
{
    int len = ((sort_args *)input)->len;
    void **arr = *(((sort_args *)input)->subarr);

    *(((sort_args *)input)->subarr) = sort_helper(arr, len);
    return NULL;
}

// might only be able to merge adjacent arrays?
void *merge_worker(void *input)
{
    merge_args *inp = ((merge_args *)input);

    int len1 = inp->len1;
    int len2 = inp->len2;
    void **arr1 = *inp->subarr1;
    void **arr2 = *inp->subarr2;

    // this is definetely questionable
    *inp->subarr1 = merge(arr1, arr2, len1, len2);
    return NULL;
}

int main(int argc, char **argv)
{
    int file_pointer;        // file pointer
    struct stat file_status; // file status
    if (argc != 4)
    {
        exit(1);
    }
    // char *input_name = argv[1];
    // char *output_name = argv[2];
    file_pointer = open(argv[1], O_RDONLY);
    fstat(file_pointer, &file_status);
    char *file_content = mmap(NULL, file_status.st_size, PROT_READ, MAP_PRIVATE, file_pointer, 0);

    // get number of entries and fill entries with proper values
    int num_entries = file_status.st_size / 100;
    void **entries = malloc(sizeof(void *) * num_entries);
    // every 100 bytes is the next entry
    for (int i = 0; i < num_entries; i++)
        entries[i] = file_content + (i * 100);
    int numThreads = atoi(argv[3]); // the third element
    // done processing arguments and file

    // begin actual main function
    pthread_t threads[numThreads];

    int extra = num_entries % numThreads;

    void **subarrays[numThreads];
    int subLengths[numThreads];
    int index = 0;
    int oldIndex = 0;

    // split large array and create threads
    for (int i = 0; i < numThreads; i++)
    {
        // assign start of each subarray
        subarrays[i] = entries + index;
        index += num_entries / numThreads;
        // add additional entries if it doesn't divide cleanly
        if (extra > 0)
        {
            index++;
            extra--;
        }
        // save length of this subarray
        subLengths[i] = index - oldIndex;

        // create a struct containing the length and pointer to subarr
        sort_args *args = (sort_args *)malloc(sizeof(sort_args));
        args->len = subLengths[i];
        args->subarr = &subarrays[i];

        pthread_create(&threads[i], NULL, sort_worker, (void *)args);

        oldIndex = index;
    }

    // wait for all threads to finish sorting
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // TODO: Merge results
    int remainingArrs = numThreads;
    while (remainingArrs > 1)
    {
        int toMerge = remainingArrs / 2;

        for (int i = 0; i < toMerge; i++)
        {
            // fill in args
            merge_args *args = (merge_args *)malloc(sizeof(merge_args));

            // find suitable subarray to merge with, start from end
            for (int j = numThreads - 1; j > 0; j--)
            {
                if (subLengths[j] != -1)
                {
                    args->subarr1 = &subarrays[i];
                    args->subarr2 = &subarrays[j];
                    args->len1 = subLengths[i];
                    args->len2 = subLengths[j];

                    // update size of sub1
                    subLengths[i] += subLengths[j];
                    // indicate that sub2 has been merged already
                    subLengths[j] = -1;
                    break;
                }
            }

            pthread_create(&threads[i], NULL, merge_worker, (void *)args);
        }

        for (int i = 0; i < toMerge; i++)
        {
            pthread_join(threads[i], NULL);
        }
        remainingArrs -= toMerge;
    }

    exit(0);
}