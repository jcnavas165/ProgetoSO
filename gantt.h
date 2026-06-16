/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 *
 * gantt.h - Interface do gerador do gráfico de Gantt
 *
 * O gráfico de Gantt mostra visualmente a execução das tarefas ao longo
 * do tempo, conforme especificado no requisito 2.1 e 2.5.
 *
 * Formato do gráfico (conforme seção 6.4 do livro do Prof. Maziero):
 *   - Eixo X: tempo (ticks do relógio)
 *   - Eixo Y: tarefas (ID menor na parte inferior, crescente para cima)
 *   - Blocos coloridos: tarefa executando (cor definida no arquivo de config)
 *   - Blocos vazios/brancos: tarefa na fila de prontos
 *   - Blocos pretos: tarefa suspensa
 *   - Ícones especiais: chegada (▶), término (■), sorteio (★)
 *
 * Implementamos o Gantt em SVG porque:
 *   - SVG é um arquivo texto (legível por humanos)
 *   - Funciona em qualquer browser sem instalar nada
 *   - Permite elementos gráficos ricos (cores, formas, texto)
 *   - Escalável sem perda de qualidade
 *
 * Autor: Projeto A - Simulador SO Multitarefa
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
