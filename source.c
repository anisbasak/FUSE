#define FUSE_USE_VERSION 31

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h> 
#include <errno.h> 
#include <zip.h>
#include <fuse.h>
#include <string.h>

#define PERM1 0755
#define PERM2 0777

static char *zip_name;
static zip_t *arch;

// Helper
static char *add_char(const char *p)
{
    char *file_dir = malloc(strlen(p) + 2);
    strcpy(file_dir, p);
    file_dir[strlen(p) + 1] = 0;
    file_dir[strlen(p)] ='/';
    return file_dir;
}

static int fuse_zip_file_type(const char *path)
{
    if (strcmp(path, "/") == 0)
	return 0;

    int value_1, value_2;
    char *path_slash = add_char(path + 1);
    value_1 = zip_name_locate(arch, path_slash, 0);
    value_2 = zip_name_locate(arch, path + 1, 0);
    if (value_1 != -1)
        return 0;
    else if (value_2 != -1)
        return 1;
    else
        return -1;
}

////// Main functions

static int fuse_getattr(const char *path, struct stat *stbuf) {
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_size = 1;
        stbuf->st_nlink = 2;
        stbuf->st_mode = S_IFDIR | PERM1;
        return 0;
    }
    zip_stat_t s_buff;
    zip_stat_init(&s_buff);
    char *p_with_slash = add_char(path + 1);
    switch (fuse_zip_file_type(path)) {
        case 0:
            zip_stat(arch, p_with_slash, 0, &s_buff);
            stbuf->st_size = 0;
            stbuf->st_nlink = 2;
            stbuf->st_mode = S_IFDIR | PERM1;
            stbuf->st_mtime = s_buff.mtime;
            break;
        case 1:
            zip_stat(arch, path + 1, 0, &s_buff);
            stbuf->st_mode = S_IFREG | PERM2;
            stbuf->st_nlink = 1;
            stbuf->st_size = s_buff.size;
            stbuf->st_mtime = s_buff.mtime;
            break;
        default:
            return -ENOENT;
    }
    return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    int i;
    int n = zip_get_num_entries(arch, 0);
    for (i = 0; i < n; i++)
    {
        struct stat stru;
        memset(&stru, 0, sizeof(stru));
        zip_stat_t s_buff;
        zip_stat_index(arch, i, 0, &s_buff);
        char *file_path = malloc(strlen(s_buff.name) + 2);
        *file_path = '/';
        strcpy(file_path + 1, s_buff.name);
        char *directory = strdup(file_path);
        char *base_directory = strdup(file_path);
        if (strcmp(path, dirname(directory)) == 0)
        {
            if (file_path[strlen(file_path) - 1] == '/') file_path[strlen(file_path) - 1] = 0;
            fuse_getattr(file_path, &stru);
            char *name = basename(base_directory);
            if (filler(buf, name, &stru, 0))
                break;
        }
    }
    (void) offset;
    (void) fi;
    return 0;
}



static int fuse_statfs(const char *path, struct statvfs *stbuf) {
        if (statvfs(path, stbuf) == -1)
            return -errno;
        else
            return 0;
}

static int fuse_open(const char *path, struct fuse_file_info *fi) {
    (void) fi;
    if(zip_name_locate(arch, path + 1, 0) < 0)
        return -ENOENT;
    return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    zip_stat_t s_buff;
    zip_stat_init(&s_buff);
    zip_stat(arch, path + 1, 0, &s_buff);
    zip_file_t *file = zip_fopen(arch, path + 1, 0);
    (void) fi;
    char temp[s_buff.size + size + offset];
    memset(temp, 0, s_buff.size + size + offset);
    if (zip_fread(file, temp, s_buff.size) == -1)
        return -ENOENT;

    memcpy(buf, temp + offset, size);
    zip_fclose(file);
    return size;
}



static int fuse_mkdir(const char *path, mode_t mode) {
    (void)path;
    (void) mode;
    return -EROFS;
}


static int fuse_rename(const char *from, const char *to) {
    (void) from;
    (void) to;
    return -EROFS;
}

static int fuse_rmdir(const char *path) {
    (void)path;
    return -EROFS;
}


static int fuse_access(const char *path, int mask) {
    (void) mask;
    if (fuse_zip_file_type(path) >= 0)
        return 0;
    return -ENOENT;
}

static int fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void)path;
    (void)buf;
    (void)size;
    (void)offset;
    (void)fi;
    return -EROFS;
}

static int fuse_unlink(const char *path) {
    (void)path;
    return -EROFS;
}


static struct fuse_operations fuse_oper = {
    .getattr = fuse_getattr,
    .readdir = fuse_readdir,
    .access = fuse_access,
    .open = fuse_open,
    .read = fuse_read,
    .write = fuse_write,
    .statfs = fuse_statfs,
    .rename = fuse_rename,
    .mkdir = fuse_mkdir,
    .unlink = fuse_unlink,
    .rmdir = fuse_rmdir,
};

int main(int argc, char *argv[]) {
    arch = zip_open(argv[1], 0, NULL); 
    char *arg[argc - 1];
    int i = 0;
    arg[0] = argv[0];
    for (i = 1; i < argc - 1; i++)
        arg[i] = argv[i + 1];
    return fuse_main(argc - 1, arg, &fuse_oper, NULL);
}