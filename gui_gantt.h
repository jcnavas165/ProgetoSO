/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 *
 * gui_gantt.h - Interface da janela GTK principal do simulador
 */
#ifndef GUI_GANTT_H
#define GUI_GANTT_H

#include "simulator.h"

/*
 * gui_run - Abre a janela GTK principal
 *
 * Parâmetros:
 *   config_file - Caminho do arquivo de config (pode ser NULL)
 *                 Se NULL, exibe a tela de boas-vindas para importar/manual
 */
void gui_run(const char *config_file);

#endif