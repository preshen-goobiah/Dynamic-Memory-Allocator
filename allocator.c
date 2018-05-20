// allocator.c
#include "allocator.h"
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <math.h>


/**
 NOTE TO MARKER: 
 Coding HW2 was collaborated on with Marc Karp (1356562)
 The following code does the required functions of hw2 but presented
 many problems with pointer arithmetic to counter this we worked around 
 alot of the problems. Please note when tracing code. Thanks. **/


void* custom_malloc(size_t size)
{
    if((double) size == 0)
    {
        return NULL; // requested size 0
    }
    
    
    // Add size of block and round up to next power of 2
    size = sizeof(struct block) + size;
    /*double exp = log2l((double) size);
    exp = ceil(exp);*/ // CANT USE MATH.H WITH MAKEFILE NEEDE -lm

    size_t return_size = 1; 
    while(return_size < size)
    {
        return_size = return_size << 1;
    }
    
    double request_size = (double) return_size;

    double block_size = MAX_SIZE;
    
    
    if(head == NULL)
    {
        //First malloc
        
        //First malloc will fit data to head (heads size is changed accordingly by splitting below) [1]
        
        head = sbrk(MAX_SIZE); // HEAP EXTENDED TO 1MiB and set start point to head


        head->size = MAX_SIZE;
        head->free = false;       //will decrease size of head below
        head->data =  (void*)head + (sizeof(struct block)); // data will be in memory addresses directly after block metadata
        
        // ONLY set data pointer once a malloc is called  NOT when splitting blocks
        
        
        struct block *split_block = head;
        
        for (int i = 0; i < MAX_EXP; i++) // initiliaze merge_buddy to NULL
        {
            head->merge_buddy[i] = NULL;
        }
        

        
        // split blocks -> decrease sizes going left initially
        
        while (block_size != request_size)
        {
            int count = 0;
            
            block_size = head->size/2;
            
            split_block = (void*) head + (int) block_size; // always split from head onwards
            split_block->size = block_size;
            split_block->next = head->next;
            split_block->buddy = (((void*) split_block)- ((int) block_size)); //set buddy to block of same size adjacent to block
            split_block->free = true;
            
            for (int i = 0; i < MAX_EXP; i++) // initiliaze merge_buddy to NULL
            {
                split_block->merge_buddy[i] = NULL;
            }
            


            split_block->merge_buddy[0] = head->buddy; // split blocks to the right have only one merge buddy block pointer
            
            
            while (head->merge_buddy[count] != NULL && count <=20)
            {
                count++;
            }
            
            head->merge_buddy[count] = split_block; // head must keep track of all merge buddies
            head->size = block_size;
            head->next = split_block;
            head->buddy = split_block;
            
        }
        
        // fix to delete extra element from merge buddy array
        int i = 0;
        while (head->merge_buddy[i] != NULL)
        {
            i++;
        }
        head->merge_buddy[i-1] = NULL;
        
        
        
        struct block *return_block = (void*)head + (sizeof(struct block)); // return block must point to where data can be added from ie. without block metadata
        
        
        return return_block;
    }
    else
    {
        //2nd malloc onwards
        struct  block *curr = head;
        struct block *return_block;
        while(curr)
        {
            
            // searching smallest blocks first
            if (curr->free == true && curr->size == request_size)
            {
                curr->free = false;
                curr->data = (void*)curr + (sizeof(struct block));
                return_block = (void*)curr + (sizeof(struct block));
                return return_block;
            }
            else if(curr->free == true && curr->size > request_size)
            {
                
                // split the first block we find thats bigger than requested size
                curr->free = false; //will decrease size of curr below
                curr->data = (void*)curr + (sizeof(struct block));
                
                struct block *split_block = curr;
                
                
                while (block_size != request_size)
                {
                    block_size = curr->size/2;
                    
                    split_block = (void*) curr + (int) block_size;
                    split_block->size = block_size;
                    split_block->next = curr->next;
                    split_block->buddy = (((void*) split_block)- ((int) block_size));
                    split_block->free = true;
                    
                    for (int i = 0; i < MAX_EXP; i++) // initiliaze merge_buddy to NULL
                    {
                        split_block->merge_buddy[i] = NULL;
                    }
                    
                    split_block->merge_buddy[0] = curr->buddy;
                    
                    int count = 0;
                    while (curr->merge_buddy[count] != NULL && count <=20) // current block must keep track of all split buddies despite changing size
                    {
                        count++;
                    }
                    
                    curr->merge_buddy[count] = split_block;
                    
                    curr->size = block_size;
                    curr->next = split_block;
                    curr->buddy = split_block;
                    
                    count++;
                    
                }
                
                //fix to delete last element from merge buddy array - above added final buddy for no reason
                int i = 0;
                while (curr->merge_buddy[i] != NULL)
                {
                    i++;
                }
                curr->merge_buddy[i-1] = NULL;
                
                return_block = (void*)curr + (sizeof(struct block));
                return return_block;
            }
            
            curr = curr->next;
        }
        if(curr == head && head->free == true)
        {
            return NULL; //requested size could not be fulfilled
        
        }
        
    }
    
}

/** Mark a data block as free and merge free buddy blocks **/
void custom_free(void* ptr)
{
    if (ptr == NULL)
    {
        //DO NOTHING
    }
    else
    {
        
        struct block *free_block = ptr - (int) sizeof(struct block);
        // should we check if block is valid is in list by traversing head?
        // what happens to the block metadata in each partition once we merge does it still exist?
        
        free_block->free = true;
        free_block->data = NULL;
        bool valid_block = true;
        
        while (free_block->size != MAX_SIZE && valid_block)
        {
            
            if (free_block->buddy == free_block->next)
            {
               
                //freeing a left child
                struct block *right_buddy = free_block->buddy;
                if ((right_buddy->size == free_block->size) && right_buddy->free == true)
                {
                    free_block->size = (free_block->size *2);
                    free_block->next = right_buddy->next;
                    free_block->buddy = right_buddy->merge_buddy[0];
                    int i = 0;
                    while (free_block->merge_buddy[i] != NULL)
                    {
                        i++;
                    }
                    
                    
                    //fix for seg fault at last merge
                   if (i-1 == 0)
                    {
                         free_block->merge_buddy[i] = NULL;
                    }
                    else
                    {
                         free_block->merge_buddy[i-1] = NULL;
                    }
                    
                }
                else
                {
                    valid_block = false;
                }
                
            }
            else
            {
                //freeing a right child
                struct block *left_buddy = free_block->buddy;
                if ((left_buddy->size == free_block->size) && left_buddy->free == true)
                {
                    left_buddy->size = (left_buddy->size *2);
                    left_buddy->next  = free_block->next;
                    
                    int i = 0;
                    while (left_buddy->merge_buddy[i] != NULL)
                    {
                        i++;
                    }
                    left_buddy->buddy = left_buddy->merge_buddy[i-1];
                    left_buddy->merge_buddy[i-1] = NULL;
                    free_block = left_buddy;
                    
                }
                else
                {
                    valid_block = false;
                }
            }
            
            
            //remove element at end
            if (free_block->size == MAX_SIZE)
            {
                free_block->merge_buddy[0] = NULL;
            }
            
        }
    
    }
    
}

/** Change the memory allocation of the data to have at least the requested size **/
void* custom_realloc(void* ptr, size_t size)
{
    
    if (ptr == NULL)
    {

        return NULL;

    }
    
    struct block *src = ptr - sizeof(struct block);
    
    if (src->size == size)
    {
        return ptr;
    }
    
    struct block *dest = (struct block*) custom_malloc(size);
    memcpy(dest, src, (size_t) src->size);
    custom_free(ptr);
    
    return (void*) dest + sizeof(struct block);
}

/*------------------------------------*\
 |            DEBUG FUNCTIONS           |
 \*------------------------------------*/

/** Prints the metadata of a block **/
void print_block(struct block* b) {
    if(!b) {
        printf("NULL block\n");
    }
    else {
        int i = 0;
        printf("Strt = %p\n",b);
        printf("Size = %lu\n",b->size);
        printf("Free = %s\n",(b->free)?"true":"false");
        printf("Data = %p\n",b->data);
        printf("Next = %p\n",b->next);
        printf("Buddy = %p\n",b->buddy);
        printf("Merge Buddies = ");
        while(b->merge_buddy[i] && i < MAX_EXP) {
            printf("%p, ",b->merge_buddy[i]);
            i++;
        }
        printf("\n\n");
    }
}

/** Prints the metadata of all blocks **/
void print_list() {
    struct block* curr = head;
    printf("--HEAP--\n");
    if(!head) printf("EMPTY\n");
    while(curr) {
        print_block(curr);
        curr = curr->next;
    }
    printf("--END--\n");
}
