/*
 * actions.c - Implementação do gerenciamento de ações (mutex e I/O)
 *
 * Funções para:
 *   - Parsear ações de configuração
 *   - Executar ações na simulação
 *   - Gerenciar mutexes e I/O
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "actions.h"
#include "task.h"

/*
 * Parseia uma ação do formato:
 *   MLxx:tt - Lock de mutex xx no tempo tt
 *   MUxx:tt - Unlock de mutex xx no tempo tt
 *   IO:tt-dd - Operação de I/O no tempo tt com duração dd
 */
int parse_action(const char *action_str, TaskAction *action) {
    if (!action_str || !action) return -1;

    char temp[128];
    strncpy(temp, action_str, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    /* Verifica formato "IO:..." */
    if (strncmp(temp, "IO:", 3) == 0) {
        int time_val, duration;
        if (sscanf(temp + 3, "%d-%d", &time_val, &duration) != 2) {
            fprintf(stderr, "[ERRO] Formato de I/O inválido: %s\n", action_str);
            return -1;
        }
        action->type = ACTION_IO;
        action->time = time_val;
        action->io_duration = duration;
        if (duration < 1) {
            fprintf(stderr, "[ERRO] Duração de I/O deve ser >= 1: %s\n", action_str);
            return -1;
        }
        return 0;
    }

    /* Verifica formato "MLxx:tt" (lock) */
    if (strncmp(temp, "ML", 2) == 0) {
        int mutex_id, time_val;
        if (sscanf(temp + 2, "%d:%d", &mutex_id, &time_val) != 2) {
            fprintf(stderr, "[ERRO] Formato de lock inválido: %s\n", action_str);
            return -1;
        }
        if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) {
            fprintf(stderr, "[ERRO] ID de mutex inválido: %d\n", mutex_id);
            return -1;
        }
        action->type = ACTION_MUTEX_LOCK;
        action->time = time_val;
        action->mutex_id = mutex_id;
        return 0;
    }

    /* Verifica formato "MUxx:tt" (unlock) */
    if (strncmp(temp, "MU", 2) == 0) {
        int mutex_id, time_val;
        if (sscanf(temp + 2, "%d:%d", &mutex_id, &time_val) != 2) {
            fprintf(stderr, "[ERRO] Formato de unlock inválido: %s\n", action_str);
            return -1;
        }
        if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) {
            fprintf(stderr, "[ERRO] ID de mutex inválido: %d\n", mutex_id);
            return -1;
        }
        action->type = ACTION_MUTEX_UNLOCK;
        action->time = time_val;
        action->mutex_id = mutex_id;
        return 0;
    }

    fprintf(stderr, "[ERRO] Tipo de ação desconhecido: %s\n", action_str);
    return -1;
}

/*
 * Adiciona uma ação a uma tarefa
 */
int task_add_action(TCB *task, const TaskAction *action) {
    if (!task || !action) return -1;

    if (task->action_count >= MAX_ACTIONS_PER_TASK) {
        fprintf(stderr, "[ERRO] Tarefa %d: limite de ações atingido\n", task->id);
        return -1;
    }

    task->actions[task->action_count] = *action;
    task->action_count++;
    return 0;
}

/*
 * Obtém a próxima ação a ser executada (relativa ao tempo atual da tarefa)
 * Retorna NULL se não há ações pendentes
 */
TaskAction *task_get_next_action(TCB *task, int relative_time) {
    if (!task || task->next_action_idx >= task->action_count) {
        return NULL;
    }

    TaskAction *action = &task->actions[task->next_action_idx];
    if (action->time == relative_time && !action->executed) {
        return action;
    }

    return NULL;
}

/*
 * Marca uma ação como executada e avança para a próxima
 */
void task_mark_action_executed(TCB *task) {
    if (!task || task->next_action_idx >= task->action_count) {
        return;
    }

    task->actions[task->next_action_idx].executed = 1;
    task->next_action_idx++;
}

/*
 * Inicializa um mutex
 */
void mutex_init(Mutex *m, int id) {
    if (!m) return;
    m->id = id;
    m->owner_task_id = -1;
    m->waiting_count = 0;
}

/*
 * Tenta adquirir um mutex
 * Retorna 1 se conseguiu, 0 se está ocupado
 */
int mutex_acquire(Mutex *m, int task_id) {
    if (!m) return 0;

    if (m->owner_task_id == -1) {
        /* Mutex está livre */
        m->owner_task_id = task_id;
        return 1;
    }

    /* Mutex está ocupado - adiciona na fila de espera */
    if (m->waiting_count < 256) {
        m->waiting_tasks[m->waiting_count++] = task_id;
    }
    return 0;
}

/*
 * Libera um mutex
 * Retorna ID da próxima tarefa a adquirir (ou -1 se fila vazia)
 */
int mutex_release(Mutex *m, int task_id) {
    if (!m) return -1;

    if (m->owner_task_id != task_id) {
        /* Tarefa não é dona do mutex */
        return -1;
    }

    m->owner_task_id = -1;

    /* Retorna próxima tarefa da fila de espera */
    if (m->waiting_count > 0) {
        int next_task = m->waiting_tasks[0];
        m->waiting_count--;
        for (int i = 0; i < m->waiting_count; i++) {
            m->waiting_tasks[i] = m->waiting_tasks[i + 1];
        }
        return next_task;
    }

    return -1;
}

/*
 * Verifica se a tarefa é dona do mutex
 */
int mutex_is_owner(const Mutex *m, int task_id) {
    if (!m) return 0;
    return (m->owner_task_id == task_id);
}

/*
 * Verifica se a tarefa está aguardando este mutex
 */
int mutex_is_waiting(const Mutex *m, int task_id) {
    if (!m) return 0;

    for (int i = 0; i < m->waiting_count; i++) {
        if (m->waiting_tasks[i] == task_id) {
            return 1;
        }
    }
    return 0;
}
