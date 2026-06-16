/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves 
 * 
 *simulator.h - Interface do motor da simulação
 *
 * Este módulo é o "coração" do simulador. Ele controla:
 *   - O relógio global do sistema (ticks)
 *   - O ciclo de vida das tarefas (chegada, execução, término)
 *   - As CPUs e quais tarefas estão nelas
 *   - O histórico completo de cada passo (para retroceder)
 *   - Os dois modos de execução: passo a passo e completo
 *
 * Fluxo de um tick de simulação:
 *   1. Verifica quais tarefas chegaram neste tick → READY
 *   2. Verifica quais tarefas terminaram → FINISHED
 *   3. Chama o escalonador para decidir próximas tarefas
 *   4. Atualiza CPUs, estados das tarefas, registra histórico
 *   5. Avança o quantum de cada tarefa rodando
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "task.h"
#include "config.h"
#include "scheduler.h"
#include "actions.h"

/* Número máximo de ticks que a simulação pode durar */
#define MAX_TICKS 512

/*
 * CpuState - Estado atual de uma CPU em um determinado tick
 *
 * Além de saber qual tarefa está rodando, precisamos saber:
 *   - Se está ociosa (ligada mas sem tarefa) ou desligada
 *   - Quanto tempo do quantum já foi consumido pela tarefa atual
 */
typedef struct {
    int task_id;       /* ID da tarefa executando (NO_TASK = ociosa/desligada) */
    int quantum_used;  /* Ticks de quantum já consumidos pela tarefa atual */
    int is_off;        /* 1 = CPU desligada, 0 = CPU ligada (mesmo que ociosa) */
} CpuState;

/*
 * SimSnapshot - "Fotografia" completa do estado da simulação em um tick
 *
 * Armazena o estado de TUDO em um determinado instante.
 * Isso é necessário para implementar o "retroceder" (requisito 1.5.2):
 * guardamos uma foto a cada passo e podemos restaurar qualquer foto anterior.
 *
 * Decisão de implementação:
 *   Armazenar snapshots completos consome mais memória do que armazenar
 *   apenas os deltas (diferenças), mas é muito mais simples de implementar
 *   e de entender. Para um simulador educacional, essa é a escolha certa.
 */
typedef struct {
    int       tick;                  /* Instante de tempo desta foto */
    TCB       tasks[MAX_TASKS];      /* Cópia de todos os TCBs neste instante */
    int       num_tasks;             /* Número de tarefas */
    CpuState  cpus[MAX_CPUS];        /* Estado de cada CPU neste instante */
    int       num_cpus;              /* Número de CPUs */
} SimSnapshot;

/*
 * SimState - Estado completo da simulação (estrutura principal do simulador)
 *
 * Agrega tudo: configuração, estado atual, histórico de snapshots.
 * Existe apenas UMA instância desta estrutura durante toda a execução.
 */
typedef struct {
    /* ── Configuração (imutável após início) ─────────────────────────────── */
    SimConfig    *config;          /* Ponteiro para a configuração carregada */
    SchedulerFunc scheduler;       /* Ponteiro para a função de escalonamento */

    /* ── Estado atual da simulação ──────────────────────────────────────── */
    int          current_tick;     /* Tick atual do relógio global */
    TCB          tasks[MAX_TASKS]; /* Estado atual de todas as tarefas */
    int          num_tasks;        /* Número total de tarefas */
    CpuState     cpus[MAX_CPUS];   /* Estado atual de cada CPU */
    int          num_cpus;         /* Número de CPUs */
    int          finished;         /* 1 se todas as tarefas terminaram */

    /* ── Sincronização (mutexes) ───────────────────────────────────────── */
    Mutex        mutexes[MAX_MUTEXES]; /* Array de mutexes */
    int          mutex_count;          /* Quantidade de mutexes em uso */

    /* ── Operações de E/S ──────────────────────────────────────────────── */
    int          io_tasks[256];    /* IDs de tarefas com I/O pendente */
    int          io_end_tick[256]; /* Ticks de término das I/Os */
    int          io_count;         /* Quantidade de I/Os ativas */

    /* ── Histórico de snapshots (para retroceder/avançar) ───────────────── */
    SimSnapshot *history;        /* Array alocado dinamicamente */
    int          history_count;      /* Quantidade de snapshots armazenados */
    int          history_pos;        /* Posição atual no histórico (para navegar) */
} SimState;

/* ── Funções do simulador ─────────────────────────────────────────────────── */

/*
 * sim_init - Inicializa o estado da simulação a partir da configuração
 *
 * Deve ser chamada uma vez antes de iniciar a simulação.
 * Copia as tarefas da configuração para o estado, inicializa CPUs, etc.
 *
 * Parâmetros:
 *   sim    - Ponteiro para o SimState a inicializar
 *   config - Configuração carregada do arquivo
 *
 * Retorno:
 *   0 em caso de sucesso, -1 em caso de erro
 */
int sim_init(SimState *sim, SimConfig *config);

/*
 * sim_step - Avança a simulação um tick para frente
 *
 * Esta é a função mais importante do simulador.
 * Executa um ciclo completo de um tick:
 *   1. Chegada de novas tarefas
 *   2. Término de tarefas que completaram
 *   3. Escalonamento
 *   4. Atualização de estados e quantum
 *   5. Snapshot do estado atual
 *
 * Retorno:
 *   0 se a simulação ainda não terminou
 *   1 se todas as tarefas terminaram (simulação completa)
 */
int sim_step(SimState *sim);

/*
 * sim_step_back - Retrocede a simulação um tick
 *
 * Restaura o snapshot do tick anterior (requisito 1.5.2).
 *
 * Retorno:
 *   0 se conseguiu retroceder
 *  -1 se já está no início (não pode retroceder mais)
 */
int sim_step_back(SimState *sim);

/*
 * sim_run_full - Executa a simulação completa sem intervenção
 *
 * Modo de execução completo (requisito 1.5.3).
 * Chama sim_step() repetidamente até todas as tarefas terminarem.
 */
void sim_run_full(SimState *sim);

/*
 * sim_run_interactive - Executa a simulação no modo passo a passo
 *
 * Modo interativo (requisito 1.5.1 e 1.5.2).
 * A cada passo, permite ao usuário:
 *   - Avançar um tick (ENTER)
 *   - Retroceder um tick (b + ENTER)
 *   - Inspecionar uma tarefa (t + ID + ENTER)
 *   - Modificar estado de uma tarefa (m + ENTER)
 *   - Ver estado das CPUs (c + ENTER)
 *   - Sair/pular para o final (q + ENTER)
 */
void sim_run_interactive(SimState *sim);

/*
 * sim_set_task_state - Modifica manualmente o estado de uma tarefa
 *
 * Implementa o requisito 1.5.2: o usuário pode modificar o estado
 * de qualquer tarefa em qualquer passo da simulação.
 *
 * Parâmetros:
 *   sim      - Estado atual da simulação
 *   task_id  - ID da tarefa a modificar
 *   new_state - Novo estado para a tarefa
 */
void sim_set_task_state(SimState *sim, int task_id, TaskState new_state);

/*
 * sim_print_status - Imprime o estado atual da simulação no terminal
 *
 * Exibe:
 *   - Tick atual
 *   - Estado de cada CPU
 *   - Estado de cada tarefa (resumido)
 */
void sim_print_status(const SimState *sim);

/*
 * sim_take_snapshot - Salva o estado atual no histórico
 *
 * Chamada internamente por sim_step() após cada tick.
 * Usuário externo normalmente não precisa chamar diretamente.
 */
void sim_take_snapshot(SimState *sim);

/*
 * sim_restore_snapshot - Restaura um snapshot específico
 *
 * Parâmetros:
 *   sim - Estado da simulação
 *   pos - Índice do snapshot a restaurar (0 = início)
 */
void sim_restore_snapshot(SimState *sim, int pos);

/*
 * sim_print_metrics - Imprime métricas finais da simulação
 *
 * Ao final da simulação, exibe:
 *   - Turnaround de cada tarefa
 *   - Tempo de espera de cada tarefa
 *   - Utilização de cada CPU
 */
void sim_print_metrics(const SimState *sim);

#endif /* SIMULATOR_H */
