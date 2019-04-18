#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

#include "inode.h"
#include "directory.h"
#include "pages.h"
#include "bitmap.h"
#include "util.h"

/*
typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
} inode;
*/
#define PAGE_SIZE 4096
#define PAGE_NUM 256
#define INUM_SIZE 76


void 
print_inode(inode* node) 
{
    printf("refs: %d\n", node->refs);
    printf("mode: %d\n", node->mode);
    printf("size: %d\n", node->size);
    printf("prt[0]: %d\n", node->ptrs[0]);
    printf("prt[1]: %d\n", node->ptrs[1]);
    printf("iptr: %d\n", node->iptr);
    printf("dir_count: %d\n", node->dir_count);
    printf("atime: %ld\n", node->atime.tv_sec);
    printf("mtime: %ld\n", node->mtime.tv_sec);
    printf("ctime: %ld\n", node->ctime.tv_sec);
} 

inode*
get_inode(int inum) 
{
    inode* b = get_inode_base();
    return b + inum;
}

int
alloc_inode()
{
    uint8_t* bm = get_inode_bitmap();
    for(int i = 0; i < 256; i++) {
        if(!bitmap_get(bm, i)) {
            bitmap_put(bm, i, 1);
            return i;
        }
    }
    //it's full
    return -1;
}

void free_inode(inode* node) 
{
    if (--node->refs) return;

    inode* base = get_inode_base();
    
    long inum = node - base;
    bitmap_put(get_inode_bitmap(), inum, 0);

    shrink_inode(node, 0);
}

int 
grow_inode(inode* node, int size)
{
    int size_in_pages = bytes_to_pages(size);
    int node_in_pages = bytes_to_pages(node->size);

    if (size < 0) {
        return -1; //invalid size
    }

    if (size_in_pages == node_in_pages) {
        node->size = size;
        return 0;
    }

    if (node->size >= size) {
        printf("How did you get here in grow_inode?\n");
        return 0;
    }

    if (size_in_pages > 2 && !node->iptr) {
        node->iptr = alloc_page();
    }

    for (int i = node_in_pages; i < size_in_pages; i++) {
        inode_set_pnum(node, i, alloc_page());
    }

    node->size = size;
    return 0;
}

int 
shrink_inode(inode* node, int size)
{
    int size_in_pages = bytes_to_pages(size);
    int node_in_pages = bytes_to_pages(node->size);

    if (size < 0) {
        return -1; //invalid size
    }

    if (size_in_pages == node_in_pages) {
        node->size = size;
        return 0;
    }

    if (node->size <= size) {
        printf("How did you get here in shrink?\n");
        return 0;
    }

    //free all the pages
    for (int i = node_in_pages - 1; i >= size_in_pages; i--) {
        free_page(inode_get_pnum(node, i)); 
    }

    // clean iptr  
    if (size_in_pages <= 2 && node->iptr) {
        free_page(node->iptr);
        node->iptr = 0;
    }

    //clean direct pointers
    if (size_in_pages < 2) {
        node->ptrs[1] = 0;
    }

    if (size_in_pages < 1) {
        node->ptrs[0] = 0;
    }

    node->size = size;
    return 0;
}

int 
inode_get_pnum(inode* node, int fpn)
{
    if (fpn < 2) {
        return node->ptrs[fpn];
    }
    int* ipnum = pages_get_page(node->iptr);
    return ipnum[fpn - 2];
}

void
inode_set_pnum(inode* node, int fpn, int val)
{
    if (fpn < 2) {
        node->ptrs[fpn] = val;
    }
    else {
        int* ipnum = pages_get_page(node->iptr);
        ipnum[fpn - 2] = val;
    }
}