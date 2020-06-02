/**
 */


/** 最もデータを取得したい環境が複数のプロセス同士でプロセスの状態を常に監視しているため,
 * waitpid などで特定のプロセスを停止させた場合システム自体が再起動を生じさせる.
 * そのため遅延をいかに生じさせず適切にデータを記録させる試みをしている.
 */

#ifndef _TMAN_H
#define _TMAN_H

/**
 */
extern size_t tman_get_size(void);

/**
 */
extern struct task_t **tman_init(void);

/**
 */
extern struct task_t *tman_add(struct task_t *task);

/**
 */
extern struct task_t **tman_gc(void);

#endif
