#include "net_raid_server.h"
#include "shared_types.h"
#include "shared_constants.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void get_server_reality_path(const char* path, char rpath[MAX_PATH]) {
	rpath[0] = '\0';
	strcat(rpath, path_to_storage);
	strcat(rpath, path);
}

struct server_response_R1
handle_getattr(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);
    printf("reality path for getattr: %s, normal path: %s\n", rpath, task.path);

    int res = lstat(rpath, &resp.stbuf);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;

    return resp;
}

struct server_response_R1
handle_access(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = access(rpath, task.mask);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;

    return resp;
}

struct server_response_R1
handle_readlink(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = readlink(rpath, resp.buf, task.size - 1);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;

    if (res != -1)
      resp.buf[res] = '\0';

    return resp;
}

//TODO!!!!!!!!!!!!!
struct server_response_R1
handle_readdir(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_mknod(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = mknod(rpath, task.mode, task.rdev);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;

    return resp;
}

struct server_response_R1
handle_mkdir(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = mkdir(rpath, task.mode);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;

    return resp;
}

struct server_response_R1
handle_unlink(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = unlink(rpath);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;

    return resp;
}

struct server_response_R1
handle_rmdir(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = rmdir(rpath);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;
    return resp;
}

struct server_response_R1
handle_rename(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = rename(task.from, task.to);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;
    return resp;
}

struct server_response_R1
handle_chmod(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = chmod(rpath, task.mode);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;
    return resp;
}

struct server_response_R1
handle_truncate(struct task_R1 task) {
    struct server_response_R1 resp;
    char rpath[MAX_PATH];
    get_server_reality_path(task.path, rpath);

    int res = mkdir(rpath, task.size);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res;
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

    int res = mkdir(rpath, task.mode);
    if (res == -1) resp.ret_val = -errno; else
                   resp.ret_val = res, close(res);

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

    return resp;
}

struct server_response_R1
handle_release(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}


void handle_task_R1(struct task_R1 task) {
  struct server_response_R1 resp;
  printf("gonna handle: %d, %s\n", task.task_type, task.comment);
  if (task.task_type == TASK_WRITE) {
    printf("btw, the buffer i got looks like this (%d):\n", (int)task.size);
    for (int i = 0; i < task.size; i++)
      printf("%c", task.buf[i]);

    printf("\n");
  }
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


}
