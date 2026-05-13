/*
 * config_ext.h - Extensão do módulo config para carregar de string
 *
 * O parser original (config_load) só aceita um caminho de arquivo.
 * Aqui adicionamos config_load_from_string que aceita o conteúdo
 * já em memória — necessário para PDF (onde extraímos o texto
 * via pdftotext antes de fazer o parse).
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#ifndef CONFIG_EXT_H
#define CONFIG_EXT_H

#include "config.h"

/*
 * config_load_from_string - Faz parse de uma string no formato config
 *
 * Mesmo formato do arquivo .txt:
 *   Linha 1: algoritmo;quantum;qtde_cpus
 *   Linhas 2+: id;cor;ingresso;duracao;prioridade
 *
 * Parâmetros:
 *   cfg     - Ponteiro para SimConfig a preencher
 *   content - String com o conteúdo completo (pode ter \n)
 *
 * Retorno:
 *   0  = sucesso
 *  -1  = erro de parse
 */
int config_load_from_string(SimConfig *cfg, const char *content);

#endif /* CONFIG_EXT_H */
