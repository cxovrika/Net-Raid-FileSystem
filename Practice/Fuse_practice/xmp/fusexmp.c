#define FUSE_USE_VERSION 26

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>

static char* ROOTDIR;
#define MAX_PATH 512

char REALITY_PATH[MAX_PATH] = "/home/vagrant/Net-Raid-FileSystem/Practice/Fuse_practice/xmp/reality";

//CX
static void get_reality_path(const char* path, char rpath[MAX_PATH]) {
	rpath[0] = '\0';
	strcat(rpath, REALITY_PATH);
	strcat(rpath, path);
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	printf("called GETATTR, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);

	int res;
	res = lstat(rpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	printf("called ACCESS, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = access(rpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	printf("called READLINK, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = readlink(rpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	printf("called READDIR, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);

	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(rpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	printf("called MKNOD, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = mknod(rpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	printf("called MKDIR, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = mkdir(rpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	printf("called UNLINK, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = unlink(rpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	printf("called RMDIR, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = rmdir(rpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	printf("called SYMLINK\n");
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	printf("called RENAME\n");
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	printf("called LINK\n");
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	printf("called CHMOD, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);

	int res;

	res = chmod(rpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	printf("called CHOWN, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = lchown(rpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	printf("called TRUNCATE, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = truncate(rpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	printf("called UTIMENS, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(rpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	printf("called OPEN, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = open(rpath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	printf("called READ, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int fd;
	int res;

	(void) fi;
	fd = open(rpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	printf("called WRITE, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int fd;
	int res;

	(void) fi;
	fd = open(rpath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	printf("called STATFS, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int res;

	res = statvfs(rpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	printf("called RELEASE, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	printf("called FSYNC, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

static int
xmp_lock(const char *path, struct fuse_file_info *fi,
	 int cmd, struct flock *lock)
{
	printf("called LOCK, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);
	int fd;

	(void)fi;

	fd = open(rpath, O_WRONLY);
	if (fd == -1)
		return -errno;

	if (cmd != F_GETLK && cmd != F_SETLK && cmd != F_SETLKW)
		return -EINVAL;

	if (fcntl(fd, cmd, lock) == -1)
		return -errno;

	return 0;
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
	.lock		= xmp_lock,
};

int main(int argc, char *argv[])
{
	printf ("mounting in: %s\n", argv[argc-2]);
	ROOTDIR = realpath(argv[argc-2], NULL);
	printf ("REAL PATH: %s\n", ROOTDIR);

	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
