#define FUSE_USE_VERSION 26

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif
#define _POSIX_C_SOURCE 200809L

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
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

#include "shared_types.h"
#include "shared_constants.h"
#include "md5.h"

int timeout;
int sysdepth = 0;

int server_count;
int server_sockets[32], server_status[32];
struct server_and_port servers[32];
struct server_and_port hotswap;
int server_unresponsiveness[32];
int errorlog_fd, cache_capacity, cache_size;
char disk_name[64];
sem_t syscall_lock;

typedef struct Cnode Cnode;

struct Cnode {
	int size;
	int pseudo_size;
	int offset;
	char* path;
	char* buf;
	Cnode* prev;
	Cnode* next;
};

Cnode* head;
Cnode* tail;
//CX

void print_cache_status() {
	printf("right now cache contains:\n");
	for (Cnode* cur = head->next; cur != tail; cur = cur->next) {
		printf("entry: path: %s , offset: %d, size: %d, pseudo_size: %d\n", cur->path, cur->offset, cur->size, cur->pseudo_size);
	}
	printf("--- ended cache info\n");
}

void set_up_cache() {
	head = malloc(sizeof(Cnode));
	tail = malloc(sizeof(Cnode));
	head->next = tail;
	head->prev = NULL;
	tail->prev = head;
	tail->next = NULL;

	cache_size = 0;
}

void remove_cache_entry(Cnode* ent) {
	ent->prev->next = ent->next;
	ent->next->prev = ent->prev;
	cache_size -= ent->size;
	free(ent->path);
	free(ent->buf);
	free(ent);
}

void insert_cache_entry(Cnode* ent) {
	ent->prev = tail->prev;
	ent->next = tail;
	tail->prev->next = ent;
	tail->prev = ent;
	cache_size += ent->size;
}

void update_cache_by_write(const char* path, const char* buf, int size, int offset) {

	Cnode* next_node;
	for (Cnode* cur = tail->prev; cur != head; cur = next_node) {
		next_node = cur->prev;
		if (strcmp(cur->path, path)) continue;
		int cst = cur->offset;
		int cen = cur->offset + cur->pseudo_size - 1;
		int wst = offset;
		int wen = offset + size - 1;
		if (wst > cen || cst > wen) continue;

		wst = wst > cst ? wst : cst;
		wen = wen < cen ? wen : cen;
		memcpy(cur->buf + (wst - cur->offset), buf, wen - wst + 1);
		cur->size = cur->size > wen+1? cur->size : wen+1;
	}

}


int check_cache_for_read(const char* path, const char* buf_, int size, int offset) {
	char* buf;
	memcpy(&buf, &buf_, sizeof(char*));

	printf("\n\nCHECKING CACHE FOR READ, path: %s\n", path);


	for (Cnode* cur = head->next; cur != tail; cur = cur->next) {
		// printf("comparing path to: %s\n", cur->path);
		if (strcmp(cur->path, path)) continue;
		int cst = cur->offset;
		int cen = cur->offset + cur->pseudo_size - 1;
		int rst = offset;
		int ren = offset + size - 1;
		// printf("after cst: %d, cen: %d, rst: %d, ren: %d \n", cst, cen, rst, ren );
		if (!(rst >= cst && ren <= cen)) continue;

		int bytes_to_read = size < cur->size ? size : cur->size;
		memcpy(buf, cur->buf + (rst - cst), bytes_to_read);

		return bytes_to_read;
	}

	return -1;
}


void add_new_cache_entry(const char* path, char* buf, int size, int offset, int pseudo_size) {
	while (cache_size + size > cache_capacity) {
		remove_cache_entry(head->next);
	}

	buf[size] = '\0';
	// printf("gonna add new cnode, path: %s, content: %s size, offset: %d %d\n", path, buf, size, offset);

	Cnode* ent = malloc(sizeof(Cnode));
	ent->size = size;
	ent->pseudo_size = pseudo_size;
	ent->offset = offset;
	ent->path = malloc(strlen(path) + 1);
	ent->buf = malloc(size);
	strcpy(ent->path, path);
	memcpy(ent->buf, buf, size);

	insert_cache_entry(ent);
	// printf("\n\nchecking after insert\n");
	// printf("from head path: %s\n", head->next->path);
	// printf("from tail path: %s\n\n", tail->prev->path);

}


void fill_cache_capacity(char* sz) {
	int len = strlen(sz);
	int koe;
	if (sz[len-1] == 'K') koe = 1024; else
												koe = 1024*1024;

	sz[len-1] = sz[len];
	cache_capacity = atoi(sz);
	cache_capacity *= koe;
	// printf("koe: %d, cache_capacity: %d\n", koe, cache_capacity);

}

void log_info(char* text) {
	dprintf(errorlog_fd, "%s\n", text);
}

void log_file_restoring(const char* path) {
	char info[256];
	time_t timer;
	char time_buffer[26];
  struct tm* tm_info;

	time(&timer);
  tm_info = localtime(&timer);
	strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	snprintf(info, sizeof(info), "[%s] %s is restoring file: %s", time_buffer, disk_name, path);
	dprintf(errorlog_fd, "%s\n", info);
}

void log_server_info(int server_index, const char* text, const char* path) {
	time_t timer;
	char time_buffer[26];
  struct tm* tm_info;

	time(&timer);
  tm_info = localtime(&timer);
	strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

	char info[256];
	info[0] = '\0';
	char port[10];
	snprintf(port, sizeof(port), "%d", servers[server_index].port);
	snprintf(info, sizeof(info), "[%s] %s %s:%s %s  %s", time_buffer, disk_name, servers[server_index].server, port, text, path);
	dprintf(errorlog_fd, "%s\n", info);
}

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
		log_server_info(i, "called getattr", path);
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
		log_server_info(i, "called access", path);
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
		log_server_info(i, "called readlink", path);
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
		log_server_info(i, "called readdir", path);
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
		log_server_info(i, "called mknod", path);
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
		log_server_info(i, "called mmkdir", path);
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
		log_server_info(i, "called unlink", path);
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
		log_server_info(i, "called rmdir", path);
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
		log_server_info(i, "called rename", from);
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
		log_server_info(i, "called chmod", path);
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
		log_server_info(i, "called truncate", path);
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
		log_server_info(i, "called open", path);
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

			log_file_restoring(path);

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
	printf("called READ(%d), path: %s\n", (int)size, path);

	print_cache_status();
	int cached_read = check_cache_for_read(path, buf, (int)size, (int)offset);
	if (cached_read != -1) {
		if (--sysdepth == 0) sem_post(&syscall_lock);
		return cached_read;
	}

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	char* buffs[2];
	buffs[0] = malloc(1);
	buffs[1] = malloc(1);

	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		log_server_info(i, "called read", path);
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("read", TASK_READ, path, buf, size, offset, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
		if (resp[i].size > 0) {
			buffs[i] = realloc(buffs[i], resp[i].size);
			recv(server_sockets[i], buffs[i], resp[i].size, 0);
		}
	}

	memcpy(buf, buffs[first_alive], resp[first_alive].size);
	free(buffs[0]);
	free(buffs[1]);

	if (resp[first_alive].size <= cache_capacity) {
		// printf("------------------------ ADDING NEW CACHE ENTRY\n");
		add_new_cache_entry(path, buf, resp[first_alive].size, (int)offset, size);
	}

	if (--sysdepth == 0) sem_post(&syscall_lock);

	return resp[first_alive].size;
}

static int cx_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	if (sysdepth++ == 0) sem_wait(&syscall_lock);
	printf("called WRITE, size: %d, offset: %d,  path: %s\n", (int)size, (int)offset, path);

	update_cache_by_write(path, buf, (int)size, (int)offset);

	struct server_response_R1 resp[server_count];
	int alive_count = 0, first_alive = -1;
	for (int i = 0; i < server_count; i++) {
		if (server_status[i] != 0) continue;
		log_server_info(i, "called write", path);
		if (first_alive == -1) first_alive = i;
		alive_count++;

		struct task_R1 task = generate_task_R1("write", TASK_WRITE, path, buf, size, offset, 0, 0, 0, NULL, NULL, 0);
		int data_sent = send(server_sockets[i], &task, sizeof(task), 0);
		(void)(data_sent);
		recv(server_sockets[i], &resp[i], sizeof(struct server_response_R1), 0);
	}


	if (--sysdepth == 0) sem_post(&syscall_lock);
	printf("return WRITE, size: %d, offset: %d,  path: %s\n", (int)size, (int)offset, path);
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
	server_count = (argc-7)/2;

	for (int i = 0; i < server_count; i++) {
		strcpy(servers[i].server, argv[2*i + 5]);
		servers[i].port = atoi(argv[2*i + 6]);
	}
	strcpy(hotswap.server, argv[2*server_count + 5]);
	hotswap.port = atoi(argv[2*server_count + 6]);
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
		log_server_info(i, "connected", "");
	}
}

void fill_up_server(int i) {
	struct initial_task it;
	it.task_type = 2;
	send(server_sockets[i], &it, sizeof(it), 0);

	server_status[i] = 0;
	server_unresponsiveness[i] = 0;

	printf("server %d is restored!\n", i);
	printf("server %d will take harakiri task now\n", i);
	struct task_R1 task = generate_task_R1("harakiri", TASK_HARAKIRI, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
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

void change_with_hotswap(int i) {
	server_sockets[i] = socket(AF_INET, SOCK_STREAM, 0);

	int server_ip;
	inet_pton(AF_INET, hotswap.server, &server_ip);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(hotswap.port);
	server_address.sin_addr.s_addr = server_ip;
	int result = connect(server_sockets[i], (struct sockaddr *) &server_address, sizeof(server_address));
	if (result == -1) {
		printf("<!-- could not connect to hotswap!--!>\n");
		return;
	}

	servers[i] = hotswap;
	fill_up_server(i);
}

void* health_check(void* data_) {
	struct task_R1 task;

	while(1) {
		sleep(1);
		printf("\n!== Checking server healths ==!\n");
		sem_wait(&syscall_lock);
		for (int i = 0; i < server_count; i++) {

			task = generate_task_R1("health", TASK_HEALTHCHECK, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, 0);
			if (server_status[i] == 0) {
				int data_sent = send(server_sockets[i], &task, sizeof(task), MSG_NOSIGNAL);
				if (data_sent == -1) {
					printf("server %d did not took our healthchek!\n", i);
					log_server_info(i, "could not send data", "");
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
						log_server_info(i, "will be changed with hotswap", "");
						change_with_hotswap(i);
						log_server_info(i, "hot swap server added", "");
					} else {
						printf("server %d is not responding %d times in a row(gonna wait till %d)\n", i, server_unresponsiveness[i], timeout);
					}
					continue;
				}

				fill_up_server(i);
			}
		}
		sem_post(&syscall_lock);
	}

	return NULL;
}

void start_health_checker() {
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, health_check, NULL);
}

int main(int argc, char *argv[])
{
	parse_server_data(argc, argv);
	errorlog_fd = open(argv[1], O_WRONLY|O_APPEND);
	strcpy(disk_name, argv[2]);
	timeout = atoi(argv[3]);
	fill_cache_capacity(argv[4]);
	set_up_cache();
	get_server_connections();
	sem_init(&syscall_lock, 0, 1);
	start_health_checker();

	//pure magic here :V
	argv[2] = NULL;
	argv[1] = argv[0];
	argv[0] = path_to_fuse_R1;
	argv[2] = malloc(3);
	strcpy(argv[2], "-f");
	argv[3] = NULL;
	// argv[2] = NULL;
	// argv[1] = argv[0];
	// argv[0] = path_to_fuse_R1;
	return fuse_main(3, argv, &cx_oper, NULL);
}
