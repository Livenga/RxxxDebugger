#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "../libtask/include/libtask.h"


#define TASK_LIMIT (128)

static size_t _task_size = 0;
static struct task_t *_tasks[TASK_LIMIT];



size_t tman_get_size(void) {
  return _task_size;
}


struct task_t **tman_init(void) {
  // タスクサイズを計算
  _task_size = sizeof(_tasks) / sizeof(*_tasks);
#if defined(__DEBUG__)
  fprintf(stderr, "- task size: %ld\n", (long)_task_size);
#endif

  int i;
  for(i = 0; i < _task_size; ++i) {
    *(_tasks + i) = NULL;
  }

  return _tasks;
}

struct task_t *tman_add(struct task_t *task) {
  int i;

  int stopped_point = -1;
  for(i = 0; i < _task_size; ++i) {
    struct task_t *ptsk = *(_tasks + i);

    if(ptsk == NULL) {
      *(_tasks + i) = task;
      return task;
    } else if(stopped_point < 0 &&
        (ptsk->status == TASK_ABORT || ptsk->status == TASK_STOPPED)) {
      stopped_point = i;
    }
  }

  // 解放済みの位置が存在ない, もしくはすべてのタスクが実行中
  if(stopped_point < 0) {
    return NULL;
  }

  *(_tasks + stopped_point) = task;
  return task;
}


struct task_t **tman_gc(void) {
  int i;

  for(i = 0; i < _task_size; ++i) {
    struct task_t *ptsk = *(_tasks + i);

    if(ptsk == NULL) {
      continue;
    }

    task_tryjoin(ptsk);

    if(ptsk != NULL
        && (ptsk->status == TASK_STOPPED || ptsk->status == TASK_ABORT)) {

      // 状態値が動作中以外の際, 一応 join にて確認の後解放
      task_join(ptsk);
      task_release(ptsk);

      ptsk = NULL;
      *(_tasks + i) = NULL;
    }
  }
  return _tasks;
}
