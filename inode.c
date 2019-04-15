#include "stdint.h"

#include "inode.h"
#include "directory.h"
#include "pages.h"
#include "bitmap.h"

/*
typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
} inode;
*/

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
    
    //FIXME: probably wrong
    long inum = node - base;
    bitmap_put(get_inode_bitmap(), inum, 0);

    // TODO: does this work with symlinks? We don't want to overwrite 
    // the data after losing this inode
    // TODO: make this a shrink inode call eventually
    bitmap_put(get_pages_bitmap(), node->ptrs[0], 0);
    bitmap_put(get_pages_bitmap(), node->ptrs[1], 0);
}

int 
grow_inode(inode* node, int size)
{
    //TODO: Grow the inode in a smarter way
    // consult the size input and determine ptrs
    if (!node->ptrs[0]) {
        int pnum = alloc_page();
        node->ptrs[0] = pnum;
    }

    node->size = size;
    return 0;
}

int 
shrink_inode(inode* node, int size)
{
    return 0;
}

int 
inode_get_pnum(inode* node, int fpn)
{
    return node->ptrs[fpn];
}
