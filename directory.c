// based on cs3650 starter code

#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>

#include "slist.h"
#include "pages.h"
#include "inode.h"
#include "directory.h"
#include "util.h"
#include "bitmap.h"

/*
typedef struct dirent {
    char name[DIR_NAME];
    int  inum;
    char _reserved[12];
} dirent;
*/


static dirent *
directory_nth_dirent(inode* node, int n)
{
    assert(n < node->dir_count);
    int page_offset = n % 64;
    int page_num = n / 64; 
    int abs_pnum = inode_get_pnum(node, page_num);
    dirent* dir_page = pages_get_page(abs_pnum);
    return dir_page + page_offset;
}
static int 
is_dir(inode* node)
{
    return node->mode & 040000;
}

//returns inum
int 
directory_init()
{
    int inum = alloc_inode();
    inode* node = get_inode(inum);

	node->mode = 040755;
    node->refs = 0;
    node->size = 0;
    node->ptrs[0] = 0;
    node->ptrs[1] = 0;
    node->iptr = 0;
    node->dir_count = 0;
    grow_inode(node, 4096);
    return inum;
}

// returns inum
int 
directory_lookup(inode* dd, const char* name)
{
    if (!is_dir(dd)) return -ENOTDIR;
    for (int i = 0; i < dd->dir_count; i++) {
        dirent* cur_ent = directory_nth_dirent(dd, i);
        if (streq(name, cur_ent->name)) return cur_ent->inum;
    }

    return -ENOENT;
}

// returns inum
int 
tree_lookup(const char* path)
{ 
    int inum;
    inode* root_ptr = get_inode(0);
    if (streq(path, "/")) {
        return 0;
    }
    
    slist* path_list = s_split(path, '/');
    slist* cpy = path_list->next;

    inode* dir = root_ptr; 
    for (;cpy  ; cpy = cpy->next) {
        inum = directory_lookup(dir, cpy->data);
        if (inum < 0) {
            s_free(path_list);
            return inum;
        }
        dir = get_inode(inum);
    }
    s_free(path_list);
    return inum;
}

int 
directory_put(inode* dd, const char* name, int inum)
{
    // directory lookup the name to make sure it doesn't exist
    int in = directory_lookup(dd, name);

    if (in != -ENOENT) {
        return -EEXIST;
    }

    if (dd->dir_count % 64 == 0) {
        int rv = grow_inode(dd, dd->size + 4096);
        if (rv < 0) {
            return rv;
        }
    }
    dd->dir_count++;
    dirent* d_entry = directory_nth_dirent(dd,dd->dir_count - 1);
    d_entry->inum = inum;
    strncpy(d_entry->name, name, 48);
    inode* node = get_inode(inum);
    node->refs++;
    return 0;
}

slist*
directory_list(const char* path)
{
    int inum = tree_lookup(path);
    inode* dir = get_inode(inum);
    slist* list = NULL;

    for (int i = 0; i < dir->dir_count; i++) {
        dirent* cur_ent = directory_nth_dirent(dir, i);
        list = s_cons(cur_ent->name , list);
    }

    return list;
}

void 
print_directory(inode* dd)
{
    print_inode(dd);
}

int 
directory_delete(inode* dd, const char* name)
{
    //TODO: multi-page stuff needs to happen
    int inum, i;
    dirent* cur_ent;
    if (!is_dir(dd)) return -ENOTDIR;
    dirent* d_entry = pages_get_page(dd->ptrs[0]);

    for (i = 0; i < dd->dir_count; i++) {
        cur_ent = d_entry + i;
        if (streq(name, cur_ent->name)) {
            inum =  cur_ent->inum;
            goto found_ent; 
        }
    }
    return -ENOENT;

found_ent:

    if(inum < 0) {
        return inum;
    }
   
    free_inode(get_inode(inum));
    
    dirent* del_ent = cur_ent;
    dd->dir_count--;

    if (!dd->dir_count) {
        return 0;
    }

    // copy last entry and move it to the deleted entry
    dirent* page = pages_get_page(dd->ptrs[0]);
    dirent* last = page + dd->dir_count;
    memcpy(del_ent, last, sizeof(dirent));

    return 0;
}

int 
directory_rename(inode* dd, const char* name, const char* to)
{
    //TODO: multi-page stuff needs to happen
    int inum, i;
    dirent* cur_ent;
    if (!is_dir(dd)) return -ENOTDIR;
    dirent* d_entry = pages_get_page(dd->ptrs[0]);

    for (i = 0; i < dd->dir_count; i++) {
        cur_ent = d_entry + i;
        if (streq(name, cur_ent->name)) {
            inum =  cur_ent->inum;
            goto found_ent; 
        }
    }
    return -ENOENT;

found_ent:

    if(inum < 0) {
        return inum;
    }

    dirent* old_ent = cur_ent;
    strncpy(old_ent->name, to, 48);

    return 0;
}
