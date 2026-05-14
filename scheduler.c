
#include <stdio.h>
#include <stdlib.h>
#include <time.h>    /* Para rand() no sorteio */
#include "scheduler.h"


static int is_schedulable(const TCB *t) {
    return (t->state == TASK_READY || t->state == TASK_RUNNING);
}


static int tiebreak(const TCB *a, const TCB *b, int current_cpu_task_id) {
    /* Critério 1: Quem está executando agora fica (evita troca de contexto) */
    if (a->id == current_cpu_task_id && b->id != current_cpu_task_id) return -1;
    if (b->id == current_cpu_task_id && a->id != current_cpu_task_id) return  1;

    /* Critério 2: Quem chegou primeiro no sistema */
    if (a->arrival < b->arrival) return -1;
    if (b->arrival < a->arrival) return  1;

    /* Critério 3: Quem tem menor duração total */
    if (a->duration < b->duration) return -1;
    if (b->duration < a->duration) return  1;

    /* Critério 4: Sorteio (empate total) */
    return 0;
}


static int collect_candidates(const SchedContext *ctx, TCB **candidates) {
    int count = 0;
    for (int i = 0; i < ctx->num_tasks; i++) {
        if (is_schedulable(&ctx->tasks[i])) {
            candidates[count++] = &ctx->tasks[i];
        }
    }
    return count;
}


static void assign_tasks_to_cpus(TCB **candidates, int num_cands,
                                  const SchedContext *ctx, SchedResult *res,
                                  int *used_lottery) {
    /* Inicializa todas as CPUs como sem tarefa */
    for (int c = 0; c < ctx->num_cpus; c++) {
        res->next_task_id[c] = NO_TASK;
    }

    
    if (num_cands >= 2 && *used_lottery) {
        /* Sorteio já foi marcado pelo algoritmo chamador */
    }

    
    int next_cand = 0;
    for (int c = 0; c < ctx->num_cpus && next_cand < num_cands; c++) {
        res->next_task_id[c] = candidates[next_cand]->id;
        next_cand++;
    }
}



static int current_cpu_task_for_compare = NO_TASK; /* Variável global para o comparador */

static int srtf_compare(const void *pa, const void *pb) {
    const TCB *a = *(const TCB **)pa;
    const TCB *b = *(const TCB **)pb;

    /* Critério principal: menor tempo restante */
    if (a->remaining < b->remaining) return -1;
    if (b->remaining < a->remaining) return  1;

    /* Empate: aplica regras de desempate */
    return tiebreak(a, b, current_cpu_task_for_compare);
}

void sched_srtf(const SchedContext *ctx, SchedResult *res) {
    TCB *candidates[MAX_TASKS];
    int  num_cands = collect_candidates(ctx, candidates);

    /* Sem candidatos: todas as CPUs ficam ociosas */
    if (num_cands == 0) {
        for (int c = 0; c < ctx->num_cpus; c++) {
            res->next_task_id[c] = NO_TASK;
        }
        res->used_lottery = 0;
        return;
    }

    
    res->used_lottery = 0;

    
    current_cpu_task_for_compare = (ctx->num_cpus > 0) ? ctx->cpu_tasks[0] : NO_TASK;

    /* Ordena candidatos pelo critério SRTF */
    qsort(candidates, num_cands, sizeof(TCB *), srtf_compare);

    /* Verifica se sorteio foi necessário (empate total entre top candidatos) */
    if (num_cands >= 2) {
        const TCB *a = candidates[0];
        const TCB *b = candidates[1];
        if (a->remaining == b->remaining &&
            a->arrival   == b->arrival   &&
            a->duration  == b->duration) {
            res->used_lottery = 1;
        }
    }

    assign_tasks_to_cpus(candidates, num_cands, ctx, res, &res->used_lottery);
}



static int priop_compare(const void *pa, const void *pb) {
    const TCB *a = *(const TCB **)pa;
    const TCB *b = *(const TCB **)pb;

    /* Critério principal PRIOP: MAIOR prioridade primeiro */
    if (a->priority > b->priority) return -1;
    if (b->priority > a->priority) return  1;

    /* Empate de prioridade: aplica regras gerais de desempate */
    return tiebreak(a, b, current_cpu_task_for_compare);
}

void sched_priop(const SchedContext *ctx, SchedResult *res) {
    TCB *candidates[MAX_TASKS];
    int  num_cands = collect_candidates(ctx, candidates);

    /* Sem candidatos: todas as CPUs ficam ociosas */
    if (num_cands == 0) {
        for (int c = 0; c < ctx->num_cpus; c++) {
            res->next_task_id[c] = NO_TASK;
        }
        res->used_lottery = 0;
        return;
    }

    res->used_lottery = 0;
    current_cpu_task_for_compare = (ctx->num_cpus > 0) ? ctx->cpu_tasks[0] : NO_TASK;

    /* Ordena candidatos pelo critério PRIOP */
    qsort(candidates, num_cands, sizeof(TCB *), priop_compare);

    /* Verifica se sorteio foi necessário */
    if (num_cands >= 2) {
        const TCB *a = candidates[0];
        const TCB *b = candidates[1];
        if (a->priority == b->priority &&
            a->arrival  == b->arrival  &&
            a->duration == b->duration) {
            res->used_lottery = 1;
        }
    }

    assign_tasks_to_cpus(candidates, num_cands, ctx, res, &res->used_lottery);
}



SchedulerFunc scheduler_get(SchedAlgo algo) {
    switch (algo) {
        case ALGO_SRTF:  return sched_srtf;
        case ALGO_PRIOP: return sched_priop;
        default:
            fprintf(stderr, "[ERRO] Algoritmo de escalonamento desconhecido: %d\n", algo);
            return NULL;
    }
}
