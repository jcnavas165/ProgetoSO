/*
*projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 * modify_dialog.h - Diálogo para modificar tarefas durante a simulação
 *
 * Regras de negócio implementadas:
 *
 *   TAREFA NEW / FINISHED:
 *     → Não pode ser modificada. Mensagem explicativa.
 *
 *   TAREFA READY (na fila, aguardando CPU):
 *     → Pode modificar prioridade e tempo restante livremente.
 *     → Simulação recalcula a partir do tick atual com novos dados.
 *
 *   TAREFA RUNNING (executando em uma CPU agora):
 *     → Exige justificativa obrigatória do usuário.
 *     → Após confirmar, a tarefa é preemptada (vai para READY)
 *       e a simulação recalcula com os novos valores.
 *
 *   TAREFA SUSPENDED (bloqueada):
 *     → Pode modificar prioridade. Tempo restante não muda
 *       enquanto suspensa. Mensagem informativa.
 *
 * Autor: Projeto A - Simulador SO Multitarefa
 */

#ifndef MODIFY_DIALOG_H
#define MODIFY_DIALOG_H

#include <gtk/gtk.h>
#include "simulator.h"
#include "task.h"

/*
 * ModifyResult - Resultado de uma modificação de tarefa
 *
 * Retornado por modify_show_dialog para indicar o que aconteceu.
 */
typedef enum {
    MODIFY_CANCELADO   = 0,  /* Usuário cancelou ou fechou o diálogo     */
    MODIFY_APLICADO    = 1,  /* Modificação aplicada com sucesso          */
    MODIFY_NEGADO      = 2   /* Modificação negada pelas regras de negócio*/
} ModifyResult;

/*
 * modify_show_dialog - Abre o diálogo de modificação de uma tarefa
 *
 * Exibe um diálogo com os dados atuais da tarefa e permite editar
 * prioridade e tempo restante, respeitando as regras de negócio.
 *
 * Se a tarefa estiver RUNNING, exige justificativa antes de aplicar.
 * Se a modificação for aplicada, a simulação é "rebobinada" para o
 * tick atual com os novos valores (sim_apply_modification).
 *
 * Parâmetros:
 *   parent  - Janela pai do diálogo
 *   sim     - Estado atual da simulação
 *   task_id - ID da tarefa a modificar
 *
 * Retorno:
 *   ModifyResult indicando o que aconteceu
 */
ModifyResult modify_show_dialog(GtkWindow *parent, SimState *sim, int task_id);

/*
 * modify_show_task_list - Abre janela de lista de tarefas para escolher qual modificar
 *
 * Exibe todas as tarefas com seu estado atual e permite o usuário
 * clicar em uma para abrir o diálogo de modificação.
 *
 * Parâmetros:
 *   parent - Janela pai
 *   sim    - Estado atual da simulação
 */
void modify_show_task_list(GtkWindow *parent, SimState *sim);

/*
 * sim_apply_modification - Aplica modificação e recalcula a partir do tick atual
 *
 * Chamada internamente após o usuário confirmar a modificação.
 * Modifica o TCB e trunca o histórico para que a simulação
 * continue a partir do tick atual com os novos valores.
 *
 * Parâmetros:
 *   sim        - Estado da simulação
 *   task_id    - ID da tarefa
 *   nova_prior - Nova prioridade (-1 = não muda)
 *   novo_rem   - Novo tempo restante (-1 = não muda)
 */
void sim_apply_modification(SimState *sim, int task_id, int nova_prior, int novo_rem);

#endif /* MODIFY_DIALOG_H */
