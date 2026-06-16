/*
 * config_ext.c - Parser de configuração a partir de string em memória
 *
 * Estratégia:
 *   Escrevemos a string num arquivo temporário e chamamos config_load().
 *   Assim reutilizamos 100% do código de parse já testado, sem duplicar.
 *
 * */

#include <stdio.h>    /* FILE, fopen, fwrite, fclose, remove, tmpnam */
#include <string.h>   /* strlen */
#include "config_ext.h"
#include "config.h"

/*
 * config_load_from_string - Salva a string num arquivo temp e chama config_load
 *
 * Por que arquivo temporário?
 *   config_load() já faz todo o parse correto (com tratamento de erros,
 *   comentários, maiúsculas, etc.). Em vez de duplicar esse código,
 *   aproveitamos a função existente: escrevemos a string num arquivo
 *   temporário, chamamos config_load(), depois apagamos o arquivo.
 *
 * tmpnam() gera um nome de arquivo temporário único (garante que não
 * haverá conflito mesmo se o programa for chamado em paralelo).
 */
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
