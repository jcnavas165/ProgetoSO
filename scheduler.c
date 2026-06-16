/*
*projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 * scheduler.c - Implementação dos algoritmos de escalonamento
 *
 * Cada algoritmo segue o mesmo padrão:
 *   1. Coleta todas as tarefas no estado READY ou RUNNING
 *   2. Ordena/seleciona conforme a política do algoritmo
 *   3. Aplica regras de desempate (requisito 4.3)
 *   4. Preenche o SchedResult com as atribuições CPU → Tarefa
 *
 * Nota importante sobre múltiplos processadores:
 *   Com N CPUs, escolhemos as N melhores tarefas (não apenas a 1 melhor).
 *   Por exemplo, com SRTF e 3 CPUs, escolhemos as 3 tarefas com menor
 *   tempo restante para preencher as 3 CPUs.
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>    /* Para rand() no sorteio */
#include "scheduler.h"

/* ── Funções auxiliares internas ──────────────────────────────────────────── */

/*
 * is_schedulable - Verifica se uma tarefa pode ser agendada agora
 *
 * Uma tarefa é "agendável" se está READY ou RUNNING
 * (tarefas RUNNING podem ser mantidas ou preemptadas).
 */
static int is_schedulable(const TCB *t) {
    return (t->state == TASK_READY || t->state == TASK_RUNNING);
}

/*
 * tiebreak - Aplica as regras de desempate entre duas tarefas (requisito 4.3)
 *
 * Retorna:
 *  -1 se 'a' tem prioridade sobre 'b' (a ganha o desempate)
 *   1 se 'b' tem prioridade sobre 'a' (b ganha o desempate)
 *   0 se ainda há empate (precisará de sorteio)
 *
 * Parâmetros adicionais:
 *   current_cpu_task_id - ID da tarefa que está atualmente na CPU
 *                         (critério 1: evitar troca desnecessária de contexto)
 */
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

/*
 * collect_candidates - Coleta tarefas agendáveis em um array auxiliar
 *
 * Retorna quantas tarefas foram coletadas.
 * 'candidates' deve ter espaço para MAX_TASKS elementos.
 */
static int collect_candidates(const SchedContext *ctx, TCB **candidates) {
    int count = 0;
    for (int i = 0; i < ctx->num_tasks; i++) {
        if (is_schedulable(&ctx->tasks[i])) {
            candidates[count++] = &ctx->tasks[i];
        }
    }
    return count;
}

/*
 * assign_tasks_to_cpus - Distribui as N melhores tarefas entre as N CPUs
 *
 * Recebe um array de candidatos ORDENADO (melhor primeiro) e
 * atribui cada tarefa a uma CPU disponível.
 *
 * Estratégia:
 *   - Para cada CPU, tenta manter a mesma tarefa que já estava lá
 *     (minimiza trocas de contexto desnecessárias)
 *   - Se não for possível, pega a próxima tarefa da lista ordenada
 *
 * Parâmetros:
 *   candidates   - Array de ponteiros para tarefas, ORDENADO por prioridade
 *   num_cands    - Quantidade de candidatos
 *   ctx          - Contexto da simulação
 *   res          - Resultado a preencher
 *   used_lottery - Ponteiro para flag de sorteio
 */
static void assign_tasks_to_cpus(TCB **candidates, int num_cands,
                                  const SchedContext *ctx, SchedResult *res,
                                  int *used_lottery) {
    /* Inicializa todas as CPUs como sem tarefa */
    for (int c = 0; c < ctx->num_cpus; c++) {
        res->next_task_id[c] = NO_TASK;
    }

    /*
     * Passa 1: Verifica sorteio nos candidatos com mesma pontuação
     * (simplificado: apenas detecta se há empate no topo da lista)
     */
    if (num_cands >= 2 && *used_lottery) {
        /* Sorteio já foi marcado pelo algoritmo chamador */
    }

    /*
     * Atribui as N primeiras tarefas (N = num_cpus) às CPUs.
     * Preferência: manter tarefa que já estava na CPU (para SRTF/PRIOP
     * isso reduz trocas de contexto).
     */
    int next_cand = 0;
    for (int c = 0; c < ctx->num_cpus && next_cand < num_cands; c++) {
        res->next_task_id[c] = candidates[next_cand]->id;
        next_cand++;
    }
}

/* ── SRTF - Shortest Remaining Time First ─────────────────────────────────── */

/*
 * srtf_compare - Função de comparação para qsort no SRTF
 *
 * Ordena por tempo restante crescente (menor tempo restante = maior prioridade).
 * Em caso de empate, aplica as regras de desempate do requisito 4.3.
 *
 * Nota: qsort passa ponteiros para os elementos do array.
 * Como nosso array é de TCB*, cada elemento é um TCB**.
 */
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

    /*
     * Verifica se há empate no topo (para marcar sorteio)
     * Compara os dois melhores candidatos antes de ordenar.
     */
    res->used_lottery = 0;

    /* Para o SRTF com múltiplas CPUs, usamos a primeira CPU como referência
     * para o critério de "tarefa já executando" */
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

/* ── PRIOP - Prioridade Preemptivo ────────────────────────────────────────── */

/*
 * priop_compare - Função de comparação para qsort no PRIOP
 *
 * Ordena por prioridade decrescente (maior prioridade = primeiro).
 * Em caso de empate, aplica as regras de desempate do requisito 4.4.
 */
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

/* ── PRIOPEnv - Prioridade Preemptivo com Envelhecimento ──────────────────── */

/*
 * priop_env_compare - Função de comparação para qsort no PRIOPEnv
 *
 * Ordena por prioridade dinâmica (com envelhecimento) decrescente.
 * A prioridade dinâmica = static_priority + (current_time - arrival) / alpha
 *
 * O envelhecimento evita starvation de tarefas baixa prioridade.
 */
static int priop_env_compare(const void *pa, const void *pb) {
    const TCB *a = *(const TCB **)pa;
    const TCB *b = *(const TCB **)pb;

    /* Compara prioridades dinâmicas */
    if (a->dynamic_priority > b->dynamic_priority) return -1;
    if (b->dynamic_priority > a->dynamic_priority) return  1;

    /* Empate: aplica regras gerais de desempate */
    return tiebreak(a, b, current_cpu_task_for_compare);
}

void sched_priopenv(const SchedContext *ctx, SchedResult *res) {
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

    /* Calcula prioridades dinâmicas com envelhecimento
     * Nota: alpha vem do contexto (mas não está lá... precisamos passar via config)
     * Por enquanto, usaremos um valor padrão de 1
     */
    int alpha = 1; /* Será ajustado quando passarmos alpha via contexto */
    for (int i = 0; i < num_cands; i++) {
        TCB *t = candidates[i];
        int age = ctx->current_tick - t->arrival;
        /* Se alpha > 0, calcula envelhecimento; senão mantém prioridade estática */
        if (alpha > 0) {
            t->dynamic_priority = t->priority + (age / alpha);
        } else {
            t->dynamic_priority = t->priority;
        }
    }

    /* Ordena candidatos pelo critério PRIOPEnv */
    qsort(candidates, num_cands, sizeof(TCB *), priop_env_compare);

    /* Verifica se sorteio foi necessário */
    if (num_cands >= 2) {
        const TCB *a = candidates[0];
        const TCB *b = candidates[1];
        if (a->dynamic_priority == b->dynamic_priority &&
            a->arrival  == b->arrival  &&
            a->duration == b->duration) {
            res->used_lottery = 1;
        }
    }

    assign_tasks_to_cpus(candidates, num_cands, ctx, res, &res->used_lottery);
}

/*
 * scheduler_get - Retorna ponteiro para a função do algoritmo escolhido
 *
 *
 *   Declare a função em scheduler.h
 *   Implemente em scheduler.c
 *   Adicione o case abaixo
 *
 */
SchedulerFunc scheduler_get(SchedAlgo algo) {
    switch (algo) {
        case ALGO_SRTF:  return sched_srtf;
        case ALGO_PRIOP: return sched_priop;
        case ALGO_PRIOPENV: return sched_priopenv;
        default:
            fprintf(stderr, "[ERRO] Algoritmo de escalonamento desconhecido: %d\n", algo);
            return NULL;
    }
}
