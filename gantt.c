/* Projeto A SO
 * Autores: Julio Cesar Navas e Nathálya Chaves 
 * gantt.c - Implementação do gerador do gráfico de Gantt em SVG
 */

#include <stdio.h>
#include <string.h>
#include "gantt.h"

//funções auxis internas 
//get_task_state_at_tick - Retorna o estado de uma tarefa em um tick específico
//Percorre o histórico da tarefa para encontrar qual era seu estado no tick dado. Usa o último evento cujo tick <= tick_query.
// Retorno:estado da tarefa (-1 se ainda não havia chegado), cpu_out: CPU onde executava (-1 se não estava rodando), event_out: tipo do evento neste tick
static int get_task_info_at_tick(const TCB *t, int tick_query,
                                  int *cpu_out, EventType *event_out) {
    *cpu_out   = NO_TASK;
    *event_out = EVENT_ARRIVAL; // default

    //Tarefa ainda não chegou 
    if (tick_query < t->arrival) {
        return -1; //não existe
    }

    //percorre o histórico em ordem, aplicando o estado mais recente
    int last_state = TASK_NEW;
    int last_cpu   = NO_TASK;
    EventType last_event = EVENT_ARRIVAL;

    for (int i = 0; i < t->history_count; i++) {
        const HistoryEntry *e = &t->history[i];
        if (e->tick <= tick_query) {
            last_state = e->state;
            last_cpu   = e->cpu_id;
            last_event = e->event;
        } else {
            break; //histórico está em ordem crescente de tick 
        }
    }

    *cpu_out   = last_cpu;
    *event_out = last_event;
    return last_state;
}

// hex_color_to_svg - Garante que a cor está no formato correto para SVG
static void hex_color_to_svg(const char *color, char *out) {
    if (color[0] == '#') {
        snprintf(out, 8, "%s", color);
    } else {
        snprintf(out, 8, "#%s", color);
    }
}

//SVG: funços de escrita 
//svg_rect - Escreve um retângulo SVG no arquivo
static void svg_rect(FILE *f, int x, int y, int w, int h,
                      const char *fill, const char *stroke, float opacity) {
    fprintf(f, "  <rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" "
               "fill=\"%s\" stroke=\"%s\" stroke-width=\"1\" opacity=\"%.2f\"/>\n",
            x, y, w, h, fill, stroke, opacity);
}

// svg_text - escreve texto SVG no arquivo
static void svg_text(FILE *f, int x, int y, const char *text,
                      int font_size, const char *anchor, const char *color) {
    fprintf(f, "  <text x=\"%d\" y=\"%d\" font-size=\"%d\" "
               "text-anchor=\"%s\" fill=\"%s\" font-family=\"monospace\">%s</text>\n",
            x, y, font_size, anchor, color, text);
}

//svg_line - Escreve uma linha SVG
static void svg_line(FILE *f, int x1, int y1, int x2, int y2, const char *color) {
    fprintf(f, "  <line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" "
               "stroke=\"%s\" stroke-width=\"1\"/>\n",
            x1, y1, x2, y2, color);
}

/* implementação principal 
gantt_generate_svg - Gera o arquivo SVG
Decisão de ordenação no eixo Y "A tarefa com ID menor é a mais próxima do eixo X"
desenhamos de cima para baixo, a última tarefa (ID maior) é a mais próxima do topo do SVG, e a primeira (ID menor) fica na parte mais baixa antes do eixo X.*/
int gantt_generate_svg(const SimState *sim, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível criar o arquivo SVG: %s\n", filename);
        return -1;
    }

    int num_ticks = sim->current_tick;
    int num_tasks = sim->num_tasks;

    // Calcula dimensões do SVG: largura = margem_esquerda + (ticks * largura_célula) + margem_direita; Altura  = margem_topo + (tarefas * altura_célula) + margem_base
    int svg_w = GANTT_MARGIN_L + (num_ticks * GANTT_CELL_W) + GANTT_MARGIN_R;
    int svg_h = GANTT_MARGIN_T + (num_tasks * GANTT_CELL_H) + GANTT_MARGIN_B;

    //garante um mínimo aceitavel
    if (svg_w < 400) svg_w = 400;
    if (svg_h < 300) svg_h = 300;

    //Cabeçalho SVG 
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" "
               "width=\"%d\" height=\"%d\" "
               "style=\"background-color:#F8F8F8;\">\n", svg_w, svg_h);

    //titulo
    char title[128];
    snprintf(title, sizeof(title), "Gantt - %s | Quantum=%d | CPUs=%d",
             algo_enum_to_name(sim->config->algo),
             sim->config->quantum,
             sim->config->num_cpus);
    svg_text(f, svg_w / 2, 25, title, 13, "middle", "#333333");

    //undo branco p/ o grafico
    svg_rect(f, GANTT_MARGIN_L, GANTT_MARGIN_T,
             num_ticks * GANTT_CELL_W, num_tasks * GANTT_CELL_H,
             "white", "#CCCCCC", 1.0f);

    //linhas de grade horizontais
    for (int i = 0; i <= num_tasks; i++) {
        int y = GANTT_MARGIN_T + i * GANTT_CELL_H;
        svg_line(f, GANTT_MARGIN_L, y,
                 GANTT_MARGIN_L + num_ticks * GANTT_CELL_W, y, "#DDDDDD");
    }

    //linhas de grade verticais a cada tick
    for (int t = 0; t <= num_ticks; t++) {
        int x = GANTT_MARGIN_L + t * GANTT_CELL_W;
        svg_line(f, x, GANTT_MARGIN_T,
                 x, GANTT_MARGIN_T + num_tasks * GANTT_CELL_H, "#EEEEEE");
    }

    //Desenha as tarefas 
    //ordem no eixo Y: ID menor = mais próximo do eixo X = linha MAIS BAIXA no SVG, desenhamos em ordem DECRESCENTE de índice.
    // Para a tarefa com índice i (0-based, ordenado por ID): row = num_tasks - 1 - i  (i=0 → row mais baixa; i=n-1 → row mais alta)
    // (assumimos que tasks[] já está em ordem de chegada/ID, mas ordenamos)
    int order[MAX_TASKS];
    for (int i = 0; i < num_tasks; i++) order[i] = i;
    // Ordenação por bolha simples por ID (num_tasks é pequeno) 
    for (int i = 0; i < num_tasks - 1; i++) {
        for (int j = 0; j < num_tasks - 1 - i; j++) {
            if (sim->tasks[order[j]].id > sim->tasks[order[j+1]].id) {
                int tmp = order[j]; order[j] = order[j+1]; order[j+1] = tmp;
            }
        }
    }

    for (int oi = 0; oi < num_tasks; oi++) {
        int idx = order[oi]; //undice real no array de task
        const TCB *t = &sim->tasks[idx];

        //row: posição da tarefa no eixo Y
        //oi=0 (ID menor) → row = num_tasks-1 (linha mais baixa)
        //oi=n-1 (ID maior) → row = 0 (linha mais alta)
        int row = num_tasks - 1 - oi;
        int y   = GANTT_MARGIN_T + row * GANTT_CELL_H;

        char svg_color[8];
        hex_color_to_svg(t->color, svg_color);

        //label da tarefa no eixo Y
        char label[16];
        snprintf(label, sizeof(label), "T%d", t->id);
        svg_text(f, GANTT_MARGIN_L - 5, y + GANTT_CELL_H / 2 + 5,
                 label, 12, "end", "#333333");

        //dsenha um bloco para cada tick
        for (int tick = 0; tick < num_ticks; tick++) {
            int x = GANTT_MARGIN_L + tick * GANTT_CELL_W;
            int cpu_id;
            EventType evt;
            int state = get_task_info_at_tick(t, tick, &cpu_id, &evt);

            if (state < 0) {
                //Tarefa ainda não chegou: fundo cinza claro 
                svg_rect(f, x, y, GANTT_CELL_W, GANTT_CELL_H,
                         "#F0F0F0", "#EEEEEE", 1.0f);
                continue;
            }

            switch (state) {
                case TASK_RUNNING:
                    //Executando: usa a cor da tarefa
                    svg_rect(f, x, y, GANTT_CELL_W, GANTT_CELL_H,
                             svg_color, "#888888", 0.9f);
                    //label da CPU dentro do bloco
                    if (cpu_id != NO_TASK) {
                        char cpu_label[8];
                        snprintf(cpu_label, sizeof(cpu_label), "C%d", cpu_id);
                        svg_text(f, x + GANTT_CELL_W / 2, y + GANTT_CELL_H / 2 + 4,
                                 cpu_label, 9, "middle", "white");
                    }
                    break;

                case TASK_READY:
                    //Pronta (aguardando CPU): branco com borda 
                    svg_rect(f, x, y, GANTT_CELL_W, GANTT_CELL_H,
                             "white", "#AAAAAA", 1.0f);
                    break;

                case TASK_SUSPENDED:
                    //Suspensa: preto 
                    svg_rect(f, x, y, GANTT_CELL_W, GANTT_CELL_H,
                             "#222222", "#000000", 1.0f);
                    break;

                case TASK_FINISHED:
                    // Já terminou: cinza muito claro 
                    svg_rect(f, x, y, GANTT_CELL_W, GANTT_CELL_H,
                             "#E8E8E8", "#DDDDDD", 1.0f);
                    break;

                default:
                    //NEW ou outro: sem bloco especial 
                    break;
            }

            //Ícones de eventos especiais 
            if (state != -1) {
                //Ícone de chegada: triângulo verde
                if (evt == EVENT_ARRIVAL) {
                    svg_text(f, x + 2, y + GANTT_CELL_H - 4,
                             "&#9654;", 10, "start", "#00AA00");
                }
                //icone de término: quadrado vermelho
                if (evt == EVENT_FINISH) {
                    svg_text(f, x + 2, y + GANTT_CELL_H - 4,
                             "&#9632;", 10, "start", "#CC0000");
                }
                //Ícone de sorteio: estrela
                if (evt == EVENT_LOTTERY) {
                    svg_text(f, x + GANTT_CELL_W / 2, y + 12,
                             "&#9733;", 10, "middle", "#FF6600");
                }
            }
        }

        //Ícones de chegada e término nas bordas
        //linha de chegada: linha vertical verde no tick de chegada
        if (t->arrival < num_ticks) {
            int xa = GANTT_MARGIN_L + t->arrival * GANTT_CELL_W;
            svg_line(f, xa, y, xa, y + GANTT_CELL_H, "#00CC00");
        }
        //Linha de término: linha vertical vermelha no tick de final
        if (t->finish_tick >= 0 && t->finish_tick <= num_ticks) {
            int xf = GANTT_MARGIN_L + t->finish_tick * GANTT_CELL_W;
            svg_line(f, xf, y, xf, y + GANTT_CELL_H, "#CC0000");
        }
    }

    //Eixo X: numeração dos ticks 
    int axis_y = GANTT_MARGIN_T + num_tasks * GANTT_CELL_H + 15;

    //linha do eixo 
    svg_line(f, GANTT_MARGIN_L, GANTT_MARGIN_T + num_tasks * GANTT_CELL_H,
             GANTT_MARGIN_L + num_ticks * GANTT_CELL_W,
             GANTT_MARGIN_T + num_tasks * GANTT_CELL_H, "#666666");

    //marcações de tempo a cada 1 tick, ou a cada 2 se houver muitos ticks 
    int tick_step = (num_ticks > 30) ? 2 : 1;
    for (int t = 0; t <= num_ticks; t += tick_step) {
        int x = GANTT_MARGIN_L + t * GANTT_CELL_W;
        char label[8];
        snprintf(label, sizeof(label), "%d", t);

        //Marca vertical pequena
        svg_line(f, x, GANTT_MARGIN_T + num_tasks * GANTT_CELL_H,
                 x, GANTT_MARGIN_T + num_tasks * GANTT_CELL_H + 5, "#666666");

        svg_text(f, x, axis_y, label, 10, "middle", "#555555");
    }

    //Label do eixo X 
    svg_text(f, GANTT_MARGIN_L + (num_ticks * GANTT_CELL_W) / 2,
             axis_y + 18, "Tempo (ticks)", 11, "middle", "#555555");

    //Legenda
    int leg_x = GANTT_MARGIN_L;
    int leg_y = axis_y + 35;

    svg_text(f, leg_x, leg_y, "Legenda:", 11, "start", "#333333");

    //Executando
    svg_rect(f, leg_x + 60, leg_y - 10, 18, 12, "#FF8800", "#888888", 0.9f);
    svg_text(f, leg_x + 82, leg_y, "Executando", 10, "start", "#555555");

    //pronta
    svg_rect(f, leg_x + 155, leg_y - 10, 18, 12, "white", "#AAAAAA", 1.0f);
    svg_text(f, leg_x + 177, leg_y, "Pronta", 10, "start", "#555555");

    //supensa
    svg_rect(f, leg_x + 225, leg_y - 10, 18, 12, "#222222", "#000000", 1.0f);
    svg_text(f, leg_x + 247, leg_y, "Suspensa", 10, "start", "#555555");

    //icone de chegada
    svg_text(f, leg_x + 315, leg_y, "&#9654; Chegada", 10, "start", "#00AA00");

    //ícone de término
    svg_text(f, leg_x + 400, leg_y, "&#9632; Término", 10, "start", "#CC0000");

    //ícone de sorteio
    svg_text(f, leg_x + 490, leg_y, "&#9733; Sorteio", 10, "start", "#FF6600");

    //fecha o SVG 
    fprintf(f, "</svg>\n");
    fclose(f);

    printf("[INFO] Gráfico de Gantt gerado: %s\n", filename);
    printf("[INFO] Abra o arquivo em qualquer navegador web para visualizar.\n");

    return 0;
}

//gantt_print_terminal - Versão textual do Gantt para o terminal
//Caracteres usados:[#] = executando  (bloco cheio), [.] = pronto/aguardando, [X] = suspensa, [ ] = fora do sistema (antes de chegar ou após terminar)
// Cada coluna representa um tick. As tarefas aparecem com ID menor embaixo e ID maior em cima (como no gráfico SVG).
void gantt_print_terminal(const SimState *sim) {
    int num_ticks = sim->current_tick;
    int num_tasks = sim->num_tasks;

    //ordena por ID
    int order[MAX_TASKS];
    for (int i = 0; i < num_tasks; i++) order[i] = i;
    for (int i = 0; i < num_tasks - 1; i++) {
        for (int j = 0; j < num_tasks - 1 - i; j++) {
            if (sim->tasks[order[j]].id > sim->tasks[order[j+1]].id) {
                int tmp = order[j]; order[j] = order[j+1]; order[j+1] = tmp;
            }
        }
    }

    printf("\n═══ GRÁFICO DE GANTT (TERMINAL) ═══\n");
    printf("     │");
    for (int t = 0; t < num_ticks; t++) printf("%-2d│", t);
    printf("\n");

    //imprime de cima (ID maior) para baixo (ID menor)
    for (int oi = num_tasks - 1; oi >= 0; oi--) {
        int idx = order[oi];
        const TCB *t = &sim->tasks[idx];

        printf("T%-3d │", t->id);

        for (int tick = 0; tick < num_ticks; tick++) {
            int cpu_id;
            EventType evt;
            int state = get_task_info_at_tick(t, tick, &cpu_id, &evt);

            if (state < 0)              printf("  │");
            else if (state == TASK_RUNNING)   printf("##│");
            else if (state == TASK_READY)     printf("..│");
            else if (state == TASK_SUSPENDED) printf("XX│");
            else if (state == TASK_FINISHED)  printf("  │");
            else                              printf("  │");
        }
        printf("\n");
    }

    printf("     │");
    for (int t = 0; t < num_ticks; t++) printf("──│");
    printf("\n");
    printf("     └");
    for (int t = 0; t < num_ticks; t++) printf("───");
    printf(" Tempo\n");

    printf("\nLegenda: ## = executando  .. = pronta  XX = suspensa\n");
}
