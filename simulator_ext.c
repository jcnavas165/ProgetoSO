/*
 * simulator_ext.c - Implementação das extensões do simulador
 *
 * Gerencia ações de mutex e I/O durante a simulação
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simulator_ext.h"

/* ── Inicialização ─────────────────────────────────────────────────────── */

void sim_init_mutexes(SimState *sim) {
    if (!sim) return;

    sim->mutex_count = 0;
    memset(sim->mutexes, 0, sizeof(sim->mutexes));
}

/* ── Execução de ações ────────────────────────────────────────────────── */

void sim_execute_actions(SimState *sim, TCB *task, int relative_time) {
    if (!sim || !task) return;

    /* Executa todas as ações do instante current_tick
     * Ordem é importante: deve ser igual à ordem do arquivo de config
     */
    for (int i = task->next_action_idx; i < task->action_count; i++) {
        TaskAction *action = &task->actions[i];

        if (action->executed) continue;
        if (action->time != relative_time) break; /* Nenhuma ação para este tick */

        switch (action->type) {
            case ACTION_MUTEX_LOCK:
                printf("[Tick %3d] Tarefa %d solicita MUTEX %d\n",
                       sim->current_tick, task->id, action->mutex_id);
                sim_handle_mutex_lock(sim, task, action->mutex_id);
                break;

            case ACTION_MUTEX_UNLOCK:
                printf("[Tick %3d] Tarefa %d libera MUTEX %d\n",
                       sim->current_tick, task->id, action->mutex_id);
                sim_handle_mutex_unlock(sim, task, action->mutex_id);
                break;

            case ACTION_IO:
                printf("[Tick %3d] Tarefa %d inicia E/S com duração %d\n",
                       sim->current_tick, task->id, action->io_duration);
                sim_handle_io(sim, task, action->io_duration);
                break;

            default:
                break;
        }

        action->executed = 1;
        task->next_action_idx = i + 1;
    }
}

/* ── Gerenciamento de mutex ────────────────────────────────────────────── */

int sim_handle_mutex_lock(SimState *sim, TCB *task, int mutex_id) {
    if (!sim || !task || mutex_id < 0 || mutex_id >= MAX_MUTEXES) {
        return 0;
    }

    Mutex *m = &sim->mutexes[mutex_id];

    /* Verifica se é a primeira vez que este mutex é usado */
    if (m->id == 0 && m->owner_task_id == 0 && m->waiting_count == 0) {
        m->id = mutex_id;
    }

    if (mutex_acquire(m, task->id)) {
        /* Conseguiu adquirir o mutex - continua normalmente */
        task_add_history(task, sim->current_tick, task->state, task->cpu_id, EVENT_MUTEX_LOCK);
        return 1;
    } else {
        /* Mutex ocupado - suspende a tarefa */
        task->state = TASK_SUSPENDED;
        task->suspend_reason = SUSPEND_REASON_MUTEX;
        task->suspended_mutex_id = mutex_id;
        task->suspended_tick = sim->current_tick;
        task->cpu_id = -1;

        task_add_history(task, sim->current_tick, TASK_SUSPENDED, -1, EVENT_SUSPEND);
        printf("         → Tarefa %d SUSPENSA aguardando MUTEX %d\n", task->id, mutex_id);

        return 0;
    }
}

void sim_handle_mutex_unlock(SimState *sim, TCB *task, int mutex_id) {
    if (!sim || !task || mutex_id < 0 || mutex_id >= MAX_MUTEXES) {
        return;
    }

    Mutex *m = &sim->mutexes[mutex_id];

    if (!mutex_is_owner(m, task->id)) {
        fprintf(stderr, "[AVISO] Tarefa %d tentou liberar MUTEX %d que não possui\n",
                task->id, mutex_id);
        return;
    }

    int next_task_id = mutex_release(m, task->id);
    task_add_history(task, sim->current_tick, task->state, task->cpu_id, EVENT_MUTEX_UNLOCK);

    if (next_task_id != -1) {
        /* Acorda a próxima tarefa da fila de espera */
        TCB *next_task = NULL;
        for (int i = 0; i < sim->num_tasks; i++) {
            if (sim->tasks[i].id == next_task_id) {
                next_task = &sim->tasks[i];
                break;
            }
        }

        if (next_task && next_task->state == TASK_SUSPENDED &&
            next_task->suspend_reason == SUSPEND_REASON_MUTEX &&
            next_task->suspended_mutex_id == mutex_id) {

            next_task->state = TASK_READY;
            next_task->suspend_reason = SUSPEND_REASON_NONE;
            next_task->suspended_mutex_id = -1;

            task_add_history(next_task, sim->current_tick, TASK_READY, -1, EVENT_RESUME);
            printf("         → Tarefa %d acordada (conseguiu MUTEX %d)\n", next_task->id, mutex_id);
        }
    }
}

/* ── Gerenciamento de E/S ──────────────────────────────────────────────── */

void sim_handle_io(SimState *sim, TCB *task, int duration) {
    if (!sim || !task || duration < 1) {
        return;
    }

    /* Suspende a tarefa até que a E/S termine */
    task->state = TASK_SUSPENDED;
    task->suspend_reason = SUSPEND_REASON_IO;
    task->io_end_tick = sim->current_tick + duration;
    task->suspended_tick = sim->current_tick;
    task->cpu_id = -1;

    task_add_history(task, sim->current_tick, TASK_SUSPENDED, -1, EVENT_IO_START);
    printf("         → Tarefa %d SUSPENSA em E/S (será acordada em tick %d)\n",
           task->id, task->io_end_tick);

    /* Adiciona na fila de I/O */
    if (sim->io_count < 256) {
        sim->io_tasks[sim->io_count] = task->id;
        sim->io_end_tick[sim->io_count] = task->io_end_tick;
        sim->io_count++;
    }
}

/* ── Verificação de tarefas suspensas ────────────────────────────────── */

void sim_check_suspended_tasks(SimState *sim) {
    if (!sim) return;

    /* Verifica E/S que terminaram */
    for (int i = 0; i < sim->io_count; i++) {
        if (sim->io_end_tick[i] == sim->current_tick) {
            int task_id = sim->io_tasks[i];
            TCB *task = NULL;

            for (int j = 0; j < sim->num_tasks; j++) {
                if (sim->tasks[j].id == task_id) {
                    task = &sim->tasks[j];
                    break;
                }
            }

            if (task && task->state == TASK_SUSPENDED &&
                task->suspend_reason == SUSPEND_REASON_IO) {

                task->state = TASK_READY;
                task->suspend_reason = SUSPEND_REASON_NONE;
                task->io_end_tick = -1;

                task_add_history(task, sim->current_tick, TASK_READY, -1, EVENT_IO_END);
                printf("[Tick %3d] Tarefa %d acordada (E/S terminou)\n",
                       sim->current_tick, task->id);
            }

            /* Remove da fila de E/S */
            for (int j = i; j < sim->io_count - 1; j++) {
                sim->io_tasks[j] = sim->io_tasks[j + 1];
                sim->io_end_tick[j] = sim->io_end_tick[j + 1];
            }
            sim->io_count--;
            i--;
        }
    }
}

/* ── Envelhecimento de prioridades (PRIOPEnv) ──────────────────────────── */

void sim_update_dynamic_priority(SimState *sim) {
    if (!sim || !sim->config) return;

    int alpha = sim->config->alpha;
    if (alpha <= 0) return; /* PRIOPEnv desativado */

    for (int i = 0; i < sim->num_tasks; i++) {
        TCB *t = &sim->tasks[i];
        if (t->state == TASK_NEW || t->state == TASK_FINISHED) continue;

        int age = sim->current_tick - t->arrival;
        t->dynamic_priority = t->priority + (age / alpha);
    }
}
