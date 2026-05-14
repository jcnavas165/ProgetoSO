
#ifndef TASK_H
#define TASK_H

/* Tamanho máximo de cor no formato RGB hexadecimal, ex: "F0E0D0" */
#define COLOR_LEN 7

/* Quantidade máxima de tarefas suportadas na simulação */
#define MAX_TASKS 256

/* Quantidade máxima de CPUs/processadores suportados */
#define MAX_CPUS 100

/* Valor especial que indica "nenhuma tarefa" em uma CPU */
#define NO_TASK -1


typedef enum {
    TASK_NEW       = 0,  /* Tarefa criada mas ainda não chegou no sistema */
    TASK_READY     = 1,  /* Tarefa na fila de prontos, aguardando CPU */
    TASK_RUNNING   = 2,  /* Tarefa atualmente executando em alguma CPU */
    TASK_SUSPENDED = 3,  /* Tarefa suspensa */
    TASK_FINISHED  = 4   /* Tarefa concluiu sua execução */
} TaskState;


typedef enum {
    EVENT_ARRIVAL,    /* Tarefa chegou no sistema */
    EVENT_START,      /* Tarefa começou/retomou execução numa CPU */
    EVENT_PREEMPT,    /* Tarefa foi preemptada (tirada da CPU) */
    EVENT_FINISH,     /* Tarefa terminou sua execução */
    EVENT_SUSPEND,    /* Tarefa foi suspensa (aguardando recurso) */
    EVENT_RESUME,     /* Tarefa foi retomada após suspensão */
    EVENT_LOTTERY     /* Desempate foi resolvido por sorteio */
} EventType;


typedef struct {
    int        tick;      /* Instante de tempo (tick do relógio global) */
    TaskState  state;     /* Estado da tarefa neste instante */
    int        cpu_id;    /* ID da CPU onde executou (-1 se não estava rodando) */
    EventType  event;     /* Tipo do evento que ocorreu neste instante */
} HistoryEntry;

/* Número máximo de entradas de histórico por tarefa */
#define MAX_HISTORY 512


typedef struct {
    /* ── Parâmetros estáticos (configurados antes da simulação) ─────────── */
    int   id;                /* Identificador único da tarefa (>= 1) */
    char  color[COLOR_LEN];  /* Cor RGB hex para o gráfico de Gantt, ex: "FF8000" */
    int   arrival;           /* Tick em que a tarefa chega no sistema */
    int   duration;          /* Tempo total de CPU necessário para terminar */
    int   priority;          /* Prioridade estática (maior = mais prioritária) */

    /* ── Estado dinâmico (muda durante a simulação) ──────────────────────── */
    TaskState state;         /* Estado atual da tarefa */
    int       remaining;     /* Tempo restante de CPU para terminar (usado pelo SRTF) */
    int       cpu_id;        /* ID da CPU onde está executando (-1 se não está) */
    int       start_tick;    /* Tick em que a tarefa começou a executar (1ª vez) */
    int       finish_tick;   /* Tick em que a tarefa terminou (-1 se ainda não terminou) */
    int       wait_time;     /* Tempo total que ficou na fila de prontos (métrica) */
    int       turnaround;    /* Tempo total desde chegada até término (métrica) */

    /* ── Histórico de execução (para Gantt e retroceder simulação) ────────── */
    HistoryEntry history[MAX_HISTORY]; /* Array de entradas de histórico */
    int          history_count;        /* Quantidade de entradas usadas */
} TCB;

void task_init(TCB *t, int id, const char *color, int arrival, int duration, int priority);

void task_add_history(TCB *t, int tick, TaskState state, int cpu_id, EventType event);

const char *task_state_name(TaskState state);

void task_print_info(const TCB *t);

#endif /* TASK_H */
