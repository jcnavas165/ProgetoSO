/*
 * config.h - Definições para leitura e armazenamento da configuração da simulação
 *
 * O arquivo de configuração tem o seguinte formato (conforme especificação):
 *
 *   Linha 1: algoritmo_escalonamento;quantum;qtde_cpus
 *   Linha 2+: id;cor;ingresso;duracao;prioridade;lista_eventos
 *
 * Exemplo:
 *   SRTF;4;2
 *   1;FF0000;0;10;3
 *   2;00FF00;2;6;5
 *   3;0000FF;4;8;1
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "task.h"

/* Comprimento máximo de uma linha no arquivo de configuração */
#define MAX_LINE_LEN 512

/* Comprimento máximo do nome do algoritmo de escalonamento */
#define MAX_ALGO_NAME 32

/*
 * SchedAlgo - Enumeração dos algoritmos de escalonamento suportados
 *
 * Projeto A exige:
 *   - SRTF  (Shortest Remaining Time First) - preemptivo
 *   - PRIOP (Prioridade Preemptivo)
 */
typedef enum {
    ALGO_SRTF  = 0,  /* Shortest Remaining Time First */
    ALGO_PRIOP = 1,  /* Prioridade Preemptivo */
    ALGO_UNKNOWN = -1
} SchedAlgo;

/*
 * SimConfig - Configuração completa da simulação
 *
 * Armazena tudo que foi lido do arquivo de configuração:
 *   - Parâmetros gerais do sistema (linha 1)
 *   - Array de TCBs com todas as tarefas (linhas 2+)
 */
typedef struct {
    /* ── Parâmetros gerais (linha 1 do arquivo) ─────────────────────────── */
    SchedAlgo algo;              /* Algoritmo de escalonamento escolhido */
    char      algo_name[MAX_ALGO_NAME]; /* Nome original lido do arquivo */
    int       quantum;           /* Quantum de tempo (fatia máxima de CPU por tarefa) */
    int       num_cpus;          /* Número de CPUs/processadores disponíveis */

    /* ── Lista de tarefas (linhas 2+ do arquivo) ────────────────────────── */
    TCB tasks[MAX_TASKS];        /* Array de TCBs para todas as tarefas */
    int num_tasks;               /* Quantidade de tarefas carregadas */
} SimConfig;

/* ── Funções de configuração ──────────────────────────────────────────────── */

/*
 * config_load - Carrega a configuração a partir de um arquivo texto
 *
 * Lê o arquivo linha por linha, fazendo o parse de cada campo.
 * A função é tolerante a maiúsculas/minúsculas no nome do algoritmo
 * (ex: "srtf", "SRTF" e "Srtf" são equivalentes).
 *
 * Parâmetros:
 *   cfg      - Ponteiro para a estrutura SimConfig a preencher
 *   filename - Caminho do arquivo de configuração
 *
 * Retorno:
 *   0 em caso de sucesso
 *  -1 em caso de erro (arquivo não encontrado, formato inválido, etc.)
 */
int config_load(SimConfig *cfg, const char *filename);

/*
 * config_print - Imprime no terminal a configuração carregada
 *
 * Útil para confirmar que o arquivo foi lido corretamente antes
 * de iniciar a simulação.
 */
void config_print(const SimConfig *cfg);

/*
 * config_create_default - Cria um arquivo de configuração de exemplo
 *
 * Gera um arquivo "config_exemplo.txt" com valores padrão para o usuário
 * usar como base. Requisito 3.2 da especificação.
 *
 * Parâmetros:
 *   filename - Nome do arquivo a criar
 */
void config_create_default(const char *filename);

/*
 * algo_name_to_enum - Converte string do nome do algoritmo para enum
 *
 * Trata maiúsculas/minúsculas conforme requisito 3.3.2.
 * Retorna ALGO_UNKNOWN se o nome não for reconhecido.
 */
SchedAlgo algo_name_to_enum(const char *name);

/*
 * algo_enum_to_name - Retorna o nome legível do algoritmo
 */
const char *algo_enum_to_name(SchedAlgo algo);

#endif /* CONFIG_H */
