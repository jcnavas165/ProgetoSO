

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "task.h"
#include "config.h"
#include "scheduler.h"

/* Número máximo de ticks que a simulação pode durar */
#define MAX_TICKS 512


typedef struct {
    int task_id;       /* ID da tarefa executando (NO_TASK = ociosa/desligada) */
    int quantum_used;  /* Ticks de quantum já consumidos pela tarefa atual */
    int is_off;        /* 1 = CPU desligada, 0 = CPU ligada (mesmo que ociosa) */
} CpuState;


typedef struct {
    int       tick;                  /* Instante de tempo desta foto */
    TCB       tasks[MAX_TASKS];      /* Cópia de todos os TCBs neste instante */
    int       num_tasks;             /* Número de tarefas */
    CpuState  cpus[MAX_CPUS];        /* Estado de cada CPU neste instante */
    int       num_cpus;              /* Número de CPUs */
} SimSnapshot;


typedef struct {
    /* ── Configuração (imutável após início)  */
    SimConfig    *config;          /* Ponteiro para a configuração carregada */
    SchedulerFunc scheduler;       /* Ponteiro para a função de escalonamento */

    /* ── Estado atual da simulação  */
    int          current_tick;     /* Tick atual do relógio global */
    TCB          tasks[MAX_TASKS]; /* Estado atual de todas as tarefas */
    int          num_tasks;        /* Número total de tarefas */
    CpuState     cpus[MAX_CPUS];   /* Estado atual de cada CPU */
    int          num_cpus;         /* Número de CPUs */
    int          finished;         /* 1 se todas as tarefas terminaram */

    /* ── Histórico de snapshots (para retroceder/avançar) ───────────────── */
    SimSnapshot *history;        /* Array alocado dinamicamente */
    int          history_count;      /* Quantidade de snapshots armazenados */
    int          history_pos;        /* Posição atual no histórico (para navegar) */
} SimState;

/* ── Funções do simulador  */


int sim_init(SimState *sim, SimConfig *config);


int sim_step(SimState *sim);

int sim_step_back(SimState *sim);

void sim_run_full(SimState *sim);

void sim_run_interactive(SimState *sim);

void sim_set_task_state(SimState *sim, int task_id, TaskState new_state);

void sim_print_status(const SimState *sim);

void sim_take_snapshot(SimState *sim);

void sim_restore_snapshot(SimState *sim, int pos);

void sim_print_metrics(const SimState *sim);

#endif