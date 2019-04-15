// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include <time.h>

#include "pages.h"

// each pointer is 8 bytes, each int 4, each timespec 16 bytes
// full inode size is 76 bytes
typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
    int dir_count; // the count of entries in a this
    struct timespec atime; // last access timestamp
    struct timespec mtime; // last modification timestamp
    struct timespec ctime; // last status change timestamp
} inode;

void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode();
void free_inode(inode* node);
int grow_inode(inode* node, int size);
int shrink_inode(inode* node, int size);
int inode_get_pnum(inode* node, int fpn);

#endif
