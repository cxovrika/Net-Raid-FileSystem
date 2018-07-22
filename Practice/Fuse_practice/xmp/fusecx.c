#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int bb_getattr(const char *path, struct stat *statbuf)
{
    return lstat(path, statbuf);
}

int bb_readlink(const char *path, char *link, size_t size)
{
    return readlink(path, link, size - 1);
}

int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
    return mknod(path, mode, dev);
}

int bb_mkdir(const char *path, mode_t mode)
{
    return mkdir(path, mode);
}

int bb_unlink(const char *path)
{
    return unlink(path);
}

int bb_rmdir(const char *path)
{
    return rmdir(path);
}

int bb_symlink(const char *path, const char *link)
{
    return symlink(path, link);
}

int bb_rename(const char *path, const char *newpath)
{
    return rename(path, newpath);
}

int bb_link(const char *path, const char *newpath)
{
    return link(path, newpath);
}

int bb_chmod(const char *path, mode_t mode)
{
    return chmod(path, mode);
}

/** Change the owner and group of a file */
int bb_chown(const char *path, uid_t uid, gid_t gid)
{
    return chown(path, uid, gid);
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
    return truncate(path, newsize);
}

/** Change the access and/or modification times of a file */
int bb_utime(const char *path, struct utimbuf *ubuf)
{
    return utime(path, ubuf);
}

static int bb_open(const char *path, struct fuse_file_info *fi)
{
    int fd;
    fd = open(path, fi->flags);
    if (fd < 0) return -errno;

    fi->fh = fd;
    return fd;
}

int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    return pread(fi->fh, buf, size, offset);
}

int bb_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    return pwrite(fi->fh, buf, size, offset);
}

int bb_statfs(const char *path, struct statvfs *statv)
{
    return statvfs(path, statv);
}
int bb_release(const char *path, struct fuse_file_info *fi)
{
    return close(fi->fh);
}

int bb_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	return fsync(fi->fh);
}

int bb_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    dp = opendir(path);
    if (dp == NULL) return -errno;
    fi->fh = (intptr_t) dp;

    return 0;
}
int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;

    dp = (DIR *) (uintptr_t) fi->fh;

    de = readdir(dp);
    if (de == 0) return -errno;
    do {
			if (filler(buf, de->d_name, NULL, 0) != 0) {
			    return -ENOMEM;
			}
    } while ((de = readdir(dp)) != NULL);

    return 0;
}

int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
    closedir((DIR *) (uintptr_t) fi->fh);
    return 0;
}


int bb_access(const char *path, int mask)
{
    int retstat = 0;
    retstat = access(path, mask);

    if (retstat < 0) return -errno;
    return retstat;
}

int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0) return -errno;

    return retstat;
}

int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;

    if (!strcmp(path, "/"))
			return bb_getattr(path, statbuf);

    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0) return -errno;

		return retstat;
}

struct fuse_operations bb_oper = {
  .getattr = bb_getattr,
  .readlink = bb_readlink,
  .getdir = NULL,
  .mknod = bb_mknod,
  .mkdir = bb_mkdir,
  .unlink = bb_unlink,
  .rmdir = bb_rmdir,
  .symlink = bb_symlink,
  .rename = bb_rename,
  .link = bb_link,
  .chmod = bb_chmod,
  .chown = bb_chown,
  .truncate = bb_truncate,
  .utime = bb_utime,
  .open = bb_open,
  .read = bb_read,
  .write = bb_write,

  .statfs = bb_statfs,
  .flush = bb_flush,
  .release = bb_release,
  .fsync = bb_fsync,


  .opendir = bb_opendir,
  .readdir = bb_readdir,
  .releasedir = bb_releasedir,
  .fsyncdir = bb_fsyncdir,
  .init = bb_init,
  .destroy = bb_destroy,
  .access = bb_access,
  .ftruncate = bb_ftruncate,
  .fgetattr = bb_fgetattr
};


int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &bb_oper, NULL);
}
