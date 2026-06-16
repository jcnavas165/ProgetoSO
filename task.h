/*
 * projeto A SO
 * Autores: Julio Cesar Navas e Nathálya Chaves 
 *
 * task.h - Definições da estrutura TCB (Task Control Block) e estados de tarefas
 *
 * O TCB é a estrutura central do simulador. Ele armazena TODAS as informações
 * de uma tarefa: desde sua criação até o seu término. É equivalente ao que
 * um sistema operacional real manteria na memória para cada processo/thread.
 *
 */

#ifndef TASK_H
#define TASK_H

#include "actions.h"

/* Tamanho máximo de cor no formato RGB hexadecimal, ex: "F0E0D0" */
#define COLOR_LEN 7

/* Quantidade máxima de tarefas suportadas na simulação */
#define MAX_TASKS 256

/* Quantidade máxima de CPUs/processadores suportados */
#define MAX_CPUS 100

/* Valor especial que indica "nenhuma tarefa" em uma CPU */
#define NO_TASK -1

/*
 * TaskState - Enumeração dos possíveis estados de uma tarefa
 *
 * Um sistema operacional gerencia tarefas através de uma máquina de estados.
 * Cada tarefa pode estar em exatamente um desses estados em qualquer instante:
 *
 *  NEW ──► READY ──► RUNNING ──► FINISHED
 *            ▲          │
 *            └──────────┘  (preempção / quantum expirado)
 *                 │
 *                 ▼
 *            SUSPENDED  
 */
typedef enum {
    TASK_NEW       = 0,  /* Tarefa criada mas ainda não chegou no sistema */
    TASK_READY     = 1,  /* Tarefa na fila de prontos, aguardando CPU */
    TASK_RUNNING   = 2,  /* Tarefa atualmente executando em alguma CPU */
    TASK_SUSPENDED = 3,  /* Tarefa suspensa */
    TASK_FINISHED  = 4   /* Tarefa concluiu sua execução */
} TaskState;

/*
 * TaskEvent - Representa um evento pontual no histórico de uma tarefa
 *
 * Durante a simulação, precisamos registrar o que aconteceu com cada tarefa
 * em cada instante. Isso é usado para:
 *   - Gerar o gráfico de Gantt
 *   - Permitir retroceder a simulação (passo a passo)
 *   - Exibir informações de debug
 */
typedef enum {
    EVENT_ARRIVAL,    /* Tarefa chegou no sistema */
    EVENT_START,      /* Tarefa começou/retomou execução numa CPU */
    EVENT_PREEMPT,    /* Tarefa foi preemptada (tirada da CPU) */
    EVENT_FINISH,     /* Tarefa terminou sua execução */
    EVENT_SUSPEND,    /* Tarefa foi suspensa (aguardando recurso) */
    EVENT_RESUME,     /* Tarefa foi retomada após suspensão */
    EVENT_LOTTERY,    /* Desempate foi resolvido por sorteio */
    EVENT_MUTEX_LOCK, /* Tentativa de lock de mutex */
    EVENT_MUTEX_UNLOCK, /* Unlock de mutex */
    EVENT_IO_START,   /* Início de operação de E/S */
    EVENT_IO_END      /* Término de operação de E/S */
} EventType;

/*
 * HistoryEntry - Entrada no histórico de execução de uma tarefa
 *
 * A cada tick do relógio, registramos o que aconteceu. Isso forma uma
 * "linha do tempo" de cada tarefa, usada para gerar o gráfico de Gantt
 * e para a funcionalidade de retroceder a simulação.
 */
typedef struct {
    int        tick;      /* Instante de tempo (tick do relógio global) */
    TaskState  state;     /* Estado da tarefa neste instante */
    int        cpu_id;    /* ID da CPU onde executou (-1 se não estava rodando) */
    EventType  event;     /* Tipo do evento que ocorreu neste instante */
} HistoryEntry;

/* Número máximo de entradas de histórico por tarefa */
#define MAX_HISTORY 512

/*
 * TCB - Task Control Block (Bloco de Controle de Tarefa)
 *
 * Esta é a estrutura mais importante do simulador. Ela armazena:
 *   - Parâmetros estáticos (lidos do arquivo de configuração)
 *   - Estado dinâmico (atualizado durante a simulação)
 *   - Histórico completo de execução (para o Gantt e para retroceder)
 *
 * Em um SO real, cada processo/thread teria um PCB (Process Control Block)
 * similar a esta estrutura.
 */
typedef struct TCB {
    /* ── Parâmetros estáticos (configurados antes da simulação) ─────────── */
    int   id;                /* Identificador único da tarefa (>= 1) */
    char  color[COLOR_LEN];  /* Cor RGB hex para o gráfico de Gantt, ex: "FF8000" */
    int   arrival;           /* Tick em que a tarefa chega no sistema */
    int   duration;          /* Tempo total de CPU necessário para terminar */
    int   priority;          /* Prioridade estática (maior = mais prioritária) */
    int   dynamic_priority;  /* Prioridade dinâmica (com envelhecimento para PRIOPEnv) */

    /* ── Ações (mutex e I/O) ──────────────────────────────────────────── */
    TaskAction actions[MAX_ACTIONS_PER_TASK]; /* Array de ações */
    int        action_count;                   /* Quantidade de ações */
    int        next_action_idx;                /* Índice da próxima ação a executar */

    /* ── Estado dinâmico (muda durante a simulação) ──────────────────────── */
    TaskState state;         /* Estado atual da tarefa */
    int       remaining;     /* Tempo restante de CPU para terminar (usado pelo SRTF) */
    int       cpu_id;        /* ID da CPU onde está executando (-1 se não está) */
    int       start_tick;    /* Tick em que a tarefa começou a executar (1ª vez) */
    int       finish_tick;   /* Tick em que a tarefa terminou (-1 se ainda não terminou) */
    int       wait_time;     /* Tempo total que ficou na fila de prontos (métrica) */
    int       turnaround;    /* Tempo total desde chegada até término (métrica) */
    
    /* ── Suspensão e sincronização ────────────────────────────────────── */
    SuspendReason suspend_reason; /* Por que foi suspensa */
    int           suspended_mutex_id; /* Qual mutex a suspendeu (-1 se nenhum) */
    int           io_end_tick;    /* Tick em que E/S vai terminar (-1 se não em E/S) */
    int           suspended_tick; /* Tick em que foi suspensa */

    /* ── Histórico de execução (para Gantt e retroceder simulação) ────────── */
    HistoryEntry history[MAX_HISTORY]; /* Array de entradas de histórico */
    int          history_count;        /* Quantidade de entradas usadas */
} TCB;

/* ── Funções de gerenciamento de tarefas ──────────────────────────────────── */

/*
 * task_init - Inicializa um TCB com os parâmetros fornecidos
 *
 * Deve ser chamada uma vez para cada tarefa, antes de iniciar a simulação.
 * Configura os valores estáticos e inicializa os campos dinâmicos com
 * valores padrão seguros.
 *
 * Parâmetros:
 *   t        - Ponteiro para o TCB a inicializar
 *   id       - ID único da tarefa
 *   color    - String da cor em RGB hex (ex: "FF8000")
 *   arrival  - Tick de chegada
 *   duration - Duração total de CPU necessária
 *   priority - Prioridade estática
 */
void task_init(TCB *t, int id, const char *color, int arrival, int duration, int priority);

/*
 * task_add_history - Registra um evento no histórico da tarefa
 *
 * Chamada toda vez que algo relevante acontece com a tarefa durante
 * a simulação (chegada, início de execução, preempção, etc.)
 *
 * Parâmetros:
 *   t      - Ponteiro para o TCB
 *   tick   - Instante de tempo atual
 *   state  - Novo estado da tarefa
 *   cpu_id - CPU onde está executando (ou -1)
 *   event  - Tipo do evento
 */
void task_add_history(TCB *t, int tick, TaskState state, int cpu_id, EventType event);

/*
 * task_state_name - Retorna string legível para o estado da tarefa
 *
 * Útil para exibir informações de debug e no modo passo a passo.
 */
const char *task_state_name(TaskState state);

/*
 * task_print_info - Imprime informações detalhadas de uma tarefa no terminal
 *
 * Usado no modo passo a passo para inspecionar o estado de uma tarefa.
 */
void task_print_info(const TCB *t);

#endif /* TASK_H */
