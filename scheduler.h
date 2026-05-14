#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include "config.h"


typedef struct {
    TCB *tasks;          /* Array de todos os TCBs */
    int  num_tasks;      /* Quantidade total de tarefas */
    int  current_tick;   /* Tick atual do relógio global */
    int  quantum;        /* Quantum de tempo configurado */
    int  cpu_tasks[MAX_CPUS]; /* ID da tarefa em cada CPU (-1 = CPU livre) */
    int  num_cpus;       /* Número de CPUs disponíveis */
} SchedContext;


typedef struct {
    int  next_task_id[MAX_CPUS]; /* ID da tarefa a colocar em cada CPU */
    int  used_lottery;           /* 1 se foi necessário sorteio, 0 caso contrário */
} SchedResult;


typedef void (*SchedulerFunc)(const SchedContext *ctx, SchedResult *res);

void sched_srtf(const SchedContext *ctx, SchedResult *res);


void sched_priop(const SchedContext *ctx, SchedResult *res);


SchedulerFunc scheduler_get(SchedAlgo algo);

#endif 
