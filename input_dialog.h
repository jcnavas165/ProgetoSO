/*
 * input_dialog.h - Diálogos GTK para importar arquivo e entrada manual
 *
 * Este módulo adiciona três formas de carregar a configuração:
 *
 *   1. IMPORTAR ARQUIVO TXT (separador ';'):
 *      O usuário escolhe um arquivo .txt pelo explorador de arquivos.
 *      Compatível com o formato original do projeto e com
 *      caso-teste-mc-005-priop.txt (IDs "t01", "t02", ';' extra no final).
 *
 *   2. IMPORTAR ARQUIVO CSV (separador ','):     ← [NOVO]
 *      O usuário escolhe um arquivo .csv exportado de planilha.
 *      Mesmo formato do .txt mas com vírgula como separador.
 *
 *   3. IMPORTAR ARQUIVO PDF (texto extraído):
 *      Usa pdftotext para converter o PDF para texto e faz parse.
 *
 *   4. ENTRADA MANUAL:
 *      Formulário GTK onde o usuário digita os dados sem arquivo.
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#ifndef INPUT_DIALOG_H
#define INPUT_DIALOG_H

#include <gtk/gtk.h>
#include "config.h"

/*
 * input_show_file_chooser - Abre o explorador de arquivos GTK
 *
 * Exibe um diálogo "Abrir arquivo" filtrado para .txt, .csv e .pdf.
 * Quando o usuário confirma, lê o arquivo e preenche cfg.
 * [ALTERADO] Agora também aceita .csv no filtro de arquivos.
 *
 * Retorno:
 *   0  = sucesso (cfg preenchido)
 *  -1  = usuário cancelou ou arquivo inválido
 */
int input_show_file_chooser(GtkWindow *parent, SimConfig *cfg);

/*
 * input_show_manual_dialog - Abre o formulário de entrada manual
 *
 * Retorno:
 *   0  = usuário confirmou (cfg preenchido)
 *  -1  = usuário cancelou
 */
int input_show_manual_dialog(GtkWindow *parent, SimConfig *cfg);

/*
 * input_read_txt - Lê um arquivo .txt (separador ';') e faz parse
 *
 * Reutiliza config_load() (que internamente usa config_load_with_sep).
 * Aceita o caso-teste-mc com IDs "t01" e ';' extra no final.
 * Retorna 0 em sucesso, -1 em erro.
 */
int input_read_txt(const char *filepath, SimConfig *cfg);

/*
 * [NOVA] input_read_csv - Lê um arquivo .csv (separador ',') e faz parse
 *
 * Chama config_load_csv() do módulo config.c.
 * Aceita arquivos exportados de Excel / LibreOffice / Google Sheets.
 * IDs com prefixo (ex: "t03") também são aceitos.
 *
 * Formato CSV esperado:
 *   algoritmo,quantum,qtde_cpus        ← linha 1
 *   id,cor,ingresso,duracao,prioridade ← demais linhas
 *
 * Retorna 0 em sucesso, -1 em erro.
 */
int input_read_csv(const char *filepath, SimConfig *cfg);

/*
 * input_read_pdf - Extrai texto de um PDF e faz parse da configuração
 *
 * Usa pdftotext (poppler-utils) para converter PDF para texto,
 * depois chama input_read_txt() com o resultado.
 *
 * Dependência: sudo apt install poppler-utils
 * Retorna 0 em sucesso, -1 em erro.
 */
int input_read_pdf(const char *filepath, SimConfig *cfg);

#endif /* INPUT_DIALOG_H */