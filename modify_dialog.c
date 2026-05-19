/*
 * modify_dialog.c - Diálogo de modificação de tarefas durante a simulação
 *
 * Regras implementadas:
 *   NEW       → bloqueado, mensagem explicativa
 *   READY     → livre para modificar prioridade, tempo e estado
 *   RUNNING   → requer justificativa obrigatória, preempta e recalcula
 *   SUSPENDED → só prioridade (tempo bloqueado)
 *   FINISHED  → pode ser "ressuscitada" via combo de estado (ex: volta a READY)
 *
 * Combo de estado disponível para READY / RUNNING / SUSPENDED:
 *   → Pronta, Suspensa, Executando, Concluída (FINISHED)
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modify_dialog.h"
#include "simulator.h"
#include "task.h"

/* ── Cores e nomes dos estados (usados no badge e na lista) ──────────────── */
static const char *cor_estado(TaskState s) {
    switch(s) {
        case TASK_NEW:       return "#888888";
        case TASK_READY:     return "#2ecc71";
        case TASK_RUNNING:   return "#3498db";
        case TASK_SUSPENDED: return "#e67e22";
        case TASK_FINISHED:  return "#555555";
        default:             return "#aaaaaa";
    }
}

static const char *nome_estado(TaskState s) {
    switch(s) {
        case TASK_NEW:       return "NOVA";
        case TASK_READY:     return "PRONTA";
        case TASK_RUNNING:   return "EXECUTANDO";
        case TASK_SUSPENDED: return "SUSPENSA";
        case TASK_FINISHED:  return "CONCLUÍDA";
        default:             return "?";
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * sim_apply_modification
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Aplica prioridade e tempo restante novos no TCB e descarta snapshots
 * futuros para que a simulação recalcule a partir do tick atual.
 *
 * Parâmetros:
 *   nova_prior >= 0  → atualiza prioridade
 *   novo_rem   >= 1  → atualiza tempo restante
 *   -1 em qualquer um → não muda aquele campo
 */
void sim_apply_modification(SimState *sim, int task_id,
                             int nova_prior, int novo_rem) {
    TCB *t = NULL;
    for (int i = 0; i < sim->num_tasks; i++) {
        if (sim->tasks[i].id == task_id) { t = &sim->tasks[i]; break; }
    }
    if (!t) return;

    /* Aplica os novos valores numéricos */
    if (nova_prior >= 0) t->priority  = nova_prior;
    if (novo_rem   >= 1) t->remaining = novo_rem;

    /*
     * Se ainda estava RUNNING após a troca de estado feita pelo diálogo,
     * preempta: libera a CPU e manda a tarefa para READY.
     * (O diálogo pode já ter trocado o estado; esta é uma garantia extra.)
     */
    if (t->state == TASK_RUNNING) {
        int cpu = t->cpu_id;
        t->state  = TASK_READY;
        t->cpu_id = NO_TASK;
        task_add_history(t, sim->current_tick,
                         TASK_READY, NO_TASK, EVENT_PREEMPT);
        if (cpu != NO_TASK) {
            sim->cpus[cpu].task_id      = NO_TASK;
            sim->cpus[cpu].quantum_used = 0;
        }
    }

    /*
     * Trunca o histórico de snapshots até a posição atual.
     * Snapshots "futuros" (calculados com os valores antigos) são
     * descartados. Ao clicar "Avançar", a simulação recalculará
     * com os novos dados.
     */
    sim->history_count = sim->history_pos + 1;
    sim->finished      = 0;  /* garante que pode continuar simulando */

    /* Salva snapshot com o estado já modificado */
    sim_take_snapshot(sim);
}

/* ══════════════════════════════════════════════════════════════════════════
 * modify_show_dialog — Janela principal de modificação
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Layout:
 *  ┌──────────────────────────────────────────────────────┐
 *  │  Tarefa T3   [ ● EXECUTANDO ]                        │
 *  │──────────────────────────────────────────────────────│
 *  │  Chegada: 2    Duração original: 8                   │
 *  │  Prior. atual: 3   Tempo restante: 5                 │
 *  │  CPU atual: CPU 1  Tempo de espera: 2                │
 *  │──────────────────────────────────────────────────────│
 *  │  Nova prioridade:     [3 ▲▼]  (maior = prioritário)  │
 *  │  Novo tempo restante: [5 ▲▼]  (ticks necessários)    │
 *  │  Novo estado:         [Pronta ▼]                     │
 *  │──────────────────────────────────────────────────────│
 *  │  ⚠ Tarefa EXECUTANDO — justificativa obrigatória:   │
 *  │  [campo de texto livre________________________________]│
 *  │──────────────────────────────────────────────────────│
 *  │                   [Cancelar]  [Aplicar Modificação]  │
 *  └──────────────────────────────────────────────────────┘
 */
ModifyResult modify_show_dialog(GtkWindow *parent, SimState *sim, int task_id) {

    /* ── Localiza o TCB pelo ID ──────────────────────────────────────── */
    TCB *t = NULL;
    for (int i = 0; i < sim->num_tasks; i++) {
        if (sim->tasks[i].id == task_id) { t = &sim->tasks[i]; break; }
    }
    if (!t) return MODIFY_NEGADO;

    /* ── Bloqueia apenas TASK_NEW ────────────────────────────────────── */
    /*
     * TASK_FINISHED agora PODE ser modificada via combo de estado,
     * permitindo "ressuscitar" uma tarefa (ex: voltar para READY).
     * Apenas TASK_NEW é bloqueada pois ainda não entrou no sistema.
     */
    if (t->state == TASK_NEW) {
        GtkWidget *msg = gtk_message_dialog_new(
            parent, GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "A tarefa T%d ainda não chegou no sistema.\n\n"
            "Estado atual: NOVA\n\n"
            "Só é possível modificar tarefas que já estejam\n"
            "na fila de prontos, em execução ou concluídas.",
            t->id);
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
        return MODIFY_NEGADO;
    }

    /* ── Cria o diálogo principal ────────────────────────────────────── */
    char titulo[64];
    snprintf(titulo, sizeof(titulo), "Modificar Tarefa T%d", t->id);

    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        titulo, parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancelar",            GTK_RESPONSE_CANCEL,
        "_Aplicar Modificação", GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 460, -1);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);

    /* ── Cabeçalho: nome + badge colorido de estado ──────────────────── */
    GtkWidget *hbox_topo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_topo, FALSE, FALSE, 0);

    char nome_tarefa[32];
    snprintf(nome_tarefa, sizeof(nome_tarefa),
             "<b><big>Tarefa T%d</big></b>", t->id);
    GtkWidget *lbl_nome = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_nome), nome_tarefa);
    gtk_box_pack_start(GTK_BOX(hbox_topo), lbl_nome, FALSE, FALSE, 0);

    char badge[128];
    snprintf(badge, sizeof(badge),
             "<span background='%s' foreground='white'> <b>%s</b> </span>",
             cor_estado(t->state), nome_estado(t->state));
    GtkWidget *lbl_badge = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_badge), badge);
    gtk_box_pack_start(GTK_BOX(hbox_topo), lbl_badge, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    /* ── Grade de informações atuais (somente leitura) ───────────────── */
    GtkWidget *grid_info = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_info), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid_info), 6);
    gtk_box_pack_start(GTK_BOX(vbox), grid_info, FALSE, FALSE, 0);

    /*
     * Macro auxiliar: adiciona um par label/valor na grade.
     * col*2   = coluna do label (alinhado à direita)
     * col*2+1 = coluna do valor (alinhado à esquerda, em negrito)
     */
    #define ADD_INFO(col, row, label, valor) \
    { \
        GtkWidget *_l = gtk_label_new(label); \
        gtk_widget_set_halign(_l, GTK_ALIGN_END); \
        GtkWidget *_v = gtk_label_new(NULL); \
        gtk_label_set_markup(GTK_LABEL(_v), valor); \
        gtk_widget_set_halign(_v, GTK_ALIGN_START); \
        gtk_grid_attach(GTK_GRID(grid_info), _l, col*2,   row, 1, 1); \
        gtk_grid_attach(GTK_GRID(grid_info), _v, col*2+1, row, 1, 1); \
    }

    char buf[64];

    snprintf(buf, sizeof(buf), "<b>%d</b>", t->arrival);
    ADD_INFO(0, 0, "Chegada:", buf);

    snprintf(buf, sizeof(buf), "<b>%d</b>", t->duration);
    ADD_INFO(1, 0, "Duração original:", buf);

    snprintf(buf, sizeof(buf), "<b>%d</b>", t->priority);
    ADD_INFO(0, 1, "Prioridade atual:", buf);

    snprintf(buf, sizeof(buf), "<b>%d</b>", t->remaining);
    ADD_INFO(1, 1, "Tempo restante:", buf);

    if (t->cpu_id != NO_TASK)
        snprintf(buf, sizeof(buf), "<b>CPU %d</b>", t->cpu_id);
    else
        snprintf(buf, sizeof(buf), "<i>nenhuma</i>");
    ADD_INFO(0, 2, "CPU atual:", buf);

    snprintf(buf, sizeof(buf), "<b>%d</b>", t->wait_time);
    ADD_INFO(1, 2, "Tempo de espera:", buf);

    #undef ADD_INFO

    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    /* ── Grade de edição ─────────────────────────────────────────────── */
    GtkWidget *grid_edit = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_edit), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid_edit), 8);
    gtk_box_pack_start(GTK_BOX(vbox), grid_edit, FALSE, FALSE, 0);

    /* Linha 0: Nova prioridade */
    GtkWidget *lbl_prior = gtk_label_new("Nova prioridade:");
    gtk_widget_set_halign(lbl_prior, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid_edit), lbl_prior, 0, 0, 1, 1);

    GtkWidget *spin_prior = gtk_spin_button_new_with_range(1, 99, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_prior), t->priority);
    gtk_grid_attach(GTK_GRID(grid_edit), spin_prior, 1, 0, 1, 1);

    GtkWidget *lbl_hp = gtk_label_new("(maior valor = mais prioritário)");
    gtk_widget_set_halign(lbl_hp, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_hp), "dim-label");
    gtk_grid_attach(GTK_GRID(grid_edit), lbl_hp, 2, 0, 1, 1);

    /* Linha 1: Novo tempo restante */
    GtkWidget *lbl_rem = gtk_label_new("Novo tempo restante:");
    gtk_widget_set_halign(lbl_rem, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid_edit), lbl_rem, 0, 1, 1, 1);

    GtkWidget *spin_rem = gtk_spin_button_new_with_range(1, 999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_rem), t->remaining);
    /* Suspensa: tempo não pode ser alterado agora */
    if (t->state == TASK_SUSPENDED)
        gtk_widget_set_sensitive(spin_rem, FALSE);
    gtk_grid_attach(GTK_GRID(grid_edit), spin_rem, 1, 1, 1, 1);

    GtkWidget *lbl_hr = gtk_label_new("(ticks de CPU ainda necessários)");
    gtk_widget_set_halign(lbl_hr, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_hr), "dim-label");
    gtk_grid_attach(GTK_GRID(grid_edit), lbl_hr, 2, 1, 1, 1);

    /* ── Linha 2: Combo de novo estado ──────────────────────────────── */
    /*
     * Permite ao usuário trocar manualmente o estado da tarefa.
     * Opções disponíveis:
     *   • Pronta     (TASK_READY)     — volta para a fila
     *   • Suspensa   (TASK_SUSPENDED) — bloqueia a tarefa
     *   • Executando (TASK_RUNNING)   — tenta alocar uma CPU
     *   • Concluída  (TASK_FINISHED)  — marca como terminada manualmente
     *
     * "Concluída" é a opção nova — permite forçar o término de uma tarefa
     * antes de ela consumir todo o seu tempo restante.
     */
    GtkWidget *lbl_est = gtk_label_new("Novo estado:");
    gtk_widget_set_halign(lbl_est, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid_edit), lbl_est, 0, 2, 1, 1);

    GtkWidget *combo_estado = gtk_combo_box_text_new();
    /* A ordem aqui define o índice retornado por gtk_combo_box_get_active() */
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_estado), "Pronta");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_estado), "Suspensa");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_estado), "Executando");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_estado), "Concluída");

    /* Pré-seleciona o estado atual no combo */
    switch (t->state) {
        case TASK_READY:     gtk_combo_box_set_active(GTK_COMBO_BOX(combo_estado), 0); break;
        case TASK_SUSPENDED: gtk_combo_box_set_active(GTK_COMBO_BOX(combo_estado), 1); break;
        case TASK_RUNNING:   gtk_combo_box_set_active(GTK_COMBO_BOX(combo_estado), 2); break;
        case TASK_FINISHED:  gtk_combo_box_set_active(GTK_COMBO_BOX(combo_estado), 3); break;
        default:             gtk_combo_box_set_active(GTK_COMBO_BOX(combo_estado), 0); break;
    }
    gtk_grid_attach(GTK_GRID(grid_edit), combo_estado, 1, 2, 1, 1);

    GtkWidget *lbl_he = gtk_label_new("(muda o estado imediatamente)");
    gtk_widget_set_halign(lbl_he, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_he), "dim-label");
    gtk_grid_attach(GTK_GRID(grid_edit), lbl_he, 2, 2, 1, 1);

    /* ── Aviso especial para RUNNING: justificativa obrigatória ──────── */
    GtkWidget *entry_justif = NULL;

    if (t->state == TASK_RUNNING) {
        gtk_box_pack_start(GTK_BOX(vbox),
                           gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                           FALSE, FALSE, 0);

        GtkWidget *box_aviso = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_box_pack_start(GTK_BOX(vbox), box_aviso, FALSE, FALSE, 0);

        GtkWidget *lbl_aviso = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl_aviso),
            "<span foreground='#c0392b'><b>⚠  Atenção: tarefa em execução!</b></span>\n"
            "Ao aplicar, esta tarefa será <b>preemptada</b> imediatamente\n"
            "e voltará para a fila com os novos valores.\n"
            "A simulação recalculará a partir deste tick.");
        gtk_widget_set_halign(lbl_aviso, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(box_aviso), lbl_aviso, FALSE, FALSE, 0);

        GtkWidget *hbox_just = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_pack_start(GTK_BOX(box_aviso), hbox_just, FALSE, FALSE, 4);

        gtk_box_pack_start(GTK_BOX(hbox_just),
                           gtk_label_new("Justificativa (opcional):"),
                           FALSE, FALSE, 0);

        entry_justif = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry_justif),
            "Ex: corrigindo prioridade, ajuste de carga...");
        gtk_entry_set_max_length(GTK_ENTRY(entry_justif), 200);
        gtk_widget_set_hexpand(entry_justif, TRUE);
        gtk_box_pack_start(GTK_BOX(hbox_just), entry_justif, TRUE, TRUE, 0);
    }

    /* ── Aviso informativo para SUSPENDED ────────────────────────────── */
    if (t->state == TASK_SUSPENDED) {
        GtkWidget *lbl_sus = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl_sus),
            "<span foreground='#e67e22'>"
            "<b>ℹ  Tarefa suspensa (aguardando recurso)</b></span>\n"
            "Só a prioridade e o estado podem ser alterados agora.\n"
            "O tempo restante será recalculado quando a tarefa retomar.");
        gtk_widget_set_halign(lbl_sus, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(vbox), lbl_sus, FALSE, FALSE, 0);
    }

    /* ── Aviso para FINISHED ─────────────────────────────────────────── */
    if (t->state == TASK_FINISHED) {
        GtkWidget *lbl_fin = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl_fin),
            "<span foreground='#8e44ad'>"
            "<b>ℹ  Tarefa concluída</b></span>\n"
            "Você pode alterar o estado para <b>Pronta</b> para recolocar\n"
            "a tarefa na fila com o tempo restante informado acima.\n"
            "A simulação recalculará a partir deste tick.");
        gtk_widget_set_halign(lbl_fin, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(vbox), lbl_fin, FALSE, FALSE, 0);
        /* Para FINISHED, habilita o campo de tempo restante */
        gtk_widget_set_sensitive(spin_rem, TRUE);
    }

    gtk_widget_show_all(dlg);

    /* ══════════════════════════════════════════════════════════════════
     * Loop de resposta
     * Fica no loop até o usuário clicar OK com dados válidos
     * ou clicar Cancelar.
     * ══════════════════════════════════════════════════════════════════ */
    ModifyResult resultado = MODIFY_CANCELADO;

    while (1) {
        /* ── Coleta os valores do diálogo quando OK é pressionado ── */
        gint resp = gtk_dialog_run(GTK_DIALOG(dlg));

        if (resp != GTK_RESPONSE_OK) {
            resultado = MODIFY_CANCELADO;
            break;
        }

        /* ── Justificativa: só registra no log, não é obrigatória ──── */
        if (t->state == TASK_RUNNING && entry_justif) {
            const char *just = gtk_entry_get_text(GTK_ENTRY(entry_justif));
            if (strlen(just) > 0) {
                printf("[MODIFICAÇÃO] Tarefa T%d: %s\n", t->id, just);
            } else {
                printf("[MODIFICAÇÃO] Tarefa T%d modificada sem justificativa.\n", t->id);
            }
            printf("[TICK] %d\n\n", sim->current_tick);
        }

        /* ── COLETA DOS VALORES ───────────────────────────────────── */
        int nova_prior = (int)gtk_spin_button_get_value(
                             GTK_SPIN_BUTTON(spin_prior));

        int novo_rem   = (t->state == TASK_SUSPENDED)
                         ? -1   /* suspenso: não muda tempo */
                         : (int)gtk_spin_button_get_value(
                               GTK_SPIN_BUTTON(spin_rem));

        /* ── NOVO: captura o estado selecionado no combo ─────────── */
        /*
         * Mapeamento dos índices do combo para os estados:
         *   0 → TASK_READY     (Pronta)
         *   1 → TASK_SUSPENDED (Suspensa)
         *   2 → TASK_RUNNING   (Executando)
         *   3 → TASK_FINISHED  (Concluída)
         */
        int novo_estado = -1;  /* -1 = não muda o estado */
        int active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_estado));
        if      (active == 0) novo_estado = TASK_READY;
        else if (active == 1) novo_estado = TASK_SUSPENDED;
        else if (active == 2) novo_estado = TASK_RUNNING;
        else if (active == 3) novo_estado = TASK_FINISHED;

        /* ── Aplica mudança de estado ANTES de sim_apply_modification ─
         *
         * A troca de estado precisa acontecer primeiro porque
         * sim_apply_modification lê t->state para decidir se preempta.
         * Se o usuário pediu FINISHED, marcamos o finish_tick antes.
         */
        if (novo_estado != -1 && novo_estado != (int)t->state) {

            /* Se estava RUNNING e vai sair: libera a CPU */
            if (t->state == TASK_RUNNING && t->cpu_id != NO_TASK) {
                sim->cpus[t->cpu_id].task_id      = NO_TASK;
                sim->cpus[t->cpu_id].quantum_used = 0;
                t->cpu_id = NO_TASK;
            }

            /* Atualiza o estado */
            t->state = (TaskState)novo_estado;

            /* Tratamento especial por estado destino */
            if (novo_estado == TASK_RUNNING) {
                /*
                 * Tenta alocar uma CPU livre para a tarefa.
                 * Se não houver CPU disponível, rebaixa para READY.
                 */
                for (int c = 0; c < sim->num_cpus; c++) {
                    if (sim->cpus[c].task_id == NO_TASK && !sim->cpus[c].is_off) {
                        sim->cpus[c].task_id      = t->id;
                        sim->cpus[c].quantum_used = 0;
                        t->cpu_id = c;
                        break;
                    }
                }
                if (t->cpu_id == NO_TASK) {
                    /* Sem CPU disponível: vai para READY mesmo */
                    t->state = TASK_READY;
                }

            } else if (novo_estado == TASK_FINISHED) {
                /*
                 * Marcar como concluída manualmente:
                 * - define finish_tick como o tick atual
                 * - calcula o turnaround
                 * - zera o remaining (tarefa concluída não tem tempo pendente)
                 */
                t->finish_tick = sim->current_tick;
                t->turnaround  = sim->current_tick - t->arrival;
                t->remaining   = 0;
                novo_rem       = -1; /* não sobrescreve o 0 que acabamos de setar */
            }

            /* Registra a mudança de estado no histórico da tarefa */
            task_add_history(t, sim->current_tick,
                             t->state, t->cpu_id, EVENT_START);
        }

        /* ── Aplica modificações de prioridade e tempo restante ───── */
        sim_apply_modification(sim, task_id, nova_prior, novo_rem);
        resultado = MODIFY_APLICADO;
        break;
    }

    gtk_widget_destroy(dlg);
    return resultado;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Lista de tarefas — permite escolher qual tarefa modificar
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * DadosLista - Dados passados para o callback do botão "Modificar →"
 * de cada linha da lista. Usamos malloc para garantir que os dados
 * persistam até o callback ser executado.
 */
typedef struct {
    GtkWidget *janela_lista;  /* referência à janela da lista */
    SimState  *sim;           /* estado da simulação          */
    int        task_id;       /* ID da tarefa desta linha     */
    GtkWidget *janela_pai;    /* janela pai original          */
} DadosLista;

/*
 * cb_modificar_da_lista - Chamado ao clicar "Modificar →" em uma linha
 *
 * Esconde a lista, abre o diálogo de modificação e:
 *   - Se aplicado: fecha a lista (gui_gantt redesenhará)
 *   - Se cancelado: volta a mostrar a lista
 */
static void cb_modificar_da_lista(GtkWidget *btn, gpointer data) {
    (void)btn;
    DadosLista *d = (DadosLista *)data;

    gtk_widget_hide(d->janela_lista);

    ModifyResult res = modify_show_dialog(
        GTK_WINDOW(d->janela_pai), d->sim, d->task_id);

    if (res == MODIFY_APLICADO) {
        gtk_widget_destroy(d->janela_lista);
    } else {
        gtk_widget_show(d->janela_lista);
    }
    free(d);
}

/*
 * modify_show_task_list - Janela com lista de todas as tarefas
 *
 * Exibe uma linha por tarefa com: ID, badge de estado, prioridade,
 * tempo restante e botão "Modificar →".
 *
 * TASK_NEW fica desabilitado (botão cinza).
 * TASK_FINISHED agora fica HABILITADO — pode ser reativada.
 *
 * Layout:
 *  ┌──────────────────────────────────────────────────────┐
 *  │  Selecione a tarefa que deseja modificar:            │
 *  │──────────────────────────────────────────────────────│
 *  │  T1  [ EXECUTANDO ]  prior=3  rest=7   [Modificar →] │
 *  │  T2  [ PRONTA     ]  prior=5  rest=4   [Modificar →] │
 *  │  T3  [ CONCLUÍDA  ]  prior=1  rest=0   [Modificar →] │
 *  │  T4  [ NOVA       ]  prior=2  rest=8   [desabilitado] │
 *  │──────────────────────────────────────────────────────│
 *  │                                        [Fechar]      │
 *  └──────────────────────────────────────────────────────┘
 */
void modify_show_task_list(GtkWindow *parent, SimState *sim) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Modificar Tarefa Durante a Simulação",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Fechar", GTK_RESPONSE_CLOSE,
        NULL);

    /*
     * [ALTERADO] Altura fixa de 480px na janela.
     * Antes era gtk_window_set_default_size(500, -1), ou seja, altura
     * automática — a janela crescia indefinidamente com muitas tarefas
     * e saía da tela. Com altura fixa o GtkScrolledWindow abaixo
     * entra em ação quando as linhas não cabem.
     */
    gtk_window_set_default_size(GTK_WINDOW(dlg), 520, 480);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    /* Instruções */
    GtkWidget *lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl),
        "<b>Selecione a tarefa que deseja modificar:</b>\n"
        "<small>Tarefas NOVAS não podem ser modificadas. "
        "Tarefas CONCLUÍDAS podem ser reativadas.</small>");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 6);

    /*
     * [NOVO] GtkScrolledWindow envolvendo o grid de tarefas.
     *
     * Por que foi adicionado:
     *   Antes o grid era adicionado direto no content do diálogo.
     *   Com 30 tarefas (como no caso-teste-mc-005) a janela crescia
     *   para fora da tela e não havia como rolar para ver as demais.
     *
     * GTK_POLICY_NEVER      = barra horizontal nunca aparece
     *   (as colunas têm largura fixa, não precisa rolar na horizontal)
     * GTK_POLICY_AUTOMATIC  = barra vertical aparece só quando necessário
     *   (com poucas tarefas fica invisível; com muitas aparece sozinha)
     *
     * TRUE, TRUE no gtk_box_pack_start = o scroll ocupa todo o espaço
     * vertical disponível na janela (expand + fill).
     */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER,      /* horizontal: nunca rola */
        GTK_POLICY_AUTOMATIC   /* vertical: rola quando necessário */
    );
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);

    /* Grade: uma linha por tarefa — agora fica DENTRO do scroll */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 4); /* pequena margem interna */

    /*
     * [ALTERADO] Antes: gtk_box_pack_start(content, grid, ...)
     * Agora: gtk_container_add(scroll, grid)
     * O grid vai para dentro do GtkScrolledWindow, não direto no diálogo.
     */
    gtk_container_add(GTK_CONTAINER(scroll), grid);

    for (int i = 0; i < sim->num_tasks; i++) {
        TCB *t = &sim->tasks[i];

        /* Coluna 0: ID da tarefa */
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "<b>T%d</b>", t->id);
        GtkWidget *lbl_id = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl_id), id_str);
        gtk_widget_set_halign(lbl_id, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl_id, 0, i, 1, 1);

        /* Coluna 1: Badge colorido de estado */
        char badge[128];
        snprintf(badge, sizeof(badge),
                 "<span background='%s' foreground='white'>"
                 "<small> %s </small></span>",
                 cor_estado(t->state), nome_estado(t->state));
        GtkWidget *lbl_est = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl_est), badge);
        gtk_widget_set_halign(lbl_est, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl_est, 1, i, 1, 1);

        /* Coluna 2: Prioridade e tempo restante */
        char info[64];
        snprintf(info, sizeof(info),
                 "prior=<b>%d</b>  rest=<b>%d</b>",
                 t->priority, t->remaining);
        GtkWidget *lbl_inf = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl_inf), info);
        gtk_widget_set_halign(lbl_inf, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl_inf, 2, i, 1, 1);

        /* Coluna 3: Botão modificar
         * Desabilitado apenas para TASK_NEW.
         * TASK_FINISHED agora está habilitado. */
        GtkWidget *btn = gtk_button_new_with_label("Modificar →");
        gtk_widget_set_sensitive(btn, t->state != TASK_NEW);
        gtk_grid_attach(GTK_GRID(grid), btn, 3, i, 1, 1);

        /* Conecta o clique alocando os dados na heap */
        DadosLista *dados = (DadosLista *)malloc(sizeof(DadosLista));
        dados->janela_lista = dlg;
        dados->sim          = sim;
        dados->task_id      = t->id;
        dados->janela_pai   = GTK_WIDGET(parent);
        g_signal_connect(btn, "clicked",
                         G_CALLBACK(cb_modificar_da_lista), dados);
    }

    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}