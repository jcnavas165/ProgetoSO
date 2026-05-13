/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 *
 * input_dialog.c - Implementação dos diálogos de importação e entrada manual
 *
 * CONCEITOS GTK USADOS AQUI:
 *
 *   GtkFileChooserDialog - janela de escolha de arquivo do sistema
 *   GtkDialog            - janela de diálogo com botões OK/Cancelar
 *   GtkGrid              - organiza widgets em grade (linhas e colunas)
 *   GtkEntry             - campo de texto para o usuário digitar
 *   GtkSpinButton        - campo numérico com setas +/-
 *   GtkComboBoxText      - lista suspensa de opções
 *   GtkScrolledWindow    - área rolável (para quando há muitas tarefas)
 *   GtkMessageDialog     - janela de mensagem de erro/aviso
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp() - comparação de strings sem case */
#include "input_dialog.h"
#include "config_ext.h"
#include "config.h"
#include "task.h"

/* ══════════════════════════════════════════════════════════════════════════
 * PARTE 1 — LEITURA DE ARQUIVO TXT
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * input_read_txt - Lê arquivo .txt e converte para SimConfig
 *
 * Estratégia:
 *   1. Abre o arquivo
 *   2. Lê o conteúdo inteiro para uma string na memória (com malloc)
 *   3. Chama config_load_from_string para fazer o parse
 *   4. Libera a memória
 *
 * Por que ler tudo de uma vez?
 *   config_load_from_string precisa da string completa. Poderíamos
 *   ler linha por linha, mas ler tudo de uma vez é mais simples e
 *   o arquivo de config é sempre pequeno (< 10 KB).
 */
int input_read_txt(const char *filepath, SimConfig *cfg) {
    /* Abre o arquivo em modo leitura de texto */
    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível abrir: %s\n", filepath);
        return -1;
    }

    /* Descobre o tamanho do arquivo:
     * fseek(f, 0, SEEK_END) move o cursor para o final do arquivo
     * ftell(f) retorna a posição atual = tamanho em bytes */
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, 0, SEEK_SET);  /* volta para o início do arquivo */

    if (tamanho <= 0) {
        fprintf(stderr, "[ERRO] Arquivo vazio: %s\n", filepath);
        fclose(f);
        return -1;
    }

    /* Aloca memória para guardar todo o conteúdo + '\0' final */
    char *conteudo = (char *)malloc(tamanho + 1);
    if (!conteudo) {
        fprintf(stderr, "[ERRO] Memória insuficiente\n");
        fclose(f);
        return -1;
    }

    /* Lê todo o arquivo de uma vez:
     * fread(buffer, tamanho_de_cada_item, quantidade, arquivo)
     * Retorna quantos itens foram lidos com sucesso */
    size_t lidos = fread(conteudo, 1, tamanho, f);
    conteudo[lidos] = '\0';  /* garante terminação nula */
    fclose(f);

    /* Faz o parse usando o parser central */
    int resultado = config_load_from_string(cfg, conteudo);

    /* Libera a memória alocada com malloc */
    free(conteudo);

    return resultado;
}

/* ══════════════════════════════════════════════════════════════════════════
 * PARTE 2 — LEITURA DE ARQUIVO PDF
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * input_read_pdf - Extrai texto do PDF com pdftotext e faz parse
 *
 * Como funciona:
 *   1. Usa pdftotext (ferramenta externa do pacote poppler-utils)
 *      para converter o PDF em texto simples
 *   2. Lê o texto gerado como se fosse um .txt normal
 *   3. Apaga o arquivo de texto temporário gerado
 *
 * pdftotext é chamado via system() que executa um comando no shell.
 * O comando é: pdftotext arquivo.pdf arquivo_temp.txt
 *
 * IMPORTANTE: O PDF deve conter o texto no formato:
 *   algoritmo;quantum;cpus
 *   id;cor;chegada;duracao;prioridade
 *   ... (mesmo formato do .txt)
 */
int input_read_pdf(const char *filepath, SimConfig *cfg) {
    /* Nome do arquivo temporário onde o texto extraído será salvo */
    char txt_temp[512];
    snprintf(txt_temp, sizeof(txt_temp), "/tmp/simulador_pdf_temp.txt");

    /* Monta o comando shell para executar o pdftotext:
     * "-layout" tenta preservar o layout do PDF
     * "-nopgbrk" remove marcadores de quebra de página */
    char comando[1024];
    snprintf(comando, sizeof(comando),
             "pdftotext -layout -nopgbrk \"%s\" \"%s\" 2>/dev/null",
             filepath, txt_temp);

    /* system() executa o comando no shell e retorna o código de saída
     * 0 = sucesso, qualquer outro valor = erro */
    int ret = system(comando);
    if (ret != 0) {
        fprintf(stderr, "[ERRO] Falha ao extrair texto do PDF.\n");
        fprintf(stderr, "       Instale com: sudo apt install poppler-utils\n");
        return -1;
    }

    /* Lê o arquivo de texto gerado (igual ao caso .txt) */
    int resultado = input_read_txt(txt_temp, cfg);

    /* Remove o arquivo temporário */
    remove(txt_temp);

    return resultado;
}

/* ══════════════════════════════════════════════════════════════════════════
 * PARTE 3 — DIÁLOGO DE ESCOLHA DE ARQUIVO (TXT ou PDF)
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * mostrar_erro - Exibe uma janela de erro para o usuário
 *
 * GtkMessageDialog é a forma padrão do GTK de mostrar mensagens.
 * gtk_dialog_run() trava a execução até o usuário fechar a janela.
 */
static void mostrar_erro(GtkWindow *parent, const char *mensagem) {
    GtkWidget *dlg = gtk_message_dialog_new(
        parent,                    /* janela pai */
        GTK_DIALOG_MODAL,          /* modal = bloqueia a janela pai */
        GTK_MESSAGE_ERROR,         /* ícone de erro */
        GTK_BUTTONS_OK,            /* botão OK */
        "%s", mensagem             /* a mensagem a exibir */
    );
    gtk_dialog_run(GTK_DIALOG(dlg));  /* espera o usuário fechar */
    gtk_widget_destroy(dlg);          /* libera o widget da memória */
}

/*
 * input_show_file_chooser - Abre o explorador de arquivos do sistema
 *
 * GtkFileChooserDialog é a janela padrão de "Abrir arquivo" do GNOME.
 * Configuramos filtros para só mostrar .txt e .pdf.
 *
 * GTK_FILE_CHOOSER_ACTION_OPEN = modo de ABERTURA (não de salvar)
 */
int input_show_file_chooser(GtkWindow *parent, SimConfig *cfg) {
    /* Cria o diálogo de escolha de arquivo */
    GtkWidget *dlg = gtk_file_chooser_dialog_new(
        "Importar configuração (TXT ou PDF)",   /* título da janela */
        parent,                                  /* janela pai */
        GTK_FILE_CHOOSER_ACTION_OPEN,            /* modo: abrir arquivo */
        "_Cancelar", GTK_RESPONSE_CANCEL,        /* botão Cancelar */
        "_Abrir",    GTK_RESPONSE_ACCEPT,        /* botão Abrir */
        NULL                                     /* fim da lista de botões */
    );

    /* ── Filtro para arquivos de texto ────────────────────────────────── */
    GtkFileFilter *filtro_txt = gtk_file_filter_new();
    gtk_file_filter_set_name(filtro_txt, "Arquivos de texto (*.txt)");
    gtk_file_filter_add_pattern(filtro_txt, "*.txt"); /* *.txt = qualquer .txt */
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filtro_txt);

    /* ── Filtro para arquivos PDF ──────────────────────────────────────── */
    GtkFileFilter *filtro_pdf = gtk_file_filter_new();
    gtk_file_filter_set_name(filtro_pdf, "Arquivos PDF (*.pdf)");
    gtk_file_filter_add_pattern(filtro_pdf, "*.pdf");
    gtk_file_filter_add_pattern(filtro_pdf, "*.PDF"); /* maiúsculas também */
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filtro_pdf);

    /* ── Filtro "todos os arquivos" ────────────────────────────────────── */
    GtkFileFilter *filtro_todos = gtk_file_filter_new();
    gtk_file_filter_set_name(filtro_todos, "Todos os arquivos (*.*)");
    gtk_file_filter_add_pattern(filtro_todos, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filtro_todos);

    /* Executa o diálogo — bloqueia até o usuário escolher ou cancelar */
    gint resposta = gtk_dialog_run(GTK_DIALOG(dlg));

    if (resposta != GTK_RESPONSE_ACCEPT) {
        /* Usuário cancelou */
        gtk_widget_destroy(dlg);
        return -1;
    }

    /* Pega o caminho do arquivo escolhido */
    char *filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    gtk_widget_destroy(dlg);  /* fecha o diálogo imediatamente */

    if (!filepath) return -1;

    /* Decide se é TXT ou PDF baseado na extensão */
    int resultado = -1;
    size_t len = strlen(filepath);

    if (len > 4 &&
        (strcasecmp(filepath + len - 4, ".pdf") == 0)) {
        /* É um PDF: usa o extrator */
        resultado = input_read_pdf(filepath, cfg);
    } else {
        /* Assume TXT para qualquer outra extensão */
        resultado = input_read_txt(filepath, cfg);
    }

    /* g_free() libera memória alocada pelo GTK (não usar free() aqui!) */
    g_free(filepath);

    if (resultado != 0) {
        mostrar_erro(parent,
            "Não foi possível carregar o arquivo.\n\n"
            "Verifique se o formato está correto:\n"
            "  Linha 1: ALGORITMO;QUANTUM;CPUS\n"
            "  Demais:  ID;COR;CHEGADA;DURAÇÃO;PRIORIDADE\n\n"
            "Exemplo:\n"
            "  SRTF;4;2\n"
            "  1;FF4444;0;10;3");
    }

    return resultado;
}

/* ══════════════════════════════════════════════════════════════════════════
 * PARTE 4 — DIÁLOGO DE ENTRADA MANUAL
 * ══════════════════════════════════════════════════════════════════════════ */

/* Número máximo de tarefas que o formulário suporta */
#define MAX_FORM_TASKS 20

/*
 * TarefaRow - Representa uma linha do formulário para uma tarefa
 *
 * Cada linha tem 5 widgets de entrada (um por campo).
 * Guardamos os ponteiros para poder ler os valores quando o
 * usuário clicar em OK.
 */
typedef struct {
    GtkWidget *entry_id;        /* Campo: ID (número) */
    GtkWidget *entry_cor;       /* Campo: Cor (hex RRGGBB) */
    GtkWidget *spin_chegada;    /* Campo: Tick de chegada */
    GtkWidget *spin_duracao;    /* Campo: Duração */
    GtkWidget *spin_prioridade; /* Campo: Prioridade */
    GtkWidget *btn_remover;     /* Botão para remover esta linha */
} TarefaRow;

/*
 * FormState - Estado do formulário de entrada manual
 *
 * Agrupa todos os widgets do formulário que precisam ser acessados
 * pelos callbacks de botão. Em GTK, passamos este ponteiro via
 * gpointer data nos g_signal_connect.
 */
typedef struct {
    GtkWidget  *combo_algo;      /* Lista suspensa: SRTF / PRIOP */
    GtkWidget  *spin_quantum;    /* Spin: quantum */
    GtkWidget  *spin_cpus;       /* Spin: número de CPUs */
    GtkWidget  *grid_tarefas;    /* Grid onde as linhas de tarefas ficam */
    TarefaRow   linhas[MAX_FORM_TASKS]; /* Array das linhas de tarefas */
    int         num_linhas;      /* Quantas linhas existem agora */
    GtkWidget  *dialogo;         /* Referência ao diálogo principal */
} FormState;

/*
 * criar_cabecalho_tarefas - Adiciona a linha de cabeçalho no grid
 *
 * O grid de tarefas tem:
 *   Coluna 0: ID
 *   Coluna 1: Cor
 *   Coluna 2: Chegada
 *   Coluna 3: Duração
 *   Coluna 4: Prioridade
 *   Coluna 5: Botão remover
 */
static void criar_cabecalho_tarefas(GtkGrid *grid) {
    const char *labels[] = {"ID", "Cor (hex)", "Chegada", "Duração", "Prioridade", ""};
    for (int col = 0; col < 6; col++) {
        GtkWidget *lbl = gtk_label_new(labels[col]);
        /* Deixa o texto em negrito usando Pango Markup */
        gchar *markup = g_markup_printf_escaped("<b>%s</b>", labels[col]);
        gtk_label_set_markup(GTK_LABEL(lbl), markup);
        g_free(markup);
        /* gtk_grid_attach(grid, widget, col, linha, col_span, row_span) */
        gtk_grid_attach(grid, lbl, col, 0, 1, 1);
    }
}

/*
 * cb_remover_linha - Callback do botão "X" de cada linha de tarefa
 *
 * Quando o usuário clica no X de uma linha, precisamos:
 *   1. Encontrar qual linha corresponde a este botão
 *   2. Remover os widgets desta linha do grid
 *   3. Mover as linhas seguintes uma posição para cima
 *   4. Decrementar o contador de linhas
 */
static void cb_remover_linha(GtkWidget *btn, gpointer data) {
    FormState *form = (FormState *)data;

    /* Encontra qual linha tem este botão */
    int linha_removida = -1;
    for (int i = 0; i < form->num_linhas; i++) {
        if (form->linhas[i].btn_remover == btn) {
            linha_removida = i;
            break;
        }
    }
    if (linha_removida < 0) return; /* segurança */

    /* Remove os 6 widgets desta linha do grid */
    /* Linha no grid = linha_removida + 1 (porque linha 0 é o cabeçalho) */
    GtkWidget *widgets[6] = {
        form->linhas[linha_removida].entry_id,
        form->linhas[linha_removida].entry_cor,
        form->linhas[linha_removida].spin_chegada,
        form->linhas[linha_removida].spin_duracao,
        form->linhas[linha_removida].spin_prioridade,
        form->linhas[linha_removida].btn_remover
    };
    for (int w = 0; w < 6; w++) {
        gtk_widget_destroy(widgets[w]);
    }

    /* Move as linhas seguintes uma posição para cima no array */
    for (int i = linha_removida; i < form->num_linhas - 1; i++) {
        form->linhas[i] = form->linhas[i + 1];
    }
    form->num_linhas--;
}

/*
 * cb_adicionar_linha - Adiciona uma nova linha de tarefa no formulário
 *
 * Cria 5 widgets de entrada + 1 botão de remover e os anexa ao grid.
 * A linha é posicionada logo após o cabeçalho e as linhas existentes.
 */
static void cb_adicionar_linha(GtkWidget *btn_add, gpointer data) {
    (void)btn_add;
    FormState *form = (FormState *)data;

    if (form->num_linhas >= MAX_FORM_TASKS) {
        g_print("[AVISO] Limite de %d tarefas atingido\n", MAX_FORM_TASKS);
        return;
    }

    int idx  = form->num_linhas;          /* índice da nova linha no array */
    int row  = idx + 1;                   /* linha no grid (+1 pelo cabeçalho) */
    TarefaRow *t = &form->linhas[idx];    /* ponteiro para a nova linha */

    /* ── Campo ID ─────────────────────────────────────────────────────── */
    t->entry_id = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(t->entry_id), 4);
    gtk_entry_set_width_chars(GTK_ENTRY(t->entry_id), 5);
    /* Valor padrão: próximo ID disponível */
    gchar id_str[8];
    snprintf(id_str, sizeof(id_str), "%d", idx + 1);
    gtk_entry_set_text(GTK_ENTRY(t->entry_id), id_str);
    gtk_grid_attach(GTK_GRID(form->grid_tarefas), t->entry_id, 0, row, 1, 1);

    /* ── Campo Cor (hex RRGGBB) ───────────────────────────────────────── */
    t->entry_cor = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(t->entry_cor), 6);
    gtk_entry_set_width_chars(GTK_ENTRY(t->entry_cor), 8);
    /* Cores padrão variadas para cada linha */
    const char *cores_padrao[] = {
        "FF4444", "44FF44", "4444FF", "FFAA00",
        "FF44FF", "44FFFF", "FF8800", "8800FF"
    };
    gtk_entry_set_text(GTK_ENTRY(t->entry_cor),
                       cores_padrao[idx % 8]);
    gtk_grid_attach(GTK_GRID(form->grid_tarefas), t->entry_cor, 1, row, 1, 1);

    /* ── Spin: Chegada ────────────────────────────────────────────────── */
    /* gtk_spin_button_new_with_range(min, max, passo) */
    t->spin_chegada = gtk_spin_button_new_with_range(0, 999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(t->spin_chegada), idx * 2);
    gtk_grid_attach(GTK_GRID(form->grid_tarefas), t->spin_chegada, 2, row, 1, 1);

    /* ── Spin: Duração ────────────────────────────────────────────────── */
    t->spin_duracao = gtk_spin_button_new_with_range(1, 999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(t->spin_duracao), 5);
    gtk_grid_attach(GTK_GRID(form->grid_tarefas), t->spin_duracao, 3, row, 1, 1);

    /* ── Spin: Prioridade ─────────────────────────────────────────────── */
    t->spin_prioridade = gtk_spin_button_new_with_range(1, 99, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(t->spin_prioridade), 3);
    gtk_grid_attach(GTK_GRID(form->grid_tarefas), t->spin_prioridade, 4, row, 1, 1);

    /* ── Botão remover ────────────────────────────────────────────────── */
    t->btn_remover = gtk_button_new_with_label("✕");
    g_signal_connect(t->btn_remover, "clicked",
                     G_CALLBACK(cb_remover_linha), form);
    gtk_grid_attach(GTK_GRID(form->grid_tarefas), t->btn_remover, 5, row, 1, 1);

    form->num_linhas++;

    /* Mostra os novos widgets (gtk_widget_show_all mostraria tudo, mas
     * aqui mostramos só os novos para eficiência) */
    gtk_widget_show_all(form->grid_tarefas);
}

/*
 * input_show_manual_dialog - Cria e exibe o formulário de entrada manual
 *
 * Estrutura visual do diálogo:
 *
 *   ┌─────────────────────────────────────────┐
 *   │ Algoritmo: [SRTF ▼]                     │
 *   │ Quantum:   [4 ▲▼]   CPUs: [2 ▲▼]       │
 *   │─────────────────────────────────────────│
 *   │ Tarefas:                                │
 *   │ ID  Cor     Chegada  Duração  Prior.    │
 *   │ [1] [FF4444] [0 ▲▼] [5 ▲▼]  [3 ▲▼] [✕]│
 *   │ [2] [44FF44] [2 ▲▼] [8 ▲▼]  [5 ▲▼] [✕]│
 *   │                                         │
 *   │         [+ Adicionar Tarefa]            │
 *   │─────────────────────────────────────────│
 *   │              [Cancelar]  [OK]           │
 *   └─────────────────────────────────────────┘
 */
int input_show_manual_dialog(GtkWindow *parent, SimConfig *cfg) {
    /* ── Cria o diálogo principal ──────────────────────────────────────
     * GTK_DIALOG_MODAL = bloqueia a janela pai enquanto aberto
     * GTK_DIALOG_DESTROY_WITH_PARENT = fecha junto com a janela pai */
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Configuração Manual da Simulação",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancelar", GTK_RESPONSE_CANCEL,
        "_OK",       GTK_RESPONSE_OK,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dlg), 600, 500);

    /* Estado do formulário — preenchido abaixo */
    FormState form;
    memset(&form, 0, sizeof(form));
    form.dialogo = dlg;

    /* ── Área de conteúdo do diálogo ───────────────────────────────────
     * gtk_dialog_get_content_area retorna o container interno do diálogo
     * onde colocamos nossos widgets (acima dos botões OK/Cancelar) */
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);

    /* ── Grid para os parâmetros gerais ────────────────────────────────
     * GtkGrid organiza widgets em uma grade de linhas e colunas
     * column_spacing = espaço horizontal entre colunas
     * row_spacing    = espaço vertical entre linhas */
    GtkWidget *grid_geral = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_geral), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid_geral), 8);
    gtk_box_pack_start(GTK_BOX(content), grid_geral, FALSE, FALSE, 0);

    /* ── Linha 0: Algoritmo ─────────────────────────────────────────── */
    gtk_grid_attach(GTK_GRID(grid_geral),
                    gtk_label_new("Algoritmo:"), 0, 0, 1, 1);

    form.combo_algo = gtk_combo_box_text_new();
    /* gtk_combo_box_text_append(combo, id_interno, texto_visível) */
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(form.combo_algo),
                              "SRTF", "SRTF — Menor Tempo Restante");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(form.combo_algo),
                              "PRIOP", "PRIOP — Prioridade Preemptivo");
    gtk_combo_box_set_active(GTK_COMBO_BOX(form.combo_algo), 0); /* seleciona SRTF */
    gtk_grid_attach(GTK_GRID(grid_geral), form.combo_algo, 1, 0, 2, 1);

    /* ── Linha 1: Quantum e CPUs ─────────────────────────────────────── */
    gtk_grid_attach(GTK_GRID(grid_geral),
                    gtk_label_new("Quantum:"), 0, 1, 1, 1);
    form.spin_quantum = gtk_spin_button_new_with_range(1, 99, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(form.spin_quantum), 4);
    gtk_grid_attach(GTK_GRID(grid_geral), form.spin_quantum, 1, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid_geral),
                    gtk_label_new("CPUs:"), 2, 1, 1, 1);
    form.spin_cpus = gtk_spin_button_new_with_range(2, MAX_CPUS, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(form.spin_cpus), 2);
    gtk_grid_attach(GTK_GRID(grid_geral), form.spin_cpus, 3, 1, 1, 1);

    /* ── Separador visual ───────────────────────────────────────────── */
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(content), sep, FALSE, FALSE, 8);

    /* ── Label "Tarefas" ─────────────────────────────────────────────── */
    GtkWidget *lbl_tarefas = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_tarefas), "<b>Tarefas</b>");
    gtk_widget_set_halign(lbl_tarefas, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), lbl_tarefas, FALSE, FALSE, 0);

    /* ── Área rolável para a lista de tarefas ──────────────────────────
     * GtkScrolledWindow adiciona barras de rolagem quando o conteúdo
     * é maior que a área visível */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC,   /* barra horizontal: automática */
        GTK_POLICY_AUTOMATIC    /* barra vertical:   automática */
    );
    gtk_widget_set_size_request(scroll, -1, 220); /* altura mínima */
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);

    /* ── Grid das tarefas (dentro da área rolável) ───────────────────── */
    form.grid_tarefas = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(form.grid_tarefas), 8);
    gtk_grid_set_row_spacing(GTK_GRID(form.grid_tarefas), 4);
    gtk_container_add(GTK_CONTAINER(scroll), form.grid_tarefas);

    /* Cria o cabeçalho das colunas */
    criar_cabecalho_tarefas(GTK_GRID(form.grid_tarefas));

    /* Adiciona 3 linhas de tarefa padrão para o usuário começar */
    for (int i = 0; i < 3; i++) {
        cb_adicionar_linha(NULL, &form);
    }

    /* ── Botão "+ Adicionar Tarefa" ──────────────────────────────────── */
    GtkWidget *btn_add = gtk_button_new_with_label("+ Adicionar Tarefa");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(cb_adicionar_linha), &form);
    gtk_box_pack_start(GTK_BOX(content), btn_add, FALSE, FALSE, 4);

    /* Mostra todos os widgets */
    gtk_widget_show_all(dlg);

    /* ── Executa o diálogo ─────────────────────────────────────────────
     * gtk_dialog_run() entra no loop de eventos e espera o usuário
     * clicar OK ou Cancelar. Retorna GTK_RESPONSE_OK ou GTK_RESPONSE_CANCEL */
    gint resposta = gtk_dialog_run(GTK_DIALOG(dlg));

    if (resposta != GTK_RESPONSE_OK) {
        gtk_widget_destroy(dlg);
        return -1; /* usuário cancelou */
    }

    /* ── Coleta os valores do formulário ─────────────────────────────── */

    /* Pega o ID do item selecionado no combo box */
    const gchar *algo_id = gtk_combo_box_get_active_id(
                               GTK_COMBO_BOX(form.combo_algo));
    int quantum = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(form.spin_quantum));
    int num_cpus = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(form.spin_cpus));

    if (form.num_linhas == 0) {
        mostrar_erro(GTK_WINDOW(dlg), "Adicione pelo menos uma tarefa!");
        gtk_widget_destroy(dlg);
        return -1;
    }

    /* Monta a string de configuração no formato aceito pelo parser */
    char config_str[4096];
    int pos = 0;
    pos += snprintf(config_str + pos, sizeof(config_str) - pos,
                    "%s;%d;%d\n", algo_id, quantum, num_cpus);

    for (int i = 0; i < form.num_linhas; i++) {
        TarefaRow *t = &form.linhas[i];
        const char *id_str  = gtk_entry_get_text(GTK_ENTRY(t->entry_id));
        const char *cor_str = gtk_entry_get_text(GTK_ENTRY(t->entry_cor));
        int chegada   = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(t->spin_chegada));
        int duracao   = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(t->spin_duracao));
        int prioridade = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(t->spin_prioridade));

        pos += snprintf(config_str + pos, sizeof(config_str) - pos,
                        "%s;%s;%d;%d;%d\n",
                        id_str, cor_str, chegada, duracao, prioridade);
    }

    gtk_widget_destroy(dlg);

    /* Faz o parse da string montada */
    int resultado = config_load_from_string(cfg, config_str);
    if (resultado != 0) {
        mostrar_erro(parent, "Erro ao processar os dados do formulário.\n"
                             "Verifique os valores digitados.");
    }
    return resultado;
}
