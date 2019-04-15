#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fuse.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>

#include "storage.h"
#include "inode.h"
#include "directory.h"
#include "bitmap.h"
#include "util.h"
#include "pages.h"

const int BLOCK_SIZE = 4096;

//Don't think this is right
void 
storage_init(const char* path)
{
    pages_init(path);
    directory_init();
}

int
storage_stat(const char* path, struct stat* st)
{
    int inum = tree_lookup(path);

    if (inum < 0) return inum;
    
    inode* node = get_inode(inum);

    st->st_mode = node->mode;
	st->st_dev = 0;
	st->st_ino = inum;
	st->st_nlink = node->refs;
	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_size = node->size;

    return 0;
}

int    
storage_readdir(const char *path, void *buf, fuse_fill_dir_t filler)
{
    struct stat st;
    int rv;
 
    rv = storage_stat("/", &st);
    assert(rv == 0);

    rv = filler(buf, ".", &st, 0);
    assert(rv == 0);

    slist* root_list = directory_list(path);
    slist* rlcpy = root_list;

    for(; rlcpy != NULL; rlcpy = rlcpy->next) {
        filler(buf, rlcpy->data, &st, 0);
    }
    
    s_free(root_list);

    return 0;
}

int    
storage_mknod(const char* path, int mode)
{
    int rv = 0;
    int inum = alloc_inode();
    inode* node = get_inode(inum);

    if(node < 0) {
        return -EDQUOT;
    }
    node->mode = mode;

    //add to dir_count in parent inode
    char* dn = strdup(path);
    char* bn = strdup(path);

    char* drn = dirname(dn);
    char* brn = basename(bn);
    
    rv = tree_lookup(drn);
    if (rv < 0) { 
        free(dn);
        free(bn);
        return rv;
    }

    inode* dir_inode = get_inode(rv);

    rv = directory_put(dir_inode, brn, inum);

    free(dn);
    free(bn);
    return rv;
}

int
storage_truncate(const char *path, off_t size)
{
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }
    //TODO: Implement Multi-page support
    inode* node = get_inode(inum);

    grow_inode(node, size);
    node->size = size;
    return 0;
}

int   
storage_read(const char* path, char* buf, size_t size, off_t offset) 
{
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    inode* node = get_inode(inum);
    char* page = pages_get_page(node->ptrs[0]);

    //TODO: ASSUMING LESS THAN 4k
    // depending on the size, you would need to get
    // multiple pages from the inode to read
    memcpy(buf, page + offset, size);

    return size;
}

int   
storage_write(const char* path, const char* buf, size_t size, off_t offset) 
{
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    storage_truncate(path, size);
    inode* node = get_inode(inum);
    char* page = pages_get_page(node->ptrs[0]);

    //TODO: ASSUMING LESS THAN 4k
    memcpy(page + offset, buf, size);
    return size;
}

int
storage_unlink(const char* path)
{ 
    int rv = 0;

    //add to dir_count in parent inode
    char* dn = strdup(path);
    char* bn = strdup(path);

    char* drn = dirname(dn);
    char* brn = basename(bn);

    rv = tree_lookup(drn);

    if (rv < 0) {
        free(dn);
        free(bn);
        return rv;
    }

    inode* dir_inode = get_inode(rv);

    rv = directory_delete(dir_inode, brn);

    free(dn);
    free(bn);
    return rv;
}

int 
storage_link(const char* from, const char* to) 
{ 
    int from_inum = tree_lookup(from); 
    if (from_inum == -ENOENT) {
         // "from" does not exist -> cannot link
         return from_inum;   
    }  

    char* dn = strdup(to);
    char* bn = strdup(to);

    char* drn = dirname(dn);
    char* brn = basename(bn);

    int to_inum = tree_lookup(drn);
    inode* to_dir = get_inode(to_inum);

    return directory_put(to_dir, brn, from_inum);
}

int
storage_rename(const char *from, const char *to)
{
    int rv = 0;
    
    char* dn = strdup(from);
    char* drn = dirname(dn);

    char* bn = strdup(from);
    char* brn = basename(bn);

    rv = tree_lookup(drn);

    if (rv < 0) {
        free(dn);
        free(bn);
        //printf("in storage_rename\n");
        return rv;
    }

    char* tobn = strdup(to);
    char* tobrn = basename(tobn);

    inode* dir_inode = get_inode(rv);

    rv = directory_rename(dir_inode, brn, tobrn);

    free(dn);
    free(bn);
    free(tobn);
    return rv;
}

int storage_symlink(const char* file, const char* link) {
    int file_inum = tree_lookup(file);
    if (file_inum < 0) return file_inum;
    
    // make the symbolic link
    int rv = storage_mknod(link, 012755); // are these the right permissions?
    if (rv < 0) return rv;
    
    int link_inum = tree_lookup(link);
    if (link_inum < 0) return link_inum; // this should never happen

    inode* link_inode = get_inode(link_inum);
    inode* file_inode = get_inode(file_inum);
    // copy data ptrs
    // FIXME: this data could be overwritten if we remove the original inode
    link_inode->ptrs[0] = file_inode->ptrs[0];
    link_inode->ptrs[1] = file_inode->ptrs[1];
    link_inode->iptr = file_inode->iptr;
    // FIXME: fuse is getting an io error for some reason
    return 0;
}

//TODO
// reads the contents of a symbolic link
int storage_readlink(const char* restrict path, char* restrict buf, size_t bufsize) {
    
    // how is this different than reading normally?
    int rv = storage_read(path, buf, bufsize, 0);

    // returns the size of the buf written
    return rv;

    /*
    FROM STORAGE_READ 
        int inum = tree_lookup(path);
        if (inum < 0) {
            return inum;
        }

        inode* node = get_inode(inum);
        char* page = pages_get_page(node->ptrs[0]);

        //TODO: ASSUMING LESS THAN 4k
        // depending on the size, you would need to get
        // multiple pages from the inode to read
        memcpy(buf, page + offset, size);

        return size;
    */
}
