/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves  
 *
 * task.c - Implementação das funções de gerenciamento de tarefas (TCB)
 *
 * Este arquivo implementa as operações sobre o Task Control Block.
 * Cada função é responsável por uma operação específica e bem definida,
 * seguindo o princípio de responsabilidade única.
 *
 */

#include <stdio.h>
#include <string.h>
#include "task.h"

/*
 * task_init - Inicializa um TCB com os parâmetros fornecidos
 *
 * Decisão de implementação:
 *   Inicializamos 'remaining' igual a 'duration' porque no início
 *   da simulação nenhuma CPU foi consumida ainda. O campo 'remaining'
 *   vai diminuindo a cada tick que a tarefa executa.
 *
 *   cpu_id = NO_TASK (-1) indica que a tarefa não está em nenhuma CPU.
 *   finish_tick = -1 indica que a tarefa ainda não terminou.
 */
void task_init(TCB *t, int id, const char *color, int arrival, int duration, int priority) {
    /* Parâmetros estáticos - vêm do arquivo de configuração */
    t->id       = id;
    t->arrival  = arrival;
    t->duration = duration;
    t->priority = priority;

    /* Copia a cor com segurança (strncpy evita overflow de buffer) */
    strncpy(t->color, color, COLOR_LEN - 1);
    t->color[COLOR_LEN - 1] = '\0'; /* Garante terminação nula */

    /* Estado inicial: tarefa criada mas ainda não chegou no sistema */
    t->state      = TASK_NEW;
    t->remaining  = duration; /* No início, tempo restante = duração total */
    t->cpu_id     = NO_TASK;  /* Não está em nenhuma CPU ainda */
    t->start_tick = -1;       /* Ainda não começou a executar */
    t->finish_tick = -1;      /* Ainda não terminou */
    t->wait_time  = 0;        /* Contador de tempo de espera na fila */
    t->turnaround = 0;        /* Será calculado quando a tarefa terminar */

    /* Histórico vazio - será preenchido durante a simulação */
    t->history_count = 0;
}

/*
 * task_add_history - Registra um evento no histórico da tarefa
 *
 * Decisão de implementação:
 *   Verificamos se o array de histórico está cheio antes de inserir.
 *   Se estiver cheio, imprimimos um aviso mas não travamos o programa
 *   (comportamento defensivo). Em produção, poderíamos realocar dinamicamente,
 *   mas para um simulador educacional, um limite fixo é mais simples e seguro.
 */
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

/*
 * task_state_name - Retorna string legível para o estado da tarefa
 *
 * Decisão de implementação:
 *   Retornamos strings literais (ponteiros para memória estática do programa).
 *   Isso é seguro e eficiente - não precisamos alocar/liberar memória.
 *   O chamador NUNCA deve tentar liberar (free()) o ponteiro retornado.
 */
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

/*
 * task_print_info - Imprime informações detalhadas de uma tarefa no terminal
 *
 * Exibe todos os campos relevantes do TCB de forma legível.
 * Usado no modo passo a passo para debug/inspeção.
 */
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
