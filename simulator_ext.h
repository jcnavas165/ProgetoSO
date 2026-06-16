/*
 * simulator_ext.h - Extensões do simulador para ações de mutex e I/O
 *
 * Funções para:
 *   - Inicializar mutexes
 *   - Executar ações de mutex e I/O durante a simulação
 *   - Gerenciar tarefas suspensas
 */

#ifndef SIMULATOR_EXT_H
#define SIMULATOR_EXT_H

#include "simulator.h"
#include "actions.h"

/*
 * sim_init_mutexes - Inicializa o array de mutexes
 */
void sim_init_mutexes(SimState *sim);

/*
 * sim_execute_actions - Executa ações pendentes de uma tarefa em um tick
 *
 * Parâmetros:
 *   sim - Estado da simulação
 *   task - Tarefa cujas ações devem ser executadas
 *   relative_time - Tempo relativo ao início da tarefa
 */
void sim_execute_actions(SimState *sim, TCB *task, int relative_time);

/*
 * sim_handle_mutex_lock - Tenta adquirir um mutex
 *
 * Se conseguir, continua. Senão, suspende a tarefa.
 *
 * Retorno: 1 se conseguiu, 0 se ficou suspensa
 */
int sim_handle_mutex_lock(SimState *sim, TCB *task, int mutex_id);

/*
 * sim_handle_mutex_unlock - Libera um mutex
 *
 * Se houver tarefas aguardando, acorda a primeira.
 */
void sim_handle_mutex_unlock(SimState *sim, TCB *task, int mutex_id);

/*
 * sim_handle_io - Inicia uma operação de E/S
 *
 * Suspende a tarefa até que a E/S termine (io_end_tick)
 */
void sim_handle_io(SimState *sim, TCB *task, int duration);

/*
 * sim_check_suspended_tasks - Verifica se tarefas podem ser acordadas
 *
 * Chamada a cada tick para verificar:
 *   - Se E/S acabou → resume a tarefa
 *   - Se mutex foi liberado → resume a próxima na fila
 */
void sim_check_suspended_tasks(SimState *sim);

/*
 * sim_update_dynamic_priority - Atualiza prioridades dinâmicas (para PRIOPEnv)
 */
void sim_update_dynamic_priority(SimState *sim);

#endif /* SIMULATOR_EXT_H */
