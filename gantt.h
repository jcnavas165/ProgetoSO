/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 */

#ifndef GANTT_H
#define GANTT_H

#include "simulator.h"

/* Dimensões do gráfico SVG */
#define GANTT_CELL_W   28    /* Largura de cada célula (pixels por tick) */
#define GANTT_CELL_H   36    /* Altura de cada linha de tarefa */
#define GANTT_MARGIN_L 80    /* Margem esquerda (para labels das tarefas) */
#define GANTT_MARGIN_T 50    /* Margem superior (para título) */
#define GANTT_MARGIN_B 80    /* Margem inferior (para eixo de tempo + legenda) */
#define GANTT_MARGIN_R 20    /* Margem direita */

/* ── Funções do Gantt ─────────────────────────────────────────────────────── */

/*
 * gantt_generate_svg - Gera o arquivo SVG com o gráfico de Gantt
 *
 * Percorre o histórico de cada tarefa e desenha um gráfico completo.
 * O arquivo SVG pode ser aberto diretamente em qualquer navegador web.
 *
 * Parâmetros:
 *   sim      - Estado final da simulação (com histórico completo)
 *   filename - Nome do arquivo SVG a gerar (ex: "gantt.svg")
 *
 * Retorno:
 *   0 em caso de sucesso, -1 em erro
 */
int gantt_generate_svg(const SimState *sim, const char *filename);

/*
 * gantt_print_terminal - Imprime versão textual do Gantt no terminal
 *
 * Versão simplificada em texto ASCII/Unicode do gráfico, útil para
 * visualizar rapidamente sem precisar abrir um arquivo.
 *
 * Usa um caractere por tick para indicar o estado:
 *   '#' = executando
 *   '.' = pronta (aguardando CPU)
 *   'X' = suspensa
 *   ' ' = ainda não chegou ou já terminou
 */
void gantt_print_terminal(const SimState *sim);

#endif /* GANTT_H */
