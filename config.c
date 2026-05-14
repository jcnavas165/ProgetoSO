

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>    /* tolower(), para tratar maiúsculas/minúsculas */
#include "config.h"

/* ── Funções auxiliares internas (static = visíveis só neste arquivo) ─────── */

/*
 * str_to_lower - Converte uma string para letras minúsculas (in-place)
 *
 * Necessário para tratar o requisito 3.3.2: nomes de algoritmos
 * são case-insensitive. Modificamos uma cópia, não o original.
 *
 * Decisão de implementação:
 *   Usamos toupper da biblioteca ctype.h para portabilidade.
 *   A função modifica a string no local (in-place).
 */
static void str_to_lower(char *s) {
    for (; *s; s++) {
        *s = (char)tolower((unsigned char)*s);
    }
}

/*
 * trim - Remove espaços e caracteres de controle das bordas da string
 *
 * Necessário porque fgets() preserva o '\n' final da linha lida,
 * e o usuário pode colocar espaços antes/depois dos valores no arquivo.
 *
 * Modifica a string in-place retornando o ponteiro para o início
 * do conteúdo sem espaços iniciais.
 */
static char *trim(char *s) {
    /* Remove espaços do início */
    while (*s && isspace((unsigned char)*s)) s++;

    /* Remove espaços/newline do final */
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

/* ── Implementação das funções públicas ───────────────────────────────────── */

/*
 * algo_name_to_enum - Converte string para enum do algoritmo
 *
 * Decisão de implementação:
 *   Convertemos para minúsculas antes de comparar, implementando
 *   o requisito 3.3.2 (case-insensitive). Fazemos a conversão em
 *   uma cópia local para não modificar o parâmetro original.
 */
SchedAlgo algo_name_to_enum(const char *name) {
    char lower[MAX_ALGO_NAME];
    strncpy(lower, name, MAX_ALGO_NAME - 1);
    lower[MAX_ALGO_NAME - 1] = '\0';
    str_to_lower(lower);

    if (strcmp(lower, "srtf")  == 0) return ALGO_SRTF;
    if (strcmp(lower, "priop") == 0) return ALGO_PRIOP;

    return ALGO_UNKNOWN;
}

/*
 * algo_enum_to_name - Retorna nome legível do algoritmo
 */
const char *algo_enum_to_name(SchedAlgo algo) {
    switch (algo) {
        case ALGO_SRTF:  return "SRTF (Shortest Remaining Time First)";
        case ALGO_PRIOP: return "PRIOP (Prioridade Preemptivo)";
        default:         return "DESCONHECIDO";
    }
}

/*
 * config_load - Carrega configuração do arquivo
 *
 * Estratégia de parse:
 *   1. Abrimos o arquivo
 *   2. Lemos linha 1 → parâmetros gerais (sscanf com separador ';')
 *   3. Para cada linha restante → parse de uma tarefa
 *   4. Inicializamos cada TCB com task_init()
 *
 * Tratamento de erros:
 *   Retornamos -1 imediatamente em qualquer erro crítico,
 *   com mensagem explicativa no stderr.
 */
int config_load(SimConfig *cfg, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível abrir o arquivo: %s\n", filename);
        return -1;
    }

    char line[MAX_LINE_LEN];
    int  line_num = 0;

    /* ── Inicializa a estrutura com zeros ────────────────────────────────── */
    memset(cfg, 0, sizeof(SimConfig));
    cfg->num_tasks = 0;

    while (fgets(line, MAX_LINE_LEN, f)) {
        /* Ignora linhas em branco e comentários ANTES de incrementar linha */
        char *trimmed = trim(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue;
        }

        line_num++;

        if (line_num == 1) {
            /* ── Parse da linha 1: parâmetros gerais ─────────────────────── */
            /*
             * Formato: algoritmo;quantum;qtde_cpus
             * Usamos strtok para dividir pelo separador ';'
             *
             * Decisão: strtok modifica a string original (coloca '\0' onde
             * encontra o separador). Por isso trabalhamos com uma cópia.
             */
            char buf[MAX_LINE_LEN];
            strncpy(buf, trimmed, MAX_LINE_LEN - 1);

            char *tok = strtok(buf, ";");
            if (!tok) { goto parse_error; }
            strncpy(cfg->algo_name, trim(tok), MAX_ALGO_NAME - 1);
            cfg->algo = algo_name_to_enum(cfg->algo_name);
            if (cfg->algo == ALGO_UNKNOWN) {
                fprintf(stderr, "[ERRO] Algoritmo desconhecido: '%s'\n", cfg->algo_name);
                fprintf(stderr, "       Algoritmos válidos: SRTF, PRIOP\n");
                fclose(f);
                return -1;
            }

            tok = strtok(NULL, ";");
            if (!tok) { goto parse_error; }
            cfg->quantum = atoi(trim(tok));
            if (cfg->quantum <= 0) {
                fprintf(stderr, "[ERRO] Quantum inválido: %d (deve ser > 0)\n", cfg->quantum);
                fclose(f);
                return -1;
            }

            tok = strtok(NULL, ";");
            if (!tok) { goto parse_error; }
            cfg->num_cpus = atoi(trim(tok));
            if (cfg->num_cpus < 2 || cfg->num_cpus > MAX_CPUS) {
                fprintf(stderr, "[ERRO] Número de CPUs inválido: %d (mínimo 2, máximo %d)\n",
                        cfg->num_cpus, MAX_CPUS);
                fclose(f);
                return -1;
            }

        } else {
            /* ── Parse das linhas 2+: parâmetros de cada tarefa ──────────── */
            /*
             * Formato: id;cor;ingresso;duracao;prioridade[;eventos]
             * Campos de eventos são opcionais neste projeto (Projeto B).
             */
            if (cfg->num_tasks >= MAX_TASKS) {
                fprintf(stderr, "[AVISO] Limite de %d tarefas atingido, linha %d ignorada\n",
                        MAX_TASKS, line_num);
                continue;
            }

            char buf[MAX_LINE_LEN];
            strncpy(buf, trimmed, MAX_LINE_LEN - 1);

            int  id, arrival, duration, priority;
            char color[COLOR_LEN];

            /* Parse campo a campo com strtok */
            char *tok;

            tok = strtok(buf, ";");
            if (!tok) { goto parse_error_task; }
            id = atoi(trim(tok));

            tok = strtok(NULL, ";");
            if (!tok) { goto parse_error_task; }
            strncpy(color, trim(tok), COLOR_LEN - 1);
            color[COLOR_LEN - 1] = '\0';

            tok = strtok(NULL, ";");
            if (!tok) { goto parse_error_task; }
            arrival = atoi(trim(tok));

            tok = strtok(NULL, ";");
            if (!tok) { goto parse_error_task; }
            duration = atoi(trim(tok));
            if (duration <= 0) {
                fprintf(stderr, "[ERRO] Tarefa %d: duração inválida (%d)\n", id, duration);
                fclose(f);
                return -1;
            }

            tok = strtok(NULL, ";");
            if (!tok) { goto parse_error_task; }
            priority = atoi(trim(tok));

            /* Inicializa o TCB desta tarefa */
            task_init(&cfg->tasks[cfg->num_tasks], id, color, arrival, duration, priority);
            cfg->num_tasks++;

            /* Nota: campo 'lista_eventos' é ignorado neste projeto (Projeto B) */
            continue;

parse_error_task:
            fprintf(stderr, "[ERRO] Formato inválido na linha %d: '%s'\n", line_num, trimmed);
            fclose(f);
            return -1;
        }
    }

    fclose(f);

    /* Verifica se carregou ao menos uma tarefa */
    if (cfg->num_tasks == 0) {
        fprintf(stderr, "[ERRO] Nenhuma tarefa encontrada no arquivo de configuração\n");
        return -1;
    }

    return 0; /* Sucesso */

parse_error:
    fprintf(stderr, "[ERRO] Formato inválido na linha 1: '%s'\n", line);
    fprintf(stderr, "       Formato esperado: algoritmo;quantum;qtde_cpus\n");
    fclose(f);
    return -1;
}

/*
 * config_print - Exibe a configuração carregada no terminal
 */
void config_print(const SimConfig *cfg) {
    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║         CONFIGURAÇÃO DA SIMULAÇÃO            ║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║ Algoritmo : %-32s║\n", algo_enum_to_name(cfg->algo));
    printf("║ Quantum   : %-32d║\n", cfg->quantum);
    printf("║ CPUs      : %-32d║\n", cfg->num_cpus);
    printf("║ Tarefas   : %-32d║\n", cfg->num_tasks);
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║  ID │ Cor    │ Chegada │ Duração │ Prioridade║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    for (int i = 0; i < cfg->num_tasks; i++) {
        const TCB *t = &cfg->tasks[i];
        printf("║  %-3d│ #%-6s│ %-7d │ %-7d │ %-10d║\n",
               t->id, t->color, t->arrival, t->duration, t->priority);
    }
    printf("╚══════════════════════════════════════════════╝\n\n");
}

/*
 * config_create_default - Cria arquivo de configuração de exemplo
 *
 * Gera um arquivo com 3 tarefas de exemplo para o usuário
 * ter um ponto de partida.
 */
void config_create_default(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível criar arquivo: %s\n", filename);
        return;
    }

    fprintf(f, "# Arquivo de configuração do Simulador SO Multitarefa\n");
    fprintf(f, "# Formato linha 1: algoritmo;quantum;qtde_cpus\n");
    fprintf(f, "# Formato demais:  id;cor;ingresso;duracao;prioridade\n");
    fprintf(f, "# Algoritmos: SRTF, PRIOP\n");
    fprintf(f, "#\n");
    fprintf(f, "SRTF;4;2\n");
    fprintf(f, "1;FF4444;0;10;3\n");
    fprintf(f, "2;44FF44;2;6;5\n");
    fprintf(f, "3;4444FF;4;8;1\n");
    fprintf(f, "4;FFAA00;6;4;4\n");

    fclose(f);
    printf("[INFO] Arquivo de configuração de exemplo criado: %s\n", filename);
}
