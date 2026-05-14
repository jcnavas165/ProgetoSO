/*
 * config.h - Definições para leitura e armazenamento da configuração da simulação
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "task.h"

//Comprimento máximo de uma linha no arquivo de configuração 
#define MAX_LINE_LEN 512

// Comprimento máximo do nome do algoritmo de escalonamento 
#define MAX_ALGO_NAME 32

// SchedAlgo - Enumeração dos algoritmos de escalonamento suportados
typedef enum {
    ALGO_SRTF  = 0,  //Shortest Remaining Time First 
    ALGO_PRIOP = 1,  //Prioridade Preemptivo 
    ALGO_UNKNOWN = -1
} SchedAlgo;

//SimConfig - Configuração completa da simulação
typedef struct {
    // Parâmetros gerais (linha 1 do arquivo)
    SchedAlgo algo;              //Algoritmo de escalonamento escolhido 
    char      algo_name[MAX_ALGO_NAME]; //Nome original lido do arquivo 
    int       quantum;           //Quantum de tempo (fatia máxima de CPU por tarefa) 
    int       num_cpus;          // Número de CPUs/processadores disponíveis 

    //Lista de tarefas (linhas 2 e demais do arquivo)
    TCB tasks[MAX_TASKS];        //array de TCBs para todas as tarefas 
    int num_tasks;               //quantidade de tarefas carregadas 
} SimConfig;

//Funções de configuração

//config_load carrega a configuração a partir de um arquivo texto
// Parâmetros:cfg = Ponteiro para a estrutura SimConfig a preencher, filename = caminho do arquivo de configuração
int config_load(SimConfig *cfg, const char *filename);

//config_print - Imprime no terminal a configuração carregada
void config_print(const SimConfig *cfg);

//config_create_default cria um arquivo de configuração de exemplo
// Parâmetro: filename nome do arquivo a criar
void config_create_default(const char *filename);

// algo_name_to_enum converte string do nome do algoritmo para enum
SchedAlgo algo_name_to_enum(const char *name);

//algo_enum_to_name - Retorna o nome legível do algoritmo
const char *algo_enum_to_name(SchedAlgo algo);

#endif //CONFIG_H 
