/*
 * actions.h - Definições para ações de mutex e I/O
 *
 * Suporta:
 *   - MLxx:tt - Lock/Solicitação de mutex xx no instante relativo tt
 *   - MUxx:tt - Unlock/Liberação de mutex xx no instante relativo tt
 *   - IO:tt-dd - Operação de E/S no instante tt com duração dd
 */

#ifndef ACTIONS_H
#define ACTIONS_H

/* Tipos de ações */
typedef enum {
    ACTION_NONE = 0,
    ACTION_MUTEX_LOCK = 1,      /* ML - solicitação de mutex */
    ACTION_MUTEX_UNLOCK = 2,    /* MU - liberação de mutex */
    ACTION_IO = 3               /* IO - operação de entrada/saída */
} ActionType;

/* Suspensão - por que a tarefa foi suspensa */
typedef enum {
    SUSPEND_REASON_NONE = 0,
    SUSPEND_REASON_MUTEX = 1,   /* Aguardando mutex */
    SUSPEND_REASON_IO = 2       /* Aguardando I/O */
} SuspendReason;

/* Estrutura de uma ação */
typedef struct {
    ActionType type;           /* Tipo da ação */
    int        time;           /* Instante relativo (ao início da tarefa) */
    int        mutex_id;       /* Para mutex: ID do mutex (0-31) */
    int        io_duration;    /* Para I/O: duração da operação */
    int        executed;       /* Flag: já foi executada? */
} TaskAction;

/* Máximo de ações por tarefa */
#define MAX_ACTIONS_PER_TASK 32

/* Máximo de mutexes no sistema */
#define MAX_MUTEXES 32

/* Estrutura de um mutex */
typedef struct {
    int id;                    /* ID do mutex (0-31) */
    int owner_task_id;         /* ID da tarefa que possui o mutex (-1 = livre) */
    int waiting_tasks[256];    /* Fila de tarefas aguardando (max 256) */
    int waiting_count;         /* Quantidade de tarefas aguardando */
} Mutex;

/* Forward declaration to avoid circular include between actions.h and task.h */
struct TCB;

typedef struct TCB TCB;

/* Funções exportadas */
int parse_action(const char *action_str, TaskAction *action);
int task_add_action(TCB *task, const TaskAction *action);
TaskAction *task_get_next_action(TCB *task, int relative_time);
void task_mark_action_executed(TCB *task);
void mutex_init(Mutex *m, int id);
int mutex_acquire(Mutex *m, int task_id);
int mutex_release(Mutex *m, int task_id);
int mutex_is_owner(const Mutex *m, int task_id);
int mutex_is_waiting(const Mutex *m, int task_id);

#endif /* ACTIONS_H */
