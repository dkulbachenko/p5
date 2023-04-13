#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct sort_args
{
    int *len;
    void ***subarr;
} sort_args;

typedef struct merge_args
{
    int *len1;
    int *len2;
    void ***subarr1;
    void ***subarr2;
} merge_args;

void *sort_worker(void *input)
{
    int len = ((sort_args *)input)->len;
    void **arr = *((sort_args *)input)->subarr;

    // *((sort_args *)input)->subarr =  sorted array
}

// might only be able to merge adjacent arrays?
void *merge_worker(void *input)
{
    merge_args *inp = ((merge_args *)input);

    int len1 = inp->len1;
    int len2 = inp->len2;
    void **arr1 = *inp->subarr1;
    void **arr2 = *inp->subarr2;

    void **ret = malloc(sizeof(void *) * (len1 + len2));
    if (ret == NULL)
    {
        printf("Failed to malloc in merge worker\n");
        exit(1);
    }

    // merge

    // this is definetely questionable
    *inp->subarr1 = ret;
    // even more questionable, probably free arr1 and arr2 instead
    // free(ret);
}

int main(char *argc, int argv)
{
    int filesize = 1000000;
    int numThreads = 12;

    pthread_t threads[numThreads];

    void **entries; // list of 100 byte entries from mmap
    int numEntries = filesize / 100;
    int extra = numEntries % numThreads;

    void *subarrays[numThreads];
    int subLengths[numThreads];
    int index = 0;
    int oldIndex = 0;

    // split large array and create threads
    for (int i = 0; i < numThreads; i++)
    {
        // assign start of each subarray
        subarrays[i] = entries + index;
        index += numEntries / numThreads;
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

    // Merge results
    int remainingArrs = numThreads;

    exit(0);
}