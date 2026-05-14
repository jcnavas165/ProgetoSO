#include <stdio.h>    /* FILE, fopen, fwrite, fclose, remove, tmpnam */
#include <string.h>   /* strlen */
#include "config_ext.h"
#include "config.h"


int config_load_from_string(SimConfig *cfg, const char *content) {
    /* Gera um nome de arquivo temporário único */
    char tmpfile[L_tmpnam];
    tmpnam(tmpfile);  /* L_tmpnam é o tamanho mínimo garantido para o nome */

    /* Abre o arquivo temporário para escrita */
    FILE *f = fopen(tmpfile, "w");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível criar arquivo temporário\n");
        return -1;
    }

    /* Escreve o conteúdo da string no arquivo */
    fwrite(content, 1, strlen(content), f);
    fclose(f);

    /* Chama o parser original — ele fará todo o trabalho */
    int resultado = config_load(cfg, tmpfile);

    /* Remove o arquivo temporário (limpeza) */
    remove(tmpfile);

    return resultado;
}
