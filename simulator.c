#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simulator.h"/* ── Funções auxiliares internas ──────────────────────────────────────────── */

static TCB *find_task_by_id(SimState *sim, int task_id) {
    for (int i = 0; i < sim->num_tasks; i++) {
        if (sim->tasks[i].id == task_id) {
            return &sim->tasks[i];
        }
    }
    return NULL;
}

int sim_init(SimState *sim, SimConfig *config) {
    memset(sim, 0, sizeof(SimState));

    sim->config     = config;
    sim->scheduler  = scheduler_get(config->algo);

    if (!sim->scheduler) {
        fprintf(stderr, "[ERRO] Falha ao obter função do escalonador\n");
        return -1;
    }

    sim->num_tasks = config->num_tasks;
    for (int i = 0; i < sim->num_tasks; i++) {
        sim->tasks[i] = config->tasks[i]; /* Cópia completa do TCB */
    }

    sim->num_cpus = config->num_cpus;
    for (int c = 0; c < sim->num_cpus; c++) {
        sim->cpus[c].task_id     = NO_TASK;
        sim->cpus[c].quantum_used = 0;
        sim->cpus[c].is_off      = 0;
    }

    sim->current_tick  = 0;
    sim->finished      = 0;
    sim->history_count = 0;
    sim->history_pos   = 0;

    /* Aloca o histórico dinamicamente para evitar stack overflow */
    sim->history = (SimSnapshot *)malloc(MAX_TICKS * sizeof(SimSnapshot));
    if (!sim->history) {
        fprintf(stderr, "[ERRO] Falha ao alocar memória para o histórico\n");
        return -1;
    }

    /* Salva o snapshot inicial (tick 0, antes de qualquer execução) */
    sim_take_snapshot(sim);

    return 0;
}


void sim_take_snapshot(SimState *sim) {
    if (sim->history_count >= MAX_TICKS) {
        fprintf(stderr, "[AVISO] Histórico cheio! Simulação muito longa.\n");
        return;
    }

    SimSnapshot *snap = &sim->history[sim->history_count];
    snap->tick      = sim->current_tick;
    snap->num_tasks = sim->num_tasks;
    snap->num_cpus  = sim->num_cpus;

    memcpy(snap->tasks, sim->tasks, sim->num_tasks * sizeof(TCB));
    memcpy(snap->cpus,  sim->cpus,  sim->num_cpus  * sizeof(CpuState));

    sim->history_count++;
    sim->history_pos = sim->history_count - 1;
}


void sim_restore_snapshot(SimState *sim, int pos) {
    if (pos < 0 || pos >= sim->history_count) {
        fprintf(stderr, "[ERRO] Posição de snapshot inválida: %d\n", pos);
        return;
    }

    const SimSnapshot *snap = &sim->history[pos];
    sim->current_tick = snap->tick;

    memcpy(sim->tasks, snap->tasks, snap->num_tasks * sizeof(TCB));
    memcpy(sim->cpus,  snap->cpus,  snap->num_cpus  * sizeof(CpuState));

    sim->history_pos = pos;

    sim->history_count = pos + 1;

    /* Verifica se a simulação ainda está em andamento */
    sim->finished = 1;
    for (int i = 0; i < sim->num_tasks; i++) {
        if (sim->tasks[i].state != TASK_FINISHED) {
            sim->finished = 0;
            break;
        }
    }
}


int sim_step(SimState *sim) {
    if (sim->finished) return 1;

    int tick = sim->current_tick;

    /* ── FASE 1: Chegada de novas tarefas  */
    for (int i = 0; i < sim->num_tasks; i++) {
        TCB *t = &sim->tasks[i];
        if (t->state == TASK_NEW && t->arrival == tick) {
            t->state = TASK_READY;
            task_add_history(t, tick, TASK_READY, NO_TASK, EVENT_ARRIVAL);
            printf("[Tick %3d] Tarefa %d chegou no sistema\n", tick, t->id);
        }
    }

    /* ── FASE 2: Verifica tarefas que terminaram no tick ANTERIOR 
     * (uma tarefa que teve remaining=0 após o último tick está concluída) */
    for (int i = 0; i < sim->num_tasks; i++) {
        TCB *t = &sim->tasks[i];
        if (t->state == TASK_RUNNING && t->remaining == 0) {
            /* Encontra em qual CPU esta tarefa estava */
            int cpu = t->cpu_id;
            t->state      = TASK_FINISHED;
            t->finish_tick = tick;
            t->turnaround  = tick - t->arrival;
            t->cpu_id     = NO_TASK;
            task_add_history(t, tick, TASK_FINISHED, NO_TASK, EVENT_FINISH);

            if (cpu != NO_TASK) {
                sim->cpus[cpu].task_id      = NO_TASK;
                sim->cpus[cpu].quantum_used  = 0;
            }

            printf("[Tick %3d] Tarefa %d CONCLUÍDA (turnaround=%d, espera=%d)\n",
                   tick, t->id, t->turnaround, t->wait_time);
        }
    }

    /* ── FASE 3: Verifica quantum esgotado  */
    for (int c = 0; c < sim->num_cpus; c++) {
        int tid = sim->cpus[c].task_id;
        if (tid == NO_TASK) continue;

        TCB *t = find_task_by_id(sim, tid);
        if (!t || t->state != TASK_RUNNING) continue;

        /* Se quantum foi totalmente usado, preempta a tarefa */
        if (sim->cpus[c].quantum_used >= sim->config->quantum) {
            t->state  = TASK_READY;
            t->cpu_id = NO_TASK;
            task_add_history(t, tick, TASK_READY, NO_TASK, EVENT_PREEMPT);

            sim->cpus[c].task_id      = NO_TASK;
            sim->cpus[c].quantum_used  = 0;

            printf("[Tick %3d] Tarefa %d PREEMPTADA (quantum esgotado na CPU %d)\n",
                   tick, t->id, c);
        }
    }

    /* ── FASE 4: Escalonamento  */
    SchedContext ctx;
    ctx.tasks       = sim->tasks;
    ctx.num_tasks   = sim->num_tasks;
    ctx.current_tick = tick;
    ctx.quantum     = sim->config->quantum;
    ctx.num_cpus    = sim->num_cpus;
    for (int c = 0; c < sim->num_cpus; c++) {
        ctx.cpu_tasks[c] = sim->cpus[c].task_id;
    }

    SchedResult sched_res;
    sim->scheduler(&ctx, &sched_res);

    /* ── FASE 5: Aplica a decisão do escalonador  */
    for (int c = 0; c < sim->num_cpus; c++) {
        int old_tid = sim->cpus[c].task_id;
        int new_tid = sched_res.next_task_id[c];

        if (old_tid != NO_TASK && old_tid != new_tid) {
            /* A tarefa que estava aqui não foi escolhida para continuar */
            TCB *old_t = find_task_by_id(sim, old_tid);
            if (old_t && old_t->state == TASK_RUNNING) {
                old_t->state  = TASK_READY;
                old_t->cpu_id = NO_TASK;
                task_add_history(old_t, tick, TASK_READY, NO_TASK, EVENT_PREEMPT);
            }
            sim->cpus[c].task_id      = NO_TASK;
            sim->cpus[c].quantum_used  = 0;
        }
    }

    /* Segundo: coloca as novas tarefas nas CPUs */
    for (int c = 0; c < sim->num_cpus; c++) {
        int new_tid = sched_res.next_task_id[c];

        if (new_tid == NO_TASK) {
            /* CPU fica ociosa - verifica se deve ser desligada */
            sim->cpus[c].task_id = NO_TASK;
            /* Desliga a CPU se não há NENHUMA tarefa pronta ou rodando */
            int has_ready = 0;
            for (int i = 0; i < sim->num_tasks; i++) {
                if (sim->tasks[i].state == TASK_READY ||
                    sim->tasks[i].state == TASK_RUNNING) {
                    has_ready = 1;
                    break;
                }
            }
            if (!has_ready) {
                sim->cpus[c].is_off = 1;
            }
            continue;
        }

        /* Liga a CPU caso esteja desligada */
        sim->cpus[c].is_off = 0;

        TCB *new_t = find_task_by_id(sim, new_tid);
        if (!new_t) continue;

        int already_here = (sim->cpus[c].task_id == new_tid);

        if (!already_here) {
            /* Tarefa nova entrando nesta CPU */
            if (new_t->state == TASK_READY) {
                /* Registra início de execução (primeira vez ou retomada) */
                if (new_t->start_tick < 0) {
                    new_t->start_tick = tick; /* Primeira vez executando */
                }
                new_t->state  = TASK_RUNNING;
                new_t->cpu_id = c;
                task_add_history(new_t, tick, TASK_RUNNING, c, EVENT_START);

                sim->cpus[c].task_id      = new_tid;
                sim->cpus[c].quantum_used  = 0; /* Reinicia quantum ao entrar */

                /* Marca sorteio se necessário */
                if (sched_res.used_lottery) {
                    task_add_history(new_t, tick, TASK_RUNNING, c, EVENT_LOTTERY);
                }

                printf("[Tick %3d] Tarefa %d → CPU %d (remaining=%d)\n",
                       tick, new_t->id, c, new_t->remaining);
            }
        } else {
            /* Tarefa continua na mesma CPU */
            sim->cpus[c].task_id = new_tid;
        }
    }

    /* ── FASE 6: Executa 1 tick (decrementa tempo restante e quantum)  */
    for (int c = 0; c < sim->num_cpus; c++) {
        int tid = sim->cpus[c].task_id;
        if (tid == NO_TASK) continue;

        TCB *t = find_task_by_id(sim, tid);
        if (!t || t->state != TASK_RUNNING) continue;

        t->remaining--;              /* Consome 1 tick de CPU */
        sim->cpus[c].quantum_used++; /* Avança o contador de quantum */
    }

    /* ── FASE 6b: Acumula wait_time para tarefas prontas sem CPU  */
    for (int i = 0; i < sim->num_tasks; i++) {
        if (sim->tasks[i].state == TASK_READY) {
            sim->tasks[i].wait_time++;
        }
    }

    /* ── FASE 7: Verifica se a simulação terminou  */
    int all_done = 1;
    for (int i = 0; i < sim->num_tasks; i++) {
        if (sim->tasks[i].state != TASK_FINISHED) {
            all_done = 0;
            break;
        }
    }

    /* ── FASE 8: Avança o relógio e salva snapshot  */
    sim->current_tick++;
    sim_take_snapshot(sim);

    if (all_done) {
        sim->finished = 1;
        printf("\n[Tick %3d] ═══ SIMULAÇÃO CONCLUÍDA ═══\n", sim->current_tick);
        return 1;
    }

    return 0;
}

/*
 * sim_step_back - Retrocede a simulação um tick
 */
int sim_step_back(SimState *sim) {
    if (sim->history_pos <= 0) {
        printf("[INFO] Já está no início da simulação (tick 0)\n");
        return -1;
    }

    sim->history_pos--;
    sim_restore_snapshot(sim, sim->history_pos);

    printf("[INFO] Retrocedeu para tick %d\n", sim->current_tick);
    return 0;
}

/*
 * sim_run_full - Executa a simulação completa (modo automático)
 */
void sim_run_full(SimState *sim) {
    printf("\n═══ INICIANDO SIMULAÇÃO COMPLETA ═══\n\n");

    /* Limite de segurança para evitar loop infinito em casos degenerados */
    int max_iter = MAX_TICKS;

    while (!sim->finished && max_iter-- > 0) {
        sim_step(sim);
    }

    if (!sim->finished) {
        fprintf(stderr, "[AVISO] Limite de %d ticks atingido sem conclusão\n", MAX_TICKS);
    }
}

/*
 * sim_print_status - Imprime o estado atual no terminal
 */
void sim_print_status(const SimState *sim) {
    printf("\n┌─── Estado em Tick %-3d ─────────────────────────┐\n", sim->current_tick);

    /* CPUs */
    printf("│ CPUs:\n");
    for (int c = 0; c < sim->num_cpus; c++) {
        const CpuState *cpu = &sim->cpus[c];
        if (cpu->is_off) {
            printf("│   CPU %d: [DESLIGADA]\n", c);
        } else if (cpu->task_id == NO_TASK) {
            printf("│   CPU %d: [ociosa]\n", c);
        } else {
            const TCB *t = NULL;
            for (int i = 0; i < sim->num_tasks; i++) {
                if (sim->tasks[i].id == cpu->task_id) { t = &sim->tasks[i]; break; }
            }
            printf("│   CPU %d: Tarefa %d (remaining=%d, quantum=%d/%d)\n",
                   c, cpu->task_id,
                   t ? t->remaining : -1,
                   cpu->quantum_used, sim->config->quantum);
        }
    }

    /* Tarefas */
    printf("│ Tarefas:\n");
    for (int i = 0; i < sim->num_tasks; i++) {
        const TCB *t = &sim->tasks[i];
        printf("│   [%d] %-10s rem=%-3d pri=%-3d",
               t->id, task_state_name(t->state), t->remaining, t->priority);
        if (t->state == TASK_RUNNING) {
            printf(" (CPU %d)", t->cpu_id);
        }
        printf("\n");
    }
    printf("└────────────────────────────────────────────────┘\n");
}

/*
 * sim_set_task_state - Modifica manualmente o estado de uma tarefa
 *
 * Implementa o requisito 1.5.2: o usuário pode mudar o estado
 * de qualquer tarefa em qualquer momento.
 *
 * Decisão de implementação:
 *   Verificamos transições inválidas (ex: não faz sentido ir de
 *   FINISHED para RUNNING) e avisamos o usuário.
 */
void sim_set_task_state(SimState *sim, int task_id, TaskState new_state) {
    TCB *t = find_task_by_id(sim, task_id);
    if (!t) {
        printf("[ERRO] Tarefa %d não encontrada\n", task_id);
        return;
    }

    TaskState old_state = t->state;

    /* Se a tarefa estava rodando em uma CPU, libera essa CPU */
    if (old_state == TASK_RUNNING && t->cpu_id != NO_TASK) {
        sim->cpus[t->cpu_id].task_id     = NO_TASK;
        sim->cpus[t->cpu_id].quantum_used = 0;
        t->cpu_id = NO_TASK;
    }

    t->state = new_state;

    /* Se for colocar em RUNNING, atribui a primeira CPU livre */
    if (new_state == TASK_RUNNING) {
        for (int c = 0; c < sim->num_cpus; c++) {
            if (sim->cpus[c].task_id == NO_TASK) {
                sim->cpus[c].task_id     = task_id;
                sim->cpus[c].quantum_used = 0;
                t->cpu_id = c;
                break;
            }
        }
    }

    task_add_history(t, sim->current_tick, new_state, t->cpu_id, EVENT_START);

    printf("[INFO] Tarefa %d: %s → %s (modificação manual)\n",
           task_id, task_state_name(old_state), task_state_name(new_state));
}

/*
 * sim_run_interactive - Modo passo a passo com interface de terminal
 *
 * O usuário controla a simulação com comandos simples.
 * A cada passo, o estado atual é exibido.
 */
void sim_run_interactive(SimState *sim) {
    char input[64];

    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║      MODO INTERATIVO (Passo a Passo)         ║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║ Comandos:                                    ║\n");
    printf("║   ENTER   → avança 1 tick                   ║\n");
    printf("║   b       → retrocede 1 tick                ║\n");
    printf("║   t <id>  → inspeciona tarefa (ex: t 2)     ║\n");
    printf("║   m <id>  → modifica estado de tarefa       ║\n");
    printf("║   s       → exibe estado atual              ║\n");
    printf("║   r       → executa até o final             ║\n");
    printf("║   q       → sai do modo interativo          ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    sim_print_status(sim);

    while (!sim->finished) {
        printf("\nTick %d > ", sim->current_tick);
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;

        /* Remove newline */
        input[strcspn(input, "\n")] = '\0';

        if (input[0] == '\0') {
            /* ENTER: avança um tick */
            sim_step(sim);
            sim_print_status(sim);

        } else if (input[0] == 'b') {
            /* b: retrocede */
            sim_step_back(sim);
            sim_print_status(sim);

        } else if (input[0] == 's') {
            /* s: mostra estado */
            sim_print_status(sim);

        } else if (input[0] == 't' && input[1] == ' ') {
            /* t <id>: inspeciona tarefa */
            int id = atoi(input + 2);
            TCB *t = find_task_by_id(sim, id);
            if (t) task_print_info(t);
            else printf("[ERRO] Tarefa %d não encontrada\n", id);

        } else if (input[0] == 'm' && input[1] == ' ') {
            /* m <id>: modifica estado de tarefa */
            int id = atoi(input + 2);
            printf("Novo estado (0=NEW, 1=READY, 2=RUNNING, 3=SUSPENDED, 4=FINISHED): ");
            fflush(stdout);
            char state_buf[16];
            if (fgets(state_buf, sizeof(state_buf), stdin)) {
                int new_state = atoi(state_buf);
                if (new_state >= 0 && new_state <= 4) {
                    sim_set_task_state(sim, id, (TaskState)new_state);
                } else {
                    printf("[ERRO] Estado inválido: %d\n", new_state);
                }
            }

        } else if (input[0] == 'r') {
            /* r: executa até o final */
            printf("[INFO] Executando até o final...\n");
            while (!sim->finished) {
                sim_step(sim);
            }
            sim_print_status(sim);

        } else if (input[0] == 'q') {
            printf("[INFO] Saindo do modo interativo\n");
            break;

        } else {
            printf("[AJUDA] Comando não reconhecido. Use ENTER, b, t, m, s, r ou q\n");
        }
    }
}

/*
 * sim_print_metrics - Imprime métricas finais da simulação
 */
void sim_print_metrics(const SimState *sim) {
    printf("\n╔══════════════════════════════════════════════════╗\n");
    printf("║             MÉTRICAS FINAIS DA SIMULAÇÃO         ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  ID │ Chegada │ Duração │ Turnaround │  Espera  ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");

    int total_turnaround = 0;
    int total_wait = 0;

    for (int i = 0; i < sim->num_tasks; i++) {
        const TCB *t = &sim->tasks[i];
        printf("║  %-3d│ %-7d │ %-7d │ %-10d │ %-8d ║\n",
               t->id, t->arrival, t->duration, t->turnaround, t->wait_time);
        total_turnaround += t->turnaround;
        total_wait       += t->wait_time;
    }

    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║ Turnaround médio : %-29.2f ║\n",
           sim->num_tasks > 0 ? (double)total_turnaround / sim->num_tasks : 0.0);
    printf("║ Espera média     : %-29.2f ║\n",
           sim->num_tasks > 0 ? (double)total_wait / sim->num_tasks : 0.0);
    printf("║ Duração total    : %-29d ║\n", sim->current_tick);
    printf("╚══════════════════════════════════════════════════╝\n");
}
