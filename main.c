/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 *
 * main.c - Ponto de entrada do Simulador de SO Multitarefa
 *
 * Modos de execução:
 *   ./simulador                    → abre janela GTK (sem config = pede para importar)
 *   ./simulador config.txt         → abre janela GTK com arquivo carregado
 *   ./simulador --sem-gui config.txt  → só terminal
 *   ./simulador --gerar-config     → cria arquivo de exemplo
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gtk/gtk.h> 

#include "task.h"       // Estrutura de tarefas
#include "config.h"     // Leitura de configuração
#include "scheduler.h"  // Escalonadores
#include "simulator.h"  // Simulação
#include "gantt.h"      // Gráfico de Gantt
#include "gui_gantt.h"  // Interface gráfica

//funçao para apresentaçao
static void print_banner(void) {
    printf("║  Simulador de SO Multitarefa de Tempo Compartilhado  ║\n");
    printf("║  Projeto A - Julio Navas & Nathalya Chaves  ║\n");
    
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));

    /* gtk_init DEVE ser chamado antes de qualquer widget GTK */
    gtk_init(&argc, &argv);

    print_banner();

    /* Modo especial: só gera o arquivo de exemplo */
    if (argc >= 2 && strcmp(argv[1], "--gerar-config") == 0) {
        config_create_default("config_exemplo.txt");
        printf("[INFO] Execute: %s config_exemplo.txt\n", argv[0]);
        return 0;
    }

    /* Modo terminal (sem GUI) */
   if (argc >= 3 && strcmp(argv[1], "--sem-gui") == 0) {
        SimConfig config;                    // Estrutura da configuração
        config_load(&config, argv[2]);       // Carrega arquivo
        config_print(&config);               // Mostra configuração
        SimState sim;                        // Estado da simulação
        sim_init(&sim, &config);             // Inicializa simulação
        sim_run_full(&sim);                  // Executa tudo automático
        gantt_print_terminal(&sim);          // Mostra Gantt no terminal
        sim_print_metrics(&sim);             // Mostra métricas finais
        gantt_generate_svg(&sim, "gantt.svg"); // Gera arquivo SVG
        if (sim.history) free(sim.history);  // Libera memória do histórico
        return 0;
    }

    /*
     * MODO GTK (padrão):
     * gui_run cuida de tudo:
     *   - Se um arquivo foi passado como argumento, carrega automaticamente
     *   - Se não, exibe a tela de boas-vindas com botões "Importar" e "Manual"
     */
    const char *config_inicial = (argc >= 2) ? argv[1] : NULL;
    gui_run(config_inicial);

    return 0;
}
