#include "net_raid_server.h"
#include "shared_types.h"
#include "shared_constants.h"


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
    int res = lstat(rpath, &resp.stbuf);
    if (res == 0) resp.ret_val = -errno; else
                  resp.ret_val = ret;

    return resp;
}

struct server_response_R1
handle_access(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_readlink(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_readdir(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_mknod(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_mkdir(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_unlink(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_rmdir(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_rename(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_chmod(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_truncate(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_utimens(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_open(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_read(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_write(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}

struct server_response_R1
handle_release(struct task_R1 task) {
    struct server_response_R1 resp;
    return resp;
}


void handle_task_R1(struct task_R1 task) {
  struct server_response_R1 resp;
  printf("in task handler\n");
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
    resp = handle_getattr(task);
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
    resp = handle_getattr(task);
  }


}
