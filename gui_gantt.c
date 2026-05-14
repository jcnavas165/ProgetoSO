/* 
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 * gui_gantt.c - Janela GTK principal com Gantt, importação e entrada manual
 */

#include <gtk/gtk.h> // Biblioteca principal do GTK 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_gantt.h"
#include "simulator.h"
#include "gantt.h"
#include "config.h"
#include "task.h"
#include "input_dialog.h"
#include "modify_dialog.h"   // diálogo de modificação de tarefas 

//Dimensões do Gantt 
#define G_MARGEM_E  90
#define G_MARGEM_T  45
#define G_MARGEM_B  55
#define G_CELL_W    42
#define G_CELL_H    44

// Estado global da janela 
typedef struct {
    //Simulação 
    SimConfig  config;
    SimState   sim;
    int        config_carregada;  // 0 = sem config ainda 

    // Widgets que precisamos acessar nos callbacks 
    GtkWidget *janela;
    GtkWidget *area;
    GtkWidget *lbl_status;
    GtkWidget *btn_avancar;
    GtkWidget *btn_retroceder;
    GtkWidget *btn_tudo;
    GtkWidget *btn_salvar;
    GtkWidget *btn_modificar;    // modificar tarefa em execução
    GtkWidget *btn_nova_sim;     //iniciar nova simulação
} AppState;

//Utilitários 

// Converte cor hex RRGGBB para doubles 0.0-1.0 para Cairo 
static void hex_para_rgb(const char *hex, double *r, double *g, double *b) {
    unsigned int ri, gi, bi;
    const char *h = (hex[0] == '#') ? hex+1 : hex;
    if (sscanf(h, "%02x%02x%02x", &ri, &gi, &bi) == 3) {
        *r = ri/255.0; *g = gi/255.0; *b = bi/255.0;
    } else {
        *r = 0.5; *g = 0.5; *b = 0.5;
    }
}

//Atualiza label de status e sensibilidade dos botões 
static void atualizar_ui(AppState *app) {
    if (!app->config_carregada) {
        gtk_label_set_text(GTK_LABEL(app->lbl_status),
                           "Nenhuma configuração carregada");
        gtk_widget_set_sensitive(app->btn_avancar,    FALSE);
        gtk_widget_set_sensitive(app->btn_retroceder, FALSE);
        gtk_widget_set_sensitive(app->btn_tudo,       FALSE);
        gtk_widget_set_sensitive(app->btn_salvar,     FALSE);
        gtk_widget_set_sensitive(app->btn_modificar,  FALSE);
        return;
    }

    //Monta o texto de status 
    char txt[200];
    const char *status_sim = app->sim.finished ? " ✓ CONCLUÍDA" : "";
    snprintf(txt, sizeof(txt),
             "Tick: %d  |  Tarefas: %d  |  CPUs: %d  |  %s%s",
             app->sim.current_tick,
             app->sim.num_tasks,
             app->sim.num_cpus,
             algo_enum_to_name(app->config.algo),
             status_sim);
    gtk_label_set_text(GTK_LABEL(app->lbl_status), txt);

    gtk_widget_set_sensitive(app->btn_avancar,    !app->sim.finished);
    gtk_widget_set_sensitive(app->btn_retroceder, app->sim.history_pos > 0);
    gtk_widget_set_sensitive(app->btn_tudo,       !app->sim.finished);
    gtk_widget_set_sensitive(app->btn_salvar,      TRUE);
    // Modificar: só quando há simulação em andamento
    gtk_widget_set_sensitive(app->btn_modificar,  app->sim.current_tick > 0);
}

//Inicializa/reinicia o simulador com a config carregada 
static int iniciar_simulacao(AppState *app) {
    //libera histórico anterior se existir
    if (app->sim.history) { free(app->sim.history); app->sim.history = NULL; }

    if (sim_init(&app->sim, &app->config) != 0) {
        GtkWidget *dlg = gtk_message_dialog_new(
            GTK_WINDOW(app->janela), GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Falha ao inicializar a simulação.");
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
        return -1;
    }
    app->config_carregada = 1;
    return 0;
}

//calback de desenho Cairo
static gboolean cb_desenhar(GtkWidget *widget, cairo_t *cr, gpointer data) {
    AppState *app = (AppState *)data;

    int W = gtk_widget_get_allocated_width(widget);
    int H = gtk_widget_get_allocated_height(widget);

    // Fundo 
    cairo_set_source_rgb(cr, 0.13, 0.13, 0.18);
    cairo_paint(cr);

    //ela de boa vindas quando não há config
    if (!app->config_carregada) {
        cairo_set_source_rgb(cr, 0.55, 0.55, 0.65);
        cairo_select_font_face(cr, "Sans",
                               CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 18);
        const char *msg1 = "Nenhuma configuração carregada";
        cairo_text_extents_t ext;
        cairo_text_extents(cr, msg1, &ext);
        cairo_move_to(cr, (W - ext.width)/2, H/2 - 20);
        cairo_show_text(cr, msg1);

        cairo_set_font_size(cr, 13);
        cairo_set_source_rgb(cr, 0.4, 0.4, 0.5);
        const char *msg2 = "Use 'Importar TXT ' ou 'Entrada Manual' acima";
        cairo_text_extents(cr, msg2, &ext);
        cairo_move_to(cr, (W - ext.width)/2, H/2 + 14);
        cairo_show_text(cr, msg2);
        return FALSE;
    }

    //Gantt real 
    SimState *sim = &app->sim;
    int num_ticks = sim->current_tick > 0 ? sim->current_tick : 1;
    int num_tasks = sim->num_tasks;

    //Ordena tarefas por ID crescente
    int order[MAX_TASKS];
    for (int i = 0; i < num_tasks; i++) order[i] = i;
    for (int i = 0; i < num_tasks-1; i++)
        for (int j = 0; j < num_tasks-1-i; j++)
            if (sim->tasks[order[j]].id > sim->tasks[order[j+1]].id) {
                int tmp=order[j]; order[j]=order[j+1]; order[j+1]=tmp;
            }

    //titulo
    cairo_set_source_rgb(cr, 0.75, 0.85, 1.0);
    cairo_select_font_face(cr, "Sans",
                           CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 13);
    char titulo[128];
    snprintf(titulo, sizeof(titulo),
             "Gantt — %s  |  Quantum: %d  |  CPUs: %d",
             algo_enum_to_name(sim->config->algo),
             sim->config->quantum,
             sim->config->num_cpus);
    cairo_move_to(cr, G_MARGEM_E, 26);
    cairo_show_text(cr, titulo);

    //tasks
    for (int oi = 0; oi < num_tasks; oi++) {
        int idx = order[oi];
        TCB *t  = &sim->tasks[idx];
        int row = num_tasks - 1 - oi;
        int y   = G_MARGEM_T + row * G_CELL_H;

        double tr, tg, tb;
        hex_para_rgb(t->color, &tr, &tg, &tb);

        // Label lateral 
        cairo_set_source_rgb(cr, 0.75, 0.75, 0.85);
        cairo_select_font_face(cr, "Monospace",
                               CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12);
        char lbl[12];
        snprintf(lbl, sizeof(lbl), "T%d", t->id);
        cairo_move_to(cr, 6, y + G_CELL_H/2 + 5);
        cairo_show_text(cr, lbl);

        //blocos por tick 
        for (int tick = 0; tick < num_ticks; tick++) {
            int x = G_MARGEM_E + tick * G_CELL_W;

            //etado neste tick via histoorico
            int last_state = -1, last_cpu = NO_TASK;
            EventType last_event = EVENT_ARRIVAL;
            if (tick >= t->arrival) {
                last_state = TASK_READY;
                for (int hi = 0; hi < t->history_count; hi++) {
                    if (t->history[hi].tick <= tick) {
                        last_state = t->history[hi].state;
                        last_cpu   = t->history[hi].cpu_id;
                        last_event = t->history[hi].event;
                    }
                }
            }

            //correção: se a tarefa já terminou e este tick é >= finish_tick, força o estado para FINISHED  eevita que o último bloco fique colorido como RUNNING após o término.
            if (t->finish_tick >= 0 && tick >= t->finish_tick) {
                last_state = TASK_FINISHED;
                last_cpu   = NO_TASK;
            }

            //Cor do bloco 
            if      (last_state < 0)              cairo_set_source_rgb(cr, 0.10,0.10,0.15);
            else if (last_state == TASK_RUNNING)  cairo_set_source_rgb(cr, tr,tg,tb);
            else if (last_state == TASK_SUSPENDED)cairo_set_source_rgb(cr, 0.15,0.15,0.15);
            else if (last_state == TASK_FINISHED) cairo_set_source_rgb(cr, 0.12,0.12,0.18);
            else                                  cairo_set_source_rgba(cr,1,1,1,0.05);

            cairo_rectangle(cr, x+1, y+2, G_CELL_W-2, G_CELL_H-4);
            cairo_fill(cr);

            //borda
            cairo_set_source_rgba(cr, 1,1,1, 0.07);
            cairo_set_line_width(cr, 0.6);
            cairo_rectangle(cr, x+1, y+2, G_CELL_W-2, G_CELL_H-4);
            cairo_stroke(cr);

            //Label CPU dentro do bloco 
            if (last_state == TASK_RUNNING && last_cpu != NO_TASK) {
                cairo_set_source_rgba(cr, 0,0,0, 0.55);
                cairo_select_font_face(cr, "Monospace",
                                       CAIRO_FONT_SLANT_NORMAL,
                                       CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 10);
                char clbl[6];
                snprintf(clbl, sizeof(clbl), "C%d", last_cpu);
                cairo_move_to(cr, x+G_CELL_W/2-8, y+G_CELL_H/2+4);
                cairo_show_text(cr, clbl);
            }

            // xhegada: triângulo verde 
            if (last_event == EVENT_ARRIVAL && last_state >= 0 && last_state != TASK_SUSPENDED) {
                cairo_set_source_rgb(cr, 0.0, 0.85, 0.35);
                cairo_move_to(cr, x+3,  y+G_CELL_H-3);
                cairo_line_to(cr, x+10, y+G_CELL_H-13);
                cairo_line_to(cr, x+17, y+G_CELL_H-3);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            // término quadrado vermelho
            if (tick == t->finish_tick) {
                cairo_set_source_rgb(cr, 0.9, 0.15, 0.15);
                cairo_rectangle(cr, x+3, y+G_CELL_H-14, 10, 10);
                cairo_fill(cr);
}
            //sorteio asterisco laranja 
            if (last_event == EVENT_LOTTERY) {
                cairo_set_source_rgb(cr, 1.0, 0.55, 0.0);
                cairo_set_font_size(cr, 13);
                cairo_select_font_face(cr,"Sans",CAIRO_FONT_SLANT_NORMAL,
                                       CAIRO_FONT_WEIGHT_BOLD);
                cairo_move_to(cr, x+G_CELL_W/2-5, y+14);
                cairo_show_text(cr, "*");
            }
        }
    }

    // Eixo X 
    int eixo_y = G_MARGEM_T + num_tasks * G_CELL_H;
    cairo_set_source_rgb(cr, 0.35,0.35,0.45);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, G_MARGEM_E, eixo_y);
    cairo_line_to(cr, G_MARGEM_E + num_ticks*G_CELL_W, eixo_y);
    cairo_stroke(cr);

    cairo_select_font_face(cr,"Monospace",CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);
    for (int t = 0; t <= num_ticks; t++) {
        int x = G_MARGEM_E + t*G_CELL_W;
        cairo_set_source_rgb(cr, 0.4,0.4,0.5);
        cairo_move_to(cr, x, eixo_y); cairo_line_to(cr, x, eixo_y+4); cairo_stroke(cr);
        char n[16]; snprintf(n, sizeof(n), "%d", t);
        cairo_set_source_rgb(cr, 0.5,0.5,0.6);
        cairo_move_to(cr, x-4, eixo_y+16);
        cairo_show_text(cr, n);
    }
    cairo_set_source_rgb(cr,0.4,0.4,0.5);
    cairo_move_to(cr, G_MARGEM_E+num_ticks*G_CELL_W/2-30, eixo_y+34);
    cairo_show_text(cr, "tempo (ticks)");

    //Legenda 
    int lx = G_MARGEM_E, ly = eixo_y + 48;
    struct { double r,g,b; const char *n; } leg[]={
        {0.9,0.3,0.3,"Executando"},{0.2,0.2,0.3,"Pronta"},
        {0.15,0.15,0.15,"Suspensa"}
    };
    cairo_set_font_size(cr,11);
    for(int i=0;i<3;i++){
        cairo_set_source_rgb(cr,leg[i].r,leg[i].g,leg[i].b);
        cairo_rectangle(cr,lx+i*100,ly-11,14,12); cairo_fill(cr);
        cairo_set_source_rgba(cr,1,1,1,0.25);
        cairo_set_line_width(cr,0.5);
        cairo_rectangle(cr,lx+i*100,ly-11,14,12); cairo_stroke(cr);
        cairo_set_source_rgb(cr,0.65,0.65,0.75);
        cairo_select_font_face(cr,"Sans",CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_move_to(cr,lx+i*100+18,ly);
        cairo_show_text(cr,leg[i].n);
    }

    return FALSE;
}

//callbacks dos botao

static void cb_avancar(GtkWidget *btn, gpointer data) {
    (void)btn;
    AppState *app = (AppState*)data;
    if (app->config_carregada && !app->sim.finished) {
        sim_step(&app->sim);
        gtk_widget_queue_draw(app->area);
        atualizar_ui(app);
    }
}

static void cb_retroceder(GtkWidget *btn, gpointer data) {
    (void)btn;
    AppState *app = (AppState*)data;
    if (app->config_carregada) {
        sim_step_back(&app->sim);
        gtk_widget_queue_draw(app->area);
        atualizar_ui(app);
    }
}

static void cb_executar_tudo(GtkWidget *btn, gpointer data) {
    (void)btn;
    AppState *app = (AppState*)data;
    if (app->config_carregada && !app->sim.finished) {
        sim_run_full(&app->sim);
        gtk_widget_queue_draw(app->area);
        atualizar_ui(app);
    }
}

static void cb_salvar_svg(GtkWidget *btn, gpointer data) {
    (void)btn;
    AppState *app = (AppState*)data;
    if (!app->config_carregada) return;

    //dálogo para escolher onde salvar
    GtkWidget *dlg = gtk_file_chooser_dialog_new(
        "Salvar Gantt como SVG",
        GTK_WINDOW(app->janela),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancelar", GTK_RESPONSE_CANCEL,
        "_Salvar",   GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), "gantt.svg");

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        char *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        gantt_generate_svg(&app->sim, fname);
        g_free(fname);

        GtkWidget *ok = gtk_message_dialog_new(
            GTK_WINDOW(app->janela), GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "Gráfico SVG salvo com sucesso!\nAbra no navegador para visualizar.");
        gtk_dialog_run(GTK_DIALOG(ok));
        gtk_widget_destroy(ok);
    }
    gtk_widget_destroy(dlg);
}

// cb_modificar_tarefa abre a lista de tarefas para modificar, quando retorna, redesenha o canvas pois o estado pode ter mudado
static void cb_modificar_tarefa(GtkWidget *btn, gpointer data) {
    (void)btn;
    AppState *app = (AppState*)data;
    if (!app->config_carregada) return;

    modify_show_task_list(GTK_WINDOW(app->janela), &app->sim);

    //redesenha sempre que voltar da lista pde ter havido modificação
    gtk_widget_queue_draw(app->area);
    atualizar_ui(app);
}

//cb_nova_simulacao recomeça a simulação sem fechar o programa, pergunta se quer usar a mesma configuração ou carregar uma nova
static void cb_nova_simulacao(GtkWidget *btn, gpointer data) {
    (void)btn;
    AppState *app = (AppState*)data;

    //Diálogo de confirmação com opções
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Nova Simulação",
        GTK_WINDOW(app->janela),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancelar",             GTK_RESPONSE_CANCEL,
        "Mesma configuração",    GTK_RESPONSE_YES,
        "Nova configuração",     GTK_RESPONSE_NO,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);

    GtkWidget *lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl),
        "<b>Iniciar nova simulação</b>\n\n"
        "Como deseja prosseguir?\n\n"
        "• <b>Mesma configuração</b>: reinicia do zero com as mesmas tarefas\n"
        "• <b>Nova configuração</b>: importa um novo arquivo ou preenche manualmente");
    gtk_box_pack_start(GTK_BOX(content), lbl, FALSE, FALSE, 0);
    gtk_widget_show_all(dlg);

    gint resp = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    if (resp == GTK_RESPONSE_CANCEL) return;

    if (resp == GTK_RESPONSE_YES) {
        //Reinicia com a mesma configuração 
        if (app->config_carregada) {
            if (app->sim.history) { free(app->sim.history); app->sim.history = NULL; }
            if (sim_init(&app->sim, &app->config) == 0) {
                gtk_widget_queue_draw(app->area);
                atualizar_ui(app);
            }
        }
    } else if (resp == GTK_RESPONSE_NO) {
        //abre o diálogo de importar/manual 
        GtkWidget *dlg2 = gtk_dialog_new_with_buttons(
            "Como carregar a nova configuração?",
            GTK_WINDOW(app->janela),
            GTK_DIALOG_MODAL,
            "_Cancelar",       GTK_RESPONSE_CANCEL,
            "📂 Importar arquivo", 1,
            "✏ Entrada manual",   2,
            NULL);
        GtkWidget *c2 = gtk_dialog_get_content_area(GTK_DIALOG(dlg2));
        gtk_container_set_border_width(GTK_CONTAINER(c2), 16);
        GtkWidget *l2 = gtk_label_new("Escolha como carregar a nova configuração:");
        gtk_box_pack_start(GTK_BOX(c2), l2, FALSE, FALSE, 0);
        gtk_widget_show_all(dlg2);

        gint resp2 = gtk_dialog_run(GTK_DIALOG(dlg2));
        gtk_widget_destroy(dlg2);

        int ret = -1;
        if (resp2 == 1) {
            ret = input_show_file_chooser(GTK_WINDOW(app->janela), &app->config);
        } else if (resp2 == 2) {
            ret = input_show_manual_dialog(GTK_WINDOW(app->janela), &app->config);
        }

        if (ret == 0) {
            if (iniciar_simulacao(app) == 0) {
                gtk_widget_queue_draw(app->area);
                atualizar_ui(app);
            }
        }
    }
}

//cb_importar abre o diálogo de escolha de arquivo TXT/PDF depois do o usuário escolher e confirmar, inicializa a simulação redesenha o canvas.
static void cb_importar(GtkWidget *btn, gpointer data) {
    (void)btn;
    AppState *app = (AppState*)data;

    int ret = input_show_file_chooser(GTK_WINDOW(app->janela), &app->config);
    if (ret != 0) return; //usuário cancelou ou erro 

    if (iniciar_simulacao(app) != 0) return;

    gtk_widget_queue_draw(app->area);
    atualizar_ui(app);
}

// cb_manual abre o formulário de entrada manual
static void cb_manual(GtkWidget *btn, gpointer data) {
    (void)btn;
    AppState *app = (AppState*)data;

    int ret = input_show_manual_dialog(GTK_WINDOW(app->janela), &app->config);
    if (ret != 0) return;

    if (iniciar_simulacao(app) != 0) return;

    gtk_widget_queue_draw(app->area);
    atualizar_ui(app);
}

//gui_run connstrói e abre a janela principal 
void gui_run(const char *config_file) {
    //aloca o estado na heap para não ter problemas de escopo 
    AppState *app = (AppState*)calloc(1, sizeof(AppState));
    app->config_carregada = 0;

    //Janela principal 
    app->janela = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->janela),
                         "Simulador SO Multitarefa — Gantt Interativo");
    gtk_window_set_default_size(GTK_WINDOW(app->janela), 820, 560);
    gtk_window_set_position(GTK_WINDOW(app->janela), GTK_WIN_POS_CENTER);
    g_signal_connect(app->janela, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    //container vertical principal 
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->janela), vbox);

    //barra superior p importar / manual 
    GtkWidget *barra_topo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(barra_topo), 8);
    gtk_box_pack_start(GTK_BOX(vbox), barra_topo, FALSE, FALSE, 0);

    GtkWidget *btn_importar = gtk_button_new_with_label(" Importar TXT");
    g_signal_connect(btn_importar, "clicked", G_CALLBACK(cb_importar), app);
    gtk_box_pack_start(GTK_BOX(barra_topo), btn_importar, FALSE, FALSE, 0);

    GtkWidget *btn_manual = gtk_button_new_with_label(" Entrada Manual");
    g_signal_connect(btn_manual, "clicked", G_CALLBACK(cb_manual), app);
    gtk_box_pack_start(GTK_BOX(barra_topo), btn_manual, FALSE, FALSE, 0);

    // Separador
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    //Área de desenho do Gantt 
    app->area = gtk_drawing_area_new();
    gtk_widget_set_size_request(app->area, 820, 440);
    g_signal_connect(app->area, "draw", G_CALLBACK(cb_desenhar), app);
    gtk_box_pack_start(GTK_BOX(vbox), app->area, TRUE, TRUE, 0);

    //Separador 
    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    //barra inferior: controles 
    GtkWidget *barra_bot = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(barra_bot), 8);
    gtk_box_pack_start(GTK_BOX(vbox), barra_bot, FALSE, FALSE, 0);

    app->btn_retroceder = gtk_button_new_with_label("< Retroceder");
    g_signal_connect(app->btn_retroceder, "clicked",
                     G_CALLBACK(cb_retroceder), app);
    gtk_box_pack_start(GTK_BOX(barra_bot), app->btn_retroceder, FALSE, FALSE, 0);

    app->btn_avancar = gtk_button_new_with_label("Avançar >");
    g_signal_connect(app->btn_avancar, "clicked",
                     G_CALLBACK(cb_avancar), app);
    gtk_box_pack_start(GTK_BOX(barra_bot), app->btn_avancar, FALSE, FALSE, 0);

    app->btn_tudo = gtk_button_new_with_label("-->> Executar tudo");
    g_signal_connect(app->btn_tudo, "clicked",
                     G_CALLBACK(cb_executar_tudo), app);
    gtk_box_pack_start(GTK_BOX(barra_bot), app->btn_tudo, FALSE, FALSE, 0);

    app->btn_salvar = gtk_button_new_with_label(" Salvar SVG");
    g_signal_connect(app->btn_salvar, "clicked",
                     G_CALLBACK(cb_salvar_svg), app);
    gtk_box_pack_start(GTK_BOX(barra_bot), app->btn_salvar, FALSE, FALSE, 0);

    // Separador visual entre controles e botões de ação 
    gtk_box_pack_start(GTK_BOX(barra_bot),
                       gtk_separator_new(GTK_ORIENTATION_VERTICAL),
                       FALSE, FALSE, 4);

    //botão modificar tarefa
    app->btn_modificar = gtk_button_new_with_label(" Modificar Tarefa");
    g_signal_connect(app->btn_modificar, "clicked",
                     G_CALLBACK(cb_modificar_tarefa), app);
    gtk_box_pack_start(GTK_BOX(barra_bot), app->btn_modificar, FALSE, FALSE, 0);

    //Botão: Nova Simulação 
    app->btn_nova_sim = gtk_button_new_with_label(" Nova Simulação");
    g_signal_connect(app->btn_nova_sim, "clicked",
                     G_CALLBACK(cb_nova_simulacao), app);
    gtk_box_pack_start(GTK_BOX(barra_bot), app->btn_nova_sim, FALSE, FALSE, 0);

    //label de status 
    app->lbl_status = gtk_label_new("Nenhuma configuração carregada");
    gtk_widget_set_halign(app->lbl_status, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(barra_bot), app->lbl_status, TRUE, TRUE, 0);

    atualizar_ui(app); // define sensibilidade inicial dos botões 
    gtk_widget_show_all(app->janela);

    //se um arquivo foi passado, carrega automaticamente 
    if (config_file) {
        if (config_load(&app->config, config_file) == 0) {
            iniciar_simulacao(app);
            gtk_widget_queue_draw(app->area);
            atualizar_ui(app);
        }
    }

    gtk_main();
    free(app);
}