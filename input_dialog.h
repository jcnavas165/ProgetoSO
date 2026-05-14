/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 
 * input_dialog.h - Diálogos GTK para importar arquivo e entrada manual
 *
 * Este módulo adiciona duas formas de carregar a configuração:
 *
 *   1. IMPORTAR ARQUIVO (TXT ou PDF lido como texto):
 *      O usuário escolhe um arquivo .txt ou .pdf pelo explorador de arquivos.
 *      O módulo lê o conteúdo e repassa para o parser já existente (config.c).
 *
 *   2. ENTRADA MANUAL:
 *      Uma janela com campos de formulário permite digitar o algoritmo,
 *      quantum, CPUs e cada tarefa sem precisar criar um arquivo.
 *
 */

#ifndef INPUT_DIALOG_H
#define INPUT_DIALOG_H

#include <gtk/gtk.h>
#include "config.h"

/*
 * input_show_file_chooser - Abre o explorador de arquivos GTK
 *
 * Exibe um diálogo "Abrir arquivo" filtrado para .txt e .pdf.
 * Quando o usuário confirma, lê o arquivo e preenche cfg.
 *
 * Parâmetros:
 *   parent  - Janela pai (para centralizar o diálogo sobre ela)
 *   cfg     - Ponteiro para SimConfig a preencher
 *
 * Retorno:
 *   0  = sucesso (cfg preenchido)
 *  -1  = usuário cancelou ou arquivo inválido
 */
int input_show_file_chooser(GtkWindow *parent, SimConfig *cfg);

/*
 * input_show_manual_dialog - Abre o formulário de entrada manual
 *
 * Exibe uma janela com:
 *   - Combo box para escolher o algoritmo (SRTF / PRIOP)
 *   - Spin button para quantum
 *   - Spin button para número de CPUs
 *   - Botão "Adicionar tarefa" que abre uma linha de campos
 *   - Botão "OK" que valida e preenche cfg
 *
 * Parâmetros:
 *   parent  - Janela pai
 *   cfg     - Ponteiro para SimConfig a preencher
 *
 * Retorno:
 *   0  = usuário confirmou (cfg preenchido)
 *  -1  = usuário cancelou
 */
int input_show_manual_dialog(GtkWindow *parent, SimConfig *cfg);

/*
 * input_read_txt - Lê um arquivo .txt e faz parse da configuração
 *
 * Reutiliza o parser de config.c (config_load_from_string).
 * Retorna 0 em sucesso, -1 em erro.
 */
int input_read_txt(const char *filepath, SimConfig *cfg);

/*
 * input_read_pdf - Extrai texto de um PDF e faz parse da configuração
 *
 * Usa pdftotext (do pacote poppler-utils) para converter o PDF
 * para texto simples, depois reutiliza o mesmo parser do .txt.
 *
 * Dependência: pdftotext deve estar instalado.
 *   sudo apt install poppler-utils
 *
 * Retorna 0 em sucesso, -1 em erro.
 */
int input_read_pdf(const char *filepath, SimConfig *cfg);

#endif /* INPUT_DIALOG_H */
