#include "net_raid_server.h"
#include "shared_types.h"
#include "shared_constants.h"
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include "md5.h"


static void get_server_reality_path(const char* path, char rpath[MAX_PATH]) {
	rpath[0] = '\0';
	strcat(rpath, path_to_storage);
	strcat(rpath, path);
}

static void get_unreal_path(const char* path, char upath[MAX_PATH]) {
	upath[0] = '\0';
	strcat(upath, path + strlen(path_to_storage));
}

struct server_response_R1
handle_getattr(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);
    printf("reality path for getattr: %s, normal path: %s\n", rpath, task.path);

    int res = lstat(rpath, &resp.stbuf);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;

    return resp;
}

struct server_response_R1
handle_access(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = access(rpath, task.mask);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;

    return resp;
}

struct server_response_R1
handle_readlink(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = readlink(rpath, resp.buf, task.size - 1);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;

    if (res != -1)
      resp.buf[res] = '\0';

    return resp;
}

struct server_response_R1
handle_readdir(struct task_R1 task) {
    struct server_response_R1 resp;
		char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

		DIR *dp;
		struct dirent *de;

		dp = opendir(rpath);
		if (dp == NULL) {
			resp.ret_val = -errno;
			return resp;
		}

		int files_in_dir = 0;
		while ((de = readdir(dp)) != NULL) {
			strcpy(resp.file_names[files_in_dir], de->d_name);
			memset(&resp.stats[files_in_dir], 0, sizeof(struct stat));
			resp.stats[files_in_dir].st_ino = de->d_ino;
			resp.stats[files_in_dir].st_mode = de->d_type << 12;

			files_in_dir++;
		}

		resp.files_in_dir = files_in_dir;
		resp.ret_val = 0;
		closedir(dp);

    return resp;
}

struct server_response_R1
handle_mknod(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = mknod(rpath, task.mode, task.rdev);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;

    return resp;
}

struct server_response_R1
handle_mkdir(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = mkdir(rpath, task.mode);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;

    return resp;
}

struct server_response_R1
handle_unlink(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = unlink(rpath);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;

    return resp;
}

struct server_response_R1
handle_rmdir(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = rmdir(rpath);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;
    return resp;
}

struct server_response_R1
handle_rename(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = rename(task.from, task.to);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;
    return resp;
}

struct server_response_R1
handle_chmod(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = chmod(rpath, task.mode);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;
    return resp;
}

struct server_response_R1
handle_truncate(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = truncate(rpath, task.size);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = 0;
    return resp;
}

//TODO or is it?
struct server_response_R1
handle_utimens(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}


struct server_response_R1
handle_open(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = open(rpath, task.flags);
    if (res == -1) resp.ret_val = -errno;
		else {
			resp.ret_val = 0, close(res);

			getxattr(rpath, "user.hash", resp.old_hash, 16);
			get_hash_from_path(rpath, resp.cur_hash);
			resp.hashes_match = memcmp(resp.old_hash, resp.cur_hash, 16);
		}

    return resp;
}

struct server_response_R1
handle_read(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);
    int fd, res;

    fd = open(rpath, O_RDONLY);
    if (fd == -1) {
      resp.ret_val = -errno;
      return resp;
    }

    res = pread(fd, resp.buf, task.size, task.offset);
    if (res == -1) {
      resp.ret_val = -errno;
      return resp;
    }
    resp.ret_val = res;
    close (fd);

    return resp;
}

struct server_response_R1
handle_write(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);
    int fd, res;

    fd = open(rpath, O_WRONLY);
    if (fd == -1) {
      resp.ret_val = -errno;
      return resp;
    }

    res = pwrite(fd, task.buf, task.size, task.offset);
    if (res == -1) {
      resp.ret_val = -errno;
      return resp;
    }
    resp.ret_val = res;
    close (fd);

		unsigned char hash[16];
		get_hash_from_path(rpath, hash);
		setxattr(rpath, "user.hash", hash, 16, 0);


		return resp;
}

struct server_response_R1
handle_release(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

void send_file_content(char* path) {
	char rpath[MAX_PATH];
	get_server_reality_path(path, rpath);

	FILE *f = fopen(rpath, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *content = malloc(fsize);
	fread(content, fsize, 1, f);
	fclose(f);

	send(client_socket, &fsize, sizeof(fsize), 0);
	send(client_socket, content, fsize, 0);
}

struct server_response_R1
handle_cpyfl(struct task_R1 task) {
    struct server_response_R1 resp;
		send_file_content(task.path);

    return resp;
}

struct server_response_R1
handle_rcvfl(struct task_R1 task) {
    struct server_response_R1 resp;
		char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

		int fsize = task.size;
		char buf[fsize + 1];

		recv(client_socket, buf, fsize, 0);
		buf[fsize] = '\0';

		FILE* fptr = fopen(rpath, "wb");
		fprintf(fptr, "%s", buf);
		fclose(fptr);

		unsigned char hash[64];
		get_hash_from_path(rpath, hash);
		setxattr(rpath, "user.hash", hash, 64, 0);

    return resp;
}

int send_client_file(char* spath, int is_dir) {
	char upath[MAX_PATH];
	get_unreal_path(spath, upath);
	struct server_response_R1 resp;
	strcpy(resp.buf, upath);
	resp.is_dir = is_dir;
	resp.files_ended = 0;

	if (is_dir) {
			send(client_socket, &resp, sizeof(resp), 0);
	} else {
			send(client_socket, &resp, sizeof(resp), 1);
			send_file_content(upath);
	}
}

int remove_directory(const char *path)
{
	DIR* dir = opendir(path);
	size_t path_len = strlen(path);
	int res = -1;
	if (dir == NULL) return -1;


  struct dirent* de;

	// printf("IN:  %s\n", path);
  while ((de = readdir(dir)) != NULL)
  {

      if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
         continue;
      }

			// printf("   found: %s\n", de->d_name);

			size_t new_len = path_len + strlen(de->d_name) + 2;
      char* new_path = malloc(new_len);

			struct stat statbuf;
			snprintf(new_path, new_len, "%s/%s", path, de->d_name);
			// printf("   new path would be: %s\n", new_path);

			if (!stat(new_path, &statbuf))
			{
			  if (S_ISDIR(statbuf.st_mode)) {
					send_client_file(new_path, 1);
					remove_directory(new_path);
			  }
					else
			  {
					send_client_file(new_path, 0);
			  }
			}
			free(new_path);

  }
  closedir(dir);

	// printf("OUT: %s\n", path);

	return 0;
}

struct server_response_R1
handle_smwyg(struct task_R1 task) {
    struct server_response_R1 resp;
		char rpath[MAX_PATH];
    get_server_reality_path("", rpath);

		printf("in smwyg, rpath: %s\n",  rpath);
		remove_directory(rpath);
		resp.files_ended = 1;
		printf("ended smwyg\n");
    return resp;
}

struct server_response_R1
handle_harakiri(struct task_R1 task) {
    struct server_response_R1 resp;
		printf("IM GONNA KILL MYSELF!\n");
		char rpath[MAX_PATH];
    get_server_reality_path("/", rpath);

		char command[1024] = "exec rm -r ";
		strcat(command, rpath);
		strcat(command, "*");
		printf("gonna: %s\n", command);
		system(command);

    return resp;
}


void handle_task_R1(struct task_R1 task) {
  struct server_response_R1 resp;
  printf("gonna handle: %d, %s\n", task.task_type, task.comment);

	if (task.task_type == TASK_GETATTR) {
    resp = handle_getattr(task);
  }

  if (task.task_type == TASK_ACCESS) {
    resp = handle_access(task);
  }

  if (task.task_type == TASK_READLINK) {
    resp = handle_readlink(task);
  }

  if (task.task_type == TASK_READDIR) {
    resp = handle_readdir(task);
  }

  if (task.task_type == TASK_MKNOD) {
    resp = handle_mknod(task);
  }

  if (task.task_type == TASK_MKDIR) {
    resp = handle_mkdir(task);
  }

  if (task.task_type == TASK_UNLINK) {
    resp = handle_unlink(task);
  }

  if (task.task_type == TASK_RMDIR) {
    resp = handle_rmdir(task);
  }

  if (task.task_type == TASK_RENAME) {
    resp = handle_rename(task);
  }

  if (task.task_type == TASK_CHMOD) {
    resp = handle_chmod(task);
  }

  if (task.task_type == TASK_TRUNCATE) {
    resp = handle_truncate(task);
  }

  if (task.task_type == TASK_OPEN) {
    resp = handle_open(task);
  }

  if (task.task_type == TASK_READ) {
    resp = handle_read(task);
  }

  if (task.task_type == TASK_WRITE) {
    resp = handle_write(task);
  }

  if (task.task_type == TASK_RELEASE) {
    resp = handle_release(task);
  }

	if (task.task_type == TASK_CPYFL) {
    resp = handle_cpyfl(task);
		return;
  }

	if (task.task_type == TASK_RCVFL) {
    resp = handle_rcvfl(task);
		return;
  }

	if (task.task_type == TASK_HEALTHCHECK) {
    return;
  }

	if (task.task_type == TASK_SMWYG) {
    resp = handle_smwyg(task);
	}

	if (task.task_type == TASK_HARAKIRI) {
    resp = handle_harakiri(task);
		return;
  }

	send(client_socket, &resp, sizeof(resp), 0);
}
