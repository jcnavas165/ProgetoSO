/*
 *projeto A SO
 *Autores: Julio Cesar Navas e Nathálya Chaves
 */

#ifndef INPUT_DIALOG_H
#define INPUT_DIALOG_H

#include <gtk/gtk.h>
#include "config.h"


int input_show_file_chooser(GtkWindow *parent, SimConfig *cfg);


int input_show_manual_dialog(GtkWindow *parent, SimConfig *cfg);


int input_read_txt(const char *filepath, SimConfig *cfg);


int input_read_pdf(const char *filepath, SimConfig *cfg);

#endif /* INPUT_DIALOG_H */
