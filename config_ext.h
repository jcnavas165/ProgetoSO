

#ifndef CONFIG_EXT_H
#define CONFIG_EXT_H

#include "config.h"

//config_load_from_string - Faz parse de uma string no formato config
//Mesmo formato do arquivo .txt
//Parâmetros: cfg = Ponteiro para SimConfig a preencher, content = String com o conteúdo completo (pode ter \n)
int config_load_from_string(SimConfig *cfg, const char *content);

#endif // CONFIG_EXT_H
