#ifndef MODIFY_DIALOG_H
#define MODIFY_DIALOG_H

#include <gtk/gtk.h>
#include "simulator.h"
#include "task.h"


typedef enum {
    MODIFY_CANCELADO   = 0,  /* Usuário cancelou ou fechou o diálogo     */
    MODIFY_APLICADO    = 1,  /* Modificação aplicada com sucesso          */
    MODIFY_NEGADO      = 2   /* Modificação negada pelas regras de negócio*/
} ModifyResult;


ModifyResult modify_show_dialog(GtkWindow *parent, SimState *sim, int task_id);


void modify_show_task_list(GtkWindow *parent, SimState *sim);


void sim_apply_modification(SimState *sim, int task_id, int nova_prior, int novo_rem);

#endif /* MODIFY_DIALOG_H */
