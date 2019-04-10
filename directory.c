// based on cs3650 starter code

#include <errno.h>

#include "slist.h"
#include "pages.h"
#include "inode.h"
#include "directory.h"
#include "util.h"

/*
typedef struct dirent {
    char name[DIR_NAME];
    int  inum;
    char _reserved[12];
} dirent;
*/

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
    // TODO: grow the inode maybe
    grow_inode(node, 4096);
    return inum;
}

// returns inum
int 
directory_lookup(inode* dd, const char* name)
{
    if (!is_dir(dd)) return -ENOTDIR;
    dirent* d_entry = pages_get_page(dd->ptrs[0]);


    for (int i = 0; i < dd->dir_count; i++) {
        dirent* cur_ent = d_entry + i;
        if (streq(name, cur_ent->name))
            return cur_ent->inum;
    }

    printf("in dir lookup\n");
    return -ENOENT;
}

// returns inum
int 
tree_lookup(const char* path)
{
    if (streq(path, "/")) {
        return 0;
    }

    //TODO: Assuming root
    return directory_lookup(get_inode(0), path + 1);
}

int 
directory_put(inode* dd, const char* name, int inum)
{
    //TODO: EEXIST if there is a file that exists with the same "name"
    dirent* d_entry = (dirent *)pages_get_page(dd->ptrs[0]) + dd->dir_count;
    d_entry->inum = inum;
    strncpy(d_entry->name, name, 48);
    dd->dir_count++;
    inode* node = get_inode(inum);
    node->refs++;
    return 0;
}

slist*
directory_list(const char* path)
{
    // TODO: actually use path, Assuming root
    inode* dir = get_inode(0);
    slist* list = 0;

    dirent* d_entry = pages_get_page(dir->ptrs[0]);

    for (int i = 0; i < dir->dir_count; i++) {
        dirent* cur_ent = d_entry + i;
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

    dirent* del_ent = cur_ent;
    free_inode(get_inode(inum));
    dd->dir_count--;

    if (!dd->dir_count) {
        return 0;
    }

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