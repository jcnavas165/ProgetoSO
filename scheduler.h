/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves

 scheduler.h - Interface dos algoritmos de escalonamento
 *
 * Este módulo implementa os algoritmos de escalonamento de tarefas.
 * O design é propositalmente modular: cada algoritmo é uma função
 * com a mesma assinatura (SchedulerFunc), permitindo adicionar novos
 * algoritmos sem modificar o código da simulação (requisito 4.2).
 *
 * Algoritmos implementados no Projeto A:
 *   - SRTF  (Shortest Remaining Time First) - preemptivo
 *   - PRIOP (Prioridade Preemptivo)
 *
 * Regras de desempate (requisito 4.3):
 *   1. Tarefa que já estava executando (evita troca de contexto desnecessária)
 *   2. Menor instante de chegada (quem chegou antes)
 *   3. Menor duração total
 *   4. Sorteio (quando aplicado, deve ser registrado no histórico)
 *
 * Para PRIOP, antes dos critérios acima vem (requisito 4.4):
 *   0. Maior prioridade estática
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include "config.h"

/*
 * SchedContext - Contexto passado para o escalonador a cada chamada
 *
 * Agrupa tudo que o escalonador precisa saber para tomar a decisão:
 *   - Array de todas as tarefas
 *   - Quais CPUs estão ocupadas e com qual tarefa
 *   - Tick atual (para critérios de desempate)
 *   - Quantum configurado
 */
typedef struct {
    TCB *tasks;          /* Array de todos os TCBs */
    int  num_tasks;      /* Quantidade total de tarefas */
    int  current_tick;   /* Tick atual do relógio global */
    int  quantum;        /* Quantum de tempo configurado */
    int  alpha;          /* Parâmetro alpha para envelhecimento (PRIOPEnv) */
    int  cpu_tasks[MAX_CPUS]; /* ID da tarefa em cada CPU (-1 = CPU livre) */
    int  num_cpus;       /* Número de CPUs disponíveis */
} SchedContext;

/*
 * SchedResult - Resultado de uma decisão de escalonamento
 *
 * Retorna qual tarefa deve executar em cada CPU.
 * Se next_task_id[i] == NO_TASK, a CPU i deve ser desligada/ociosa.
 */
typedef struct {
    int  next_task_id[MAX_CPUS]; /* ID da tarefa a colocar em cada CPU */
    int  used_lottery;           /* 1 se foi necessário sorteio, 0 caso contrário */
} SchedResult;

/*
 * SchedulerFunc - Tipo de ponteiro para função de escalonamento
 *
 * Esta é a "interface" do escalonador. Todo algoritmo deve ter
 * exatamente esta assinatura. Isso permite trocar o algoritmo
 * em tempo de execução simplesmente mudando o ponteiro de função.
 *
 * Parâmetros:
 *   ctx - Contexto atual da simulação
 *   res - Resultado: qual tarefa vai para qual CPU
 */
typedef void (*SchedulerFunc)(const SchedContext *ctx, SchedResult *res);

/* ── Algoritmos de escalonamento ──────────────────────────────────────────── */

/*
 * sched_srtf - Shortest Remaining Time First (preemptivo)
 *
 * Sempre escolhe a tarefa com MENOR tempo restante de execução.
 * É preemptivo: se uma tarefa nova chega com tempo menor do que
 * a que está executando, a nova tarefa toma a CPU.
 *
 * Complexidade: O(n) por chamada, onde n = número de tarefas prontas.
 */
void sched_srtf(const SchedContext *ctx, SchedResult *res);

/*
 * sched_priop - Prioridade Preemptivo
 *
 * Sempre escolhe a tarefa com MAIOR prioridade estática.
 * É preemptivo: se uma tarefa nova chega com prioridade maior,
 * ela toma a CPU da tarefa atual.
 *
 * Desempate: prioridade → critérios gerais (4.3)
 */
void sched_priop(const SchedContext *ctx, SchedResult *res);

/*
 * sched_priopenv - Prioridade Preemptivo com Envelhecimento
 *
 * Similar ao PRIOP, mas usa prioridade dinâmica que aumenta com o tempo.
 * A prioridade dinâmica envelhece de acordo com o parâmetro alpha:
 *   dynamic_priority = static_priority + (current_time - arrival) / alpha
 *
 * Isto resolve o problema de starvation: tarefas de baixa prioridade
 * eventualmente envelhecem e adquirem prioridade suficiente para executar.
 */
void sched_priopenv(const SchedContext *ctx, SchedResult *res);

/*
 * scheduler_get - Retorna o ponteiro para a função do algoritmo escolhido
 *
 * Esta função implementa o requisito 4.2: o mecanismo de seleção do
 * algoritmo é configurável. O simulador não chama os algoritmos
 * diretamente, mas obtém o ponteiro via esta função.
 *
 * Exemplo de uso:
 *   SchedulerFunc sched = scheduler_get(ALGO_SRTF);
 *   sched(&ctx, &result);  // Chama o SRTF
 *
 * Parâmetros:
 *   algo - Enum do algoritmo desejado
 *
 * Retorno:
 *   Ponteiro para a função do algoritmo, ou NULL se desconhecido
 */
SchedulerFunc scheduler_get(SchedAlgo algo);

#endif /* SCHEDULER_H */
