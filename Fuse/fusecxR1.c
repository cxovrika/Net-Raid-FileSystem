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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>

#include "shared_types.h"
#include "shared_constants.h"
#include "md5.h"

int timeout = 10; //TODO should be passed
int sysdepth = 0;

int server_count;
int server_sockets[32], server_status[32];
struct server_and_port servers[32];
struct server_and_port hotswap;
int server_unresponsiveness[32];

sem_t syscall_lock;

//CX
static void get_reality_path(const char* path, char rpath[MAX_PATH]) {
	rpath[0] = '\0';
	strcat(rpath, REALITY_PATH);
	strcat(rpath, path);
}

static struct task_R1 generate_task_R1(char* comment, int task_type, const char* path, const char* buf,
	 																		int size, int offset, int mask, mode_t mode, dev_t rdev, const char* from, const char* to, int flags) {
	struct task_R1 task;
	strcpy(task.comment, comment);
	task.task_type = task_type;
	if (path != NULL) strcpy(task.path, path);
	if (buf != NULL && task_type == TASK_WRITE) memcpy(task.buf, buf, size);
	task.size = size;
	task.offset = offset;
	task.mask = mask;
	task.mode = mode;
	task.rdev = rdev;
	if (from != NULL) strcpy(task.from, from);
	if (to != NULL) strcpy(task.to, to);
	task.flags = flags;
	return task;
}


static int cx_getattr(const char *path, struct stat *stbuf)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called GETATTR, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("getattr", TASK_GETATTR, path, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (resp[first_alive].ret_val == 0) {
		memcpy(stbuf, &resp[first_alive].stbuf, sizeof(struct stat));
	}

	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

static int cx_access(const char *path, int mask)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called ACCESS, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("access", TASK_ACCESS, path, NULL, 0, 0, mask, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

static int cx_readlink(const char *path, char *buf, size_t size)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called READLINK, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("readlink", TASK_READLINK, path, buf, size, 0, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (resp[first_alive].ret_val == 0) {
		memcpy(buf, resp[first_alive].buf, size);
	}

	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}


static int cx_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called READDIR, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("readdir", TASK_READDIR, path, buf, 0, offset, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}



	for (int i = 0; i < resp[first_alive].files_in_dir; i++) {
		filler(buf, resp[first_alive].file_names[i], &resp[first_alive].stats[i], 0);
	}
	// while ((de = readdir(dp)) != NULL) {
	// 	struct stat st;
	// 	memset(&st, 0, sizeof(st));
	// 	st.st_ino = de->d_ino;
	// 	st.st_mode = de->d_type << 12;
	// 	if (filler(buf, de->d_name, &st, 0))
	// 		break;
	// }

	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

static int cx_mknod(const char *path, mode_t mode, dev_t rdev)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called MKNOD, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("mknod", TASK_MKNOD, path, NULL, 0, 0, 0, mode, rdev, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}



	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

static int cx_mkdir(const char *path, mode_t mode)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called MKDIR, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("mkdir", TASK_MKDIR, path, NULL, 0, 0, 0, mode, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

static int cx_unlink(const char *path)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called UNLINK, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("unlink", TASK_UNLINK, path, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

static int cx_rmdir(const char *path)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called RMDIR, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("rmdir", TASK_RMDIR, path, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}
/*
static int cx_symlink(const char *from, const char *to)
{
	printf("called SYMLINK\n");
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}*/

static int cx_rename(const char *from, const char *to)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called RENAME\n");

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("rename", TASK_RENAME, NULL, NULL, 0, 0, 0, 0, 0, from, to, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

/*
static int cx_link(const char *from, const char *to)
{
	printf("called LINK\n");
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}
*/

static int cx_chmod(const char *path, mode_t mode)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called CHMOD, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("chmod", TASK_CHMOD, path, NULL, 0, 0, 0, mode, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

/*
static int cx_chown(const char *path, uid_t uid, gid_t gid)
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
*/

static int cx_truncate(const char *path, off_t size)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called TRUNCATE, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("truncate", TASK_TRUNCATE, path, NULL, size, 0, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

static int cx_utimens(const char *path, const struct timespec ts[2])
{
	printf("called UTIMENS, path: %s\n", path);
	return 0; //for now :V

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


static int cx_open(const char *path, struct fuse_file_info *fi)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called OPEN, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("open", TASK_OPEN, path, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (alive_count <= 1) {
		if (--sysdepth == 0) sem_post(&syscall_lock);
		return resp[first_alive].ret_val;
	}
	// printf("ret vals: %d %d\n", resp[0].ret_val, resp[1].ret_val);
	// printf("resp[0].hashes_match:%d\n",resp[0].hashes_match);
	// printf("shahes compared: %d\n", memcmp(resp[0].cur_hash, resp[1].cur_hash, 16));

	if ((resp[0].ret_val == 0 && resp[1].ret_val != 0) ||
 			(resp[0].ret_val != 0 && resp[1].ret_val == 0) ||
			(resp[0].ret_val == 0 && resp[1].ret_val == 0 && resp[0].hashes_match == 0 && memcmp(resp[0].cur_hash, resp[1].cur_hash, 16)) ||
			(resp[0].ret_val == 0 && resp[1].ret_val == 0 && resp[1].hashes_match == 0 && memcmp(resp[0].cur_hash, resp[1].cur_hash, 16) && resp[0].hashes_match != 0) ) {

			int from, to;
			if (resp[0].ret_val == 0 && resp[1].ret_val != 0) from = 0, to = 1; else
			if (resp[0].ret_val != 0 && resp[1].ret_val == 0) from = 1, to = 0; else
			if (resp[0].ret_val == 0 && resp[1].ret_val == 0 && resp[0].hashes_match == 0 && memcmp(resp[0].cur_hash, resp[1].cur_hash, 16)) {
				from  = 0;
				to = 1;
			} else {
				from  = 1;
				to = 0;
			}

			// printf("from and to: %d %d\n", from, to);

			struct task_R1 task = generate_task_R1("copy file", TASK_CPYFL, path, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
			int data_sent = send(server_sockets[from], &task, sizeof(task), 0);
			(void)(data_sent);

			long file_size;
			recv(server_sockets[from], &file_size, sizeof(file_size), 0);
			char buf[file_size];
			recv(server_sockets[from], buf, file_size, 0);

			task = generate_task_R1("recieve file", TASK_RCVFL, path, NULL, file_size, 0, 0, 0, 0, NULL, NULL, 0);
			data_sent = send(server_sockets[to], &task, sizeof(task), 0);
			send(server_sockets[to], buf, file_size, 0);

		}
	else if (resp[0].ret_val != 0 && resp[1].ret_val != 0) {
		if (--sysdepth == 0) sem_post(&syscall_lock);
		return resp[0].ret_val;
	}

	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[0].ret_val;
}

static int cx_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called READ, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("read", TASK_READ, path, buf, size, offset, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	memcpy(buf, resp[first_alive].buf, size);

	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

static int cx_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called WRITE, path: %s\n", path);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("write", TASK_WRITE, path, buf, size, offset, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	return resp[first_alive].ret_val;
}

/*
static int cx_statfs(const char *path, struct statvfs *stbuf)
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
*/
static int cx_release(const char *path, struct fuse_file_info *fi)
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

/*
static int cx_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	printf("called FSYNC, path: %s\n", path);
	char rpath[MAX_PATH];
	get_reality_path(path, rpath);
	// printf("path: %s , mpath: %s\n", path, mpath);

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}
*/

/*
static int
cx_lock(const char *path, struct fuse_file_info *fi,
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
}*/

int cx_releasedir(const char* path, struct fuse_file_info *fi){
	return 0;
}

int cx_opendir(const char* path, struct fuse_file_info *fi){
	return 0;
}




static struct fuse_operations cx_oper = {
	.opendir = cx_opendir,
	.releasedir = cx_releasedir,
	.getattr	= cx_getattr,
	.access		= cx_access,
	.readlink	= cx_readlink,
	.readdir	= cx_readdir,
	.mknod		= cx_mknod,
	.mkdir		= cx_mkdir,
	// .symlink	= cx_symlink,
	.unlink		= cx_unlink,
	.rmdir		= cx_rmdir,
	.rename		= cx_rename,
	// .link		= cx_link,
	.chmod		= cx_chmod,
	// .chown		= cx_chown,
	.truncate	= cx_truncate,
	.utimens	= cx_utimens,
	.open		= cx_open,
	.read		= cx_read,
	.write		= cx_write,
	// .statfs		= cx_statfs,
	.release	= cx_release,
	// .fsync		= cx_fsync,
	// .lock	= cx_lock,
};

void parse_server_data(int argc, char *argv[]) {
	server_count = argc/2 - 1;

	for (int i = 0; i < server_count; i++) {
		strcpy(servers[i].server, argv[2*i + 1]);
		servers[i].port = atoi(argv[2*i + 2]);
	}
	strcpy(hotswap.server, argv[2*server_count + 1]);
	hotswap.port = atoi(argv[2*server_count + 2]);
}

void get_server_connections() {
	for (int i = 0; i < server_count; i++) {
		server_sockets[i] = socket(AF_INET, SOCK_STREAM, 0);

		int server_ip;
		inet_pton(AF_INET, servers[i].server, &server_ip);

		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(servers[i].port);
		server_address.sin_addr.s_addr = server_ip;

		connect(server_sockets[i], (struct sockaddr *) &server_address, sizeof(server_address));
		struct initial_task it;
		it.task_type = 2;
		send(server_sockets[i], &it, sizeof(it), MSG_NOSIGNAL);
	}
}

void* health_check(void* data_) {
	struct task_R1 task;

	while(1) {
		sleep(2);
		printf("\n!== Checking server healths ==!\n");
		sem_wait(&syscall_lock);
		for (int i = 0; i < server_count; i++) {

			task = generate_task_R1("health", TASK_HEALTHCHECK, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
			if (server_status[i] == 0) {
				int data_sent = send(server_sockets[i], &task, sizeof(task), MSG_NOSIGNAL);
				if (data_sent == -1) {
					printf("server %d did not took our healthchek!\n", i);
					server_status[i] = -1;
				} else {
					printf("server %d is alive!\n", i);
				}
			} else {
				server_sockets[i] = socket(AF_INET, SOCK_STREAM, 0);

				int server_ip;
				inet_pton(AF_INET, servers[i].server, &server_ip);

				struct sockaddr_in server_address;
				server_address.sin_family = AF_INET;
				server_address.sin_port = htons(servers[i].port);
				server_address.sin_addr.s_addr = server_ip;

				int result = connect(server_sockets[i], (struct sockaddr *) &server_address, sizeof(server_address));
				if (result == -1) {
					server_unresponsiveness[i]++;
					if (server_unresponsiveness[i] >= timeout) {
						printf("server %d NEEDS TO BE CHANGED WITH HOTSWAP!!!!\n", i);\
						printf("so sad that its not implemented... ;(\n");
					} else {
						printf("server %d is not responding %d times in a row(gonna wait till %d)\n", i, server_unresponsiveness[i], timeout);
					}
					continue;
				}

				struct initial_task it;
				it.task_type = 2;
				send(server_sockets[i], &it, sizeof(it), 0);

				server_status[i] = 0;
				server_unresponsiveness[i] = 0;

				printf("server %d is restored!\n", i);
				printf("server %d will take harakiri task now\n", i);
				task = generate_task_R1("harakiri", TASK_HARAKIRI, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
				int data_sent = send(server_sockets[i], &task, sizeof(task), 0);

				printf("server %d will be used to copy files now\n", 1 - i);
				task = generate_task_R1("SMWYG", TASK_SMWYG, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
				data_sent = send(server_sockets[1 - i], &task, sizeof(task), 0);
				(void)data_sent;

				struct server_response_R1 resp;
				while (1) {
					recv(server_sockets[1 - i], &resp, sizeof(resp), 0);
					if (resp.files_ended == 1) break;

					if (resp.is_dir == 1) {
						printf("recieved  dir: %s\n", resp.buf);
						task = generate_task_R1("mkdir", TASK_MKDIR, resp.buf, NULL, 0, 0, 0, S_IRWXU, 0, NULL, NULL, 0);
						int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
						(void)(data_sent);

						recv(server_sockets[i], &resp, sizeof(struct server_response_R1), 0);

					}
						else
					{
						long file_size;
						recv(server_sockets[1 - i], &file_size, sizeof(file_size), 0);
						char buf[file_size];
						recv(server_sockets[1 - i], buf, file_size, 0);
						// printf("recieved file: %s\n", resp.buf);
						// printf("content:\n");
						// for (int i = 0; i < file_size; i++)
						// 	printf("%c", buf[i]);
						// printf("\nend of file\n");
						task = generate_task_R1("recieve file", TASK_RCVFL, resp.buf, NULL, file_size, 0, 0, 0, 0, NULL, NULL, 0);
						data_sent = send(server_sockets[i], &task, sizeof(task), 0);
						send(server_sockets[i], buf, file_size, 0);
					}
				}

			}
		}
		sem_post(&syscall_lock);
	}

	return NULL;
}

void start_health_checker() {
	sem_init(&syscall_lock, 0, 1);

	pthread_t thread_id;
	pthread_create(&thread_id, NULL, health_check, NULL);
}

int main(int argc, char *argv[])
{
	parse_server_data(argc, argv);
	get_server_connections();
	start_health_checker();

	// argv[2] = NULL;
	// argv[1] = argv[0];
	// argv[0] = path_to_fuse_R1; //pure magic here :V
	argv[2] = NULL;
	argv[1] = argv[0];
	argv[0] = path_to_fuse_R1; //pure magic here :V
	argv[2] = malloc(3);
	strcpy(argv[2], "-f");
	argv[3] = NULL;
	printf("servers: %d\n", server_count);
	return fuse_main(3, argv, &cx_oper, NULL);
}
