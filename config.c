/*
 * config.c - Implementação da leitura e parse do arquivo de configuração
 *
 * O parse segue o formato especificado no documento:
 *   Linha 1: algoritmo;quantum;qtde_cpus
 *   Linha N: id;cor;ingresso;duracao;prioridade[;eventos]
 *
 * ALTERAÇÕES v2 (suporte a CSV e caso-teste-mc):
 *   - parse_task_line() extraída para função auxiliar reutilizável
 *   - parse_id_field()  aceita IDs numéricos ("5") OU com prefixo ("t05")
 *   - config_load_csv() novo: lê arquivo .csv com separador vírgula (',')
 *   - config_load()     agora tolera ';' extra no final das linhas de tarefa
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>    /* tolower(), isdigit() - para tratar maiúsculas e dígitos */
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

/*
 * [NOVO] parse_id_field - Converte campo de ID para inteiro
 *
 * Por quê foi adicionado:
 *   O caso-teste-mc-005-priop.txt usa IDs no formato "t01", "t02", etc.
 *   O parser original só aceitava inteiros puros como "1", "2".
 *   Esta função detecta automaticamente o formato e converte ambos.
 *
 * Estratégia:
 *   - Se o campo começa com letra (ex: "t01"), pula os caracteres não-dígitos
 *     e converte o número restante com atoi()
 *   - Se começa com dígito (ex: "5"), converte diretamente com atoi()
 *
 * Exemplos:
 *   "t01"  → 1
 *   "t15"  → 15
 *   "5"    → 5
 *   "id_3" → 3  (pula "id_")
 */
static int parse_id_field(const char *s) {
    /* Avança enquanto o caractere não for dígito */
    while (*s && !isdigit((unsigned char)*s)) {
        s++;
    }
    /* atoi converte a partir do primeiro dígito encontrado */
    return atoi(s);
}

/*
 * [NOVO] parse_task_line - Faz parse de uma linha de tarefa com separador genérico
 *
 * Por quê foi extraída para função separada:
 *   Tanto config_load() (separador ';') quanto config_load_csv() (separador ',')
 *   precisam fazer exatamente o mesmo parse de tarefa — só o separador muda.
 *   Extrair evita duplicação de código (princípio DRY: Don't Repeat Yourself).
 *
 * Formato esperado:
 *   id<sep>cor<sep>ingresso<sep>duracao<sep>prioridade[<sep>]
 *
 *   O ';' ou ',' extra no final (como em "t01;0000FF;5;9;1;") é tolerado:
 *   strtok simplesmente não encontra mais tokens e paramos antes disso.
 *
 * Parâmetros:
 *   buf      - Buffer mutável com a linha (será modificado por strtok)
 *   sep      - String do separador, ex: ";" ou ","
 *   cfg      - Estrutura de configuração a preencher
 *   line_num - Número da linha no arquivo (para mensagens de erro)
 *
 * Retorno:
 *   0  = sucesso, tarefa adicionada em cfg->tasks[cfg->num_tasks-1]
 *  -1  = erro de formato
 */
static int parse_task_line(char *buf, const char *sep,
                            SimConfig *cfg, int line_num) {
    int  id, arrival, duration, priority;
    char color[COLOR_LEN];
    char *tok;

    /* Campo 1: ID — aceita "t01" ou "5" graças a parse_id_field() */
    tok = strtok(buf, sep);
    if (!tok) { goto err; }
    id = parse_id_field(trim(tok));   /* [ALTERADO] era: id = atoi(trim(tok)) */

    /* Campo 2: cor em RGB hex, ex: "FF4444" */
    tok = strtok(NULL, sep);
    if (!tok) { goto err; }
    strncpy(color, trim(tok), COLOR_LEN - 1);
    color[COLOR_LEN - 1] = '\0';     /* garante terminação nula */

    /* Campo 3: instante de chegada (ingresso) */
    tok = strtok(NULL, sep);
    if (!tok) { goto err; }
    arrival = atoi(trim(tok));

    /* Campo 4: duração total necessária de CPU */
    tok = strtok(NULL, sep);
    if (!tok) { goto err; }
    duration = atoi(trim(tok));
    if (duration <= 0) {
        fprintf(stderr, "[ERRO] Tarefa %d: duração inválida (%d)\n", id, duration);
        return -1;
    }

    /* Campo 5: prioridade estática */
    tok = strtok(NULL, sep);
    if (!tok) { goto err; }
    priority = atoi(trim(tok));

    /* Inicializa o TCB desta tarefa no próximo slot livre do array */
    task_init(&cfg->tasks[cfg->num_tasks], id, color, arrival, duration, priority);
    cfg->num_tasks++;   /* incrementa o contador de tarefas carregadas */

    /* Campo 6 em diante (eventos, ';' extra, etc.) são ignorados:
     * o caso-teste-mc termina cada linha com ";" extra, o que é normal
     * — strtok simplesmente não encontra mais tokens e encerramos. */
    return 0; /* Sucesso */

err:
    fprintf(stderr, "[ERRO] Formato inválido na linha %d\n", line_num);
    return -1;
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
 * [NOVO] parse_header_line - Faz parse da linha 1 (parâmetros gerais)
 *                            com separador genérico
 *
 * Por quê foi extraída:
 *   Igual à parse_task_line — tanto config_load() quanto config_load_csv()
 *   precisam parsear o cabeçalho. Separar evita duplicação.
 *
 * Formato: algoritmo<sep>quantum<sep>qtde_cpus
 *
 * Retorno: 0 = sucesso, -1 = erro
 */
static int parse_header_line(char *buf, const char *sep,
                              SimConfig *cfg, FILE *f) {
    /* Token 1: nome do algoritmo (ex: "SRTF", "PRIOP") */
    char *tok = strtok(buf, sep);
    if (!tok) { return -1; }
    strncpy(cfg->algo_name, trim(tok), MAX_ALGO_NAME - 1);
    cfg->algo = algo_name_to_enum(cfg->algo_name);
    if (cfg->algo == ALGO_UNKNOWN) {
        fprintf(stderr, "[ERRO] Algoritmo desconhecido: '%s'\n", cfg->algo_name);
        fprintf(stderr, "       Algoritmos válidos: SRTF, PRIOP\n");
        if (f) fclose(f);
        return -1;
    }

    /* Token 2: quantum (fatia máxima de CPU por tarefa) */
    tok = strtok(NULL, sep);
    if (!tok) { return -1; }
    cfg->quantum = atoi(trim(tok));
    if (cfg->quantum <= 0) {
        fprintf(stderr, "[ERRO] Quantum inválido: %d (deve ser > 0)\n", cfg->quantum);
        if (f) fclose(f);
        return -1;
    }

    /* Token 3: número de CPUs disponíveis */
    tok = strtok(NULL, sep);
    if (!tok) { return -1; }
    cfg->num_cpus = atoi(trim(tok));
    if (cfg->num_cpus < 2 || cfg->num_cpus > MAX_CPUS) {
        fprintf(stderr, "[ERRO] Número de CPUs inválido: %d (mínimo 2, máximo %d)\n",
                cfg->num_cpus, MAX_CPUS);
        if (f) fclose(f);
        return -1;
    }

    return 0; /* Sucesso */
}

/*
 * config_load_with_sep - Núcleo compartilhado de leitura de arquivo
 *
 * [NOVO] Por quê existe esta função interna:
 *   config_load() (separador ";") e config_load_csv() (separador ",")
 *   têm exatamente a mesma lógica de loop de arquivo — só o separador
 *   difere. Esta função centraliza o loop e recebe o separador como
 *   parâmetro, evitando duplicar ~80 linhas de código.
 *
 * Estratégia de parse (mesma de antes, agora parametrizada):
 *   1. Abre o arquivo
 *   2. Lê linha 1 → parse_header_line()
 *   3. Para cada linha restante → parse_task_line()
 */
static int config_load_with_sep(SimConfig *cfg, const char *filename,
                                 const char *sep) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível abrir o arquivo: %s\n", filename);
        return -1;
    }

    char line[MAX_LINE_LEN];
    int  line_num = 0;

    /* Inicializa a estrutura com zeros antes de começar o parse */
    memset(cfg, 0, sizeof(SimConfig));
    cfg->num_tasks = 0;

    while (fgets(line, MAX_LINE_LEN, f)) {
        /* Ignora linhas em branco e comentários (linhas que começam com '#') */
        char *trimmed = trim(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue; /* pula para a próxima linha sem incrementar line_num */
        }

        line_num++;

        if (line_num == 1) {
            /* ── Primeira linha válida: parâmetros gerais ─────────────── */
            char buf[MAX_LINE_LEN];
            strncpy(buf, trimmed, MAX_LINE_LEN - 1);
            if (parse_header_line(buf, sep, cfg, f) != 0) {
                /* parse_header_line já fechou o arquivo e imprimiu erro */
                return -1;
            }

        } else {
            /* ── Demais linhas válidas: uma tarefa cada ───────────────── */
            if (cfg->num_tasks >= MAX_TASKS) {
                fprintf(stderr, "[AVISO] Limite de %d tarefas atingido, "
                        "linha %d ignorada\n", MAX_TASKS, line_num);
                continue;
            }

            char buf[MAX_LINE_LEN];
            strncpy(buf, trimmed, MAX_LINE_LEN - 1);

            /* parse_task_line faz o parse e adiciona a tarefa em cfg */
            if (parse_task_line(buf, sep, cfg, line_num) != 0) {
                fclose(f);
                return -1;
            }
        }
    }

    fclose(f);

    /* Verifica se pelo menos uma tarefa foi carregada */
    if (cfg->num_tasks == 0) {
        fprintf(stderr, "[ERRO] Nenhuma tarefa encontrada no arquivo: %s\n",
                filename);
        return -1;
    }

    return 0; /* Sucesso */
}

/*
 * config_load - Carrega configuração do arquivo com separador ';' (ponto-e-vírgula)
 *
 * Comportamento idêntico ao original. Internamente chama config_load_with_sep()
 * passando ";" como separador.
 *
 * [ALTERAÇÃO] Antes: toda a lógica de parse estava aqui.
 *             Agora: delega para config_load_with_sep(cfg, filename, ";").
 *             O comportamento externo é 100% igual — só a organização interna mudou.
 *
 * Compatível com:
 *   - Formato original do projeto: "SRTF;4;2" / "1;FF4444;0;10;3"
 *   - caso-teste-mc-005-priop.txt: "PRIOP;3;10" / "t01;0000FF;5;9;1;"
 *     (IDs com prefixo "t" e ';' extra no final são tratados corretamente)
 */
int config_load(SimConfig *cfg, const char *filename) {
    return config_load_with_sep(cfg, filename, ";");
}

/*
 * [NOVA FUNÇÃO] config_load_csv - Carrega configuração de arquivo .csv (vírgula)
 *
 * Por quê foi adicionada:
 *   CSV (Comma-Separated Values) é o formato mais comum de planilhas exportadas
 *   pelo Excel, LibreOffice Calc, Google Sheets, etc. Adicionar suporte a CSV
 *   permite ao usuário preparar o caso de teste diretamente numa planilha.
 *
 * Formato CSV esperado (mesmo conteúdo do .txt, mas com ',' em vez de ';'):
 *   Linha 1: algoritmo,quantum,qtde_cpus
 *   Linha N: id,cor,ingresso,duracao,prioridade
 *
 * Exemplo de arquivo .csv válido:
 *   SRTF,4,2
 *   1,FF4444,0,10,3
 *   2,44FF44,2,6,5
 *   t03,FF0000,1,4,4
 *
 * IDs com prefixo (ex: "t03") também são aceitos, assim como no .txt.
 *
 * Parâmetros:
 *   cfg      - Ponteiro para SimConfig a preencher
 *   filename - Caminho do arquivo .csv
 *
 * Retorno:
 *   0 = sucesso, -1 = erro
 */
int config_load_csv(SimConfig *cfg, const char *filename) {
    return config_load_with_sep(cfg, filename, ",");
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

/*
 * [NOVA FUNÇÃO] config_create_default_csv - Cria arquivo CSV de exemplo
 *
 * Gera o mesmo conteúdo de config_create_default(), mas com vírgulas
 * como separador. Útil para o usuário ter um modelo de CSV para editar
 * no Excel ou LibreOffice.
 */
void config_create_default_csv(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível criar arquivo: %s\n", filename);
        return;
    }

    /* Cabeçalho em formato CSV com vírgula */
    fprintf(f, "# Arquivo CSV de configuração do Simulador SO Multitarefa\n");
    fprintf(f, "# Formato linha 1: algoritmo,quantum,qtde_cpus\n");
    fprintf(f, "# Formato demais:  id,cor,ingresso,duracao,prioridade\n");
    fprintf(f, "# Algoritmos: SRTF, PRIOP\n");
    fprintf(f, "#\n");
    fprintf(f, "SRTF,4,2\n");          /* linha 1: parâmetros gerais com vírgula */
    fprintf(f, "1,FF4444,0,10,3\n");   /* tarefas com vírgula */
    fprintf(f, "2,44FF44,2,6,5\n");
    fprintf(f, "3,4444FF,4,8,1\n");
    fprintf(f, "4,FFAA00,6,4,4\n");

    fclose(f);
    printf("[INFO] Arquivo CSV de exemplo criado: %s\n", filename);
}