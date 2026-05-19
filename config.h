/*
 * config.h - Definições para leitura e armazenamento da configuração da simulação
 *
 * ALTERAÇÕES v2:
 *   - config_load_csv()          declarada (lê arquivo .csv com separador ',')
 *   - config_create_default_csv() declarada (gera arquivo CSV de exemplo)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "task.h"

/* Comprimento máximo de uma linha no arquivo de configuração */
#define MAX_LINE_LEN 512

/* Comprimento máximo do nome do algoritmo de escalonamento */
#define MAX_ALGO_NAME 32

/* SchedAlgo - Enumeração dos algoritmos de escalonamento suportados */
typedef enum {
    ALGO_SRTF  = 0,   /* Shortest Remaining Time First */
    ALGO_PRIOP = 1,   /* Prioridade Preemptivo */
    ALGO_UNKNOWN = -1
} SchedAlgo;

/* SimConfig - Configuração completa da simulação */
typedef struct {
    /* Parâmetros gerais (linha 1 do arquivo) */
    SchedAlgo algo;                    /* Algoritmo de escalonamento escolhido */
    char      algo_name[MAX_ALGO_NAME]; /* Nome original lido do arquivo */
    int       quantum;                 /* Quantum de tempo (fatia máxima de CPU) */
    int       num_cpus;                /* Número de CPUs/processadores disponíveis */

    /* Lista de tarefas (linhas 2 e demais do arquivo) */
    TCB tasks[MAX_TASKS];              /* Array de TCBs para todas as tarefas */
    int num_tasks;                     /* Quantidade de tarefas carregadas */
} SimConfig;

/* ── Funções de configuração ──────────────────────────────────────────────── */

/*
 * config_load - Carrega configuração de arquivo com separador ';' (ponto-e-vírgula)
 *
 * Aceita o formato original do projeto E o formato do caso-teste-mc:
 *   - IDs numéricos: "1", "2", ...
 *   - IDs com prefixo: "t01", "t02", ...  ← caso-teste-mc-005-priop.txt
 *   - ';' extra no final das linhas de tarefa é tolerado
 */
int config_load(SimConfig *cfg, const char *filename);

/*
 * [NOVA] config_load_csv - Carrega configuração de arquivo .csv (separador ',')
 *
 * Mesmo formato do .txt, mas usando vírgula como separador.
 * Compatível com arquivos exportados do Excel / LibreOffice / Google Sheets.
 *
 * Exemplo de arquivo CSV válido:
 *   SRTF,4,2
 *   1,FF4444,0,10,3
 *   t02,00FF00,14,10,6
 */
int config_load_csv(SimConfig *cfg, const char *filename);

/* config_print - Imprime no terminal a configuração carregada */
void config_print(const SimConfig *cfg);

/* config_create_default - Cria arquivo TXT de configuração de exemplo */
void config_create_default(const char *filename);

/*
 * [NOVA] config_create_default_csv - Cria arquivo CSV de configuração de exemplo
 *
 * Gera o mesmo conteúdo de config_create_default(), mas com vírgulas.
 * Útil como modelo para editar em planilha.
 */
void config_create_default_csv(const char *filename);

/* algo_name_to_enum - Converte string do nome do algoritmo para enum */
SchedAlgo algo_name_to_enum(const char *name);

/* algo_enum_to_name - Retorna o nome legível do algoritmo */
const char *algo_enum_to_name(SchedAlgo algo);

#endif /* CONFIG_H */