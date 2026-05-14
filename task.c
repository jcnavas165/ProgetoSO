#include <stdio.h>
#include <string.h>
#include "task.h"

void task_init(TCB *t, int id, const char *color, int arrival, int duration, int priority) {
   
    t->id       = id;
    t->arrival  = arrival;
    t->duration = duration;
    t->priority = priority;

    /* Copia a cor com segurança (strncpy evita overflow de buffer) */
    strncpy(t->color, color, COLOR_LEN - 1);
    t->color[COLOR_LEN - 1] = '\0'; /* Garante terminação nula */

    t->state      = TASK_NEW;
    t->remaining  = duration; /* No início, tempo restante = duração total */
    t->cpu_id     = NO_TASK;  /* Não está em nenhuma CPU ainda */
    t->start_tick = -1;       /* Ainda não começou a executar */
    t->finish_tick = -1;      /* Ainda não terminou */
    t->wait_time  = 0;        /* Contador de tempo de espera na fila */
    t->turnaround = 0;        /* Será calculado quando a tarefa terminar */

    t->history_count = 0;
}


void task_add_history(TCB *t, int tick, TaskState state, int cpu_id, EventType event) {
    /* Verifica se ainda há espaço no histórico */
    if (t->history_count >= MAX_HISTORY) {
        fprintf(stderr, "[AVISO] Tarefa %d: histórico cheio, evento no tick %d ignorado\n",
                t->id, tick);
        return;
    }

    /* Registra a entrada no próximo slot disponível */
    HistoryEntry *entry = &t->history[t->history_count];
    entry->tick   = tick;
    entry->state  = state;
    entry->cpu_id = cpu_id;
    entry->event  = event;

    t->history_count++; /* Avança o contador */
}


const char *task_state_name(TaskState state) {
    switch (state) {
        case TASK_NEW:       return "NOVA";
        case TASK_READY:     return "PRONTA";
        case TASK_RUNNING:   return "EXECUTANDO";
        case TASK_SUSPENDED: return "SUSPENSA";
        case TASK_FINISHED:  return "CONCLUÍDA";
        default:             return "DESCONHECIDO";
    }
}


void task_print_info(const TCB *t) {
    printf("┌─────────────────────────────────────────┐\n");
    printf("│ Tarefa ID: %-30d│\n", t->id);
    printf("├─────────────────────────────────────────┤\n");
    printf("│ Cor (RGB hex) : %-24s│\n", t->color);
    printf("│ Chegada       : tick %-19d│\n", t->arrival);
    printf("│ Duração total : %-24d│\n", t->duration);
    printf("│ Prioridade    : %-24d│\n", t->priority);
    printf("├─────────────────────────────────────────┤\n");
    printf("│ Estado atual  : %-24s│\n", task_state_name(t->state));
    printf("│ Tempo restante: %-24d│\n", t->remaining);

    if (t->cpu_id == NO_TASK) {
        printf("│ CPU atual     : (nenhuma)               │\n");
    } else {
        printf("│ CPU atual     : CPU %-20d│\n", t->cpu_id);
    }

    if (t->start_tick >= 0) {
        printf("│ Início exec.  : tick %-19d│\n", t->start_tick);
    } else {
        printf("│ Início exec.  : (ainda não executou)   │\n");
    }

    if (t->finish_tick >= 0) {
        printf("│ Fim exec.     : tick %-19d│\n", t->finish_tick);
        printf("│ Turnaround    : %-24d│\n", t->turnaround);
    } else {
        printf("│ Fim exec.     : (ainda não terminou)   │\n");
    }

    printf("│ Tempo espera  : %-24d│\n", t->wait_time);
    printf("│ Eventos reg.  : %-24d│\n", t->history_count);
    printf("└─────────────────────────────────────────┘\n");
}
