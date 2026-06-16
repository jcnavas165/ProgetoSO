# RESUMO DAS IMPLEMENTAÇÕES

## Requisito 1: Escalonador PRIOPEnv com Envelhecimento

### Arquivos modificados:

- **config.h**: Adicionado `ALGO_PRIOPENV` enum
- **config.h**: Adicionado campo `alpha` em `SimConfig`
- **config.c**:
  - Atualizado `algo_name_to_enum()` para reconhecer "priopenv"
  - Atualizado `algo_enum_to_name()` para exibir PRIOPEnv
  - Atualizado `parse_header_line()` para parse do 4º campo (alpha)
- **scheduler.h**: Declarada função `sched_priopenv()`
- **scheduler.c**:
  - Implementada `sched_priopenv()` com fórmula de envelhecimento
  - Atualizado `scheduler_get()` para suportar PRIOPEnv
- **task.h**: Adicionado campo `dynamic_priority` ao TCB

### Formato de configuração:

```
PRIOPEnv;quantum;num_cpus;alpha
```

Exemplo: `PRIOPEnv;5;2;1`

- Quantum: 5
- Processadores: 2
- Alpha para envelhecimento: 1

### Fórmula de envelhecimento:

```
dynamic_priority = static_priority + (current_tick - arrival) / alpha
```

## Requisito 2: Operações de Mutex

### Arquivos criados:

- **actions.h**: Definições de `TaskAction`, `Mutex`, tipos de ação
- **actions.c**: Implementação de gerenciamento de mutex

### Estruturas:

- `TaskAction`: Representa uma ação (mutex lock/unlock ou I/O)
- `Mutex`: Representa um mutex com fila de espera
- `ACTION_MUTEX_LOCK` (ML): Solicitação de mutex
- `ACTION_MUTEX_UNLOCK` (MU): Liberação de mutex

### Formato no arquivo de configuração:

```
MLxx:tt  - Lock do mutex xx no tempo tt (relativo à tarefa)
MUxx:tt  - Unlock do mutex xx no tempo tt
```

Exemplo: `ML0:3;MU0:5`

- Solicita mutex 0 no instante 3 (relativo)
- Libera mutex 0 no instante 5 (relativo)

### Estados de suspensão:

- Tarefa suspensa aguardando mutex: `SUSPEND_REASON_MUTEX`
- Fila de espera gerenciada em `Mutex.waiting_tasks[]`
- Acordar próxima tarefa ao liberar mutex

## Requisito 3: Operações de E/S

### Estruturas:

- `ACTION_IO`: Operação de entrada/saída
- `io_end_tick`: Tick em que a E/S termina
- `SUSPEND_REASON_IO`: Motivo da suspensão

### Formato no arquivo de configuração:

```
IO:tt-dd - E/S no tempo tt com duração dd (relativo à tarefa)
```

Exemplo: `IO:2-3`

- Inicia E/S no instante 2 (relativo)
- Duração: 3 ticks
- Tarefa acordada no instante 2 + 3 = 5

### IRQ (Interrupt Request):

- Quando E/S termina, gera IRQ que acorda a tarefa
- Implementado em `sim_check_suspended_tasks()`

## Requisito 4: Extensões nas Estruturas

### Arquivo task.h (TCB expandido):

```c
TaskAction actions[MAX_ACTIONS_PER_TASK];  // Array de ações
int action_count;                           // Quantidade de ações
int next_action_idx;                        // Próxima ação a executar
int dynamic_priority;                       // Para envelhecimento
SuspendReason suspend_reason;              // Motivo da suspensão
int suspended_mutex_id;                    // Qual mutex suspendeu
int io_end_tick;                           // Quando I/O termina
int suspended_tick;                        // Quando foi suspensa
```

### EventType expandido:

- `EVENT_MUTEX_LOCK`: Tentativa de lock
- `EVENT_MUTEX_UNLOCK`: Unlock de mutex
- `EVENT_IO_START`: Início de E/S
- `EVENT_IO_END`: Fim de E/S

## Requisito 5: Integração no Simulador

### Arquivos criados:

- **simulator_ext.h**: Interface para extensões
- **simulator_ext.c**: Implementação das extensões

### Funções principais:

- `sim_init_mutexes()`: Inicializa array de mutexes
- `sim_execute_actions()`: Executa ações de uma tarefa
- `sim_handle_mutex_lock()`: Adquire mutex ou suspende
- `sim_handle_mutex_unlock()`: Libera mutex
- `sim_handle_io()`: Inicia operação de E/S
- `sim_check_suspended_tasks()`: Acorda tarefas com I/O pronto
- `sim_update_dynamic_priority()`: Calcula envelhecimento (PRIOPEnv)

### Modificações em simulator.c:

- `sim_init()`: Chama `sim_init_mutexes()`
- `sim_step()`:
  - Chama `sim_update_dynamic_priority()` se PRIOPEnv
  - Adiciona `alpha` ao `SchedContext`
  - Chama `sim_check_suspended_tasks()` após execução

### Modificações em config.c:

- `parse_task_line()` estendida para parse de ações
- Suporte a múltiplas ações por tarefa
- Ações executadas na ordem especificada

## Ordem de Execução das Ações

Conforme requisito 2.5 e 2.6:

1. Se ações no instante zero (ou início da tarefa):
   - Primeira: reinicia/inicia a execução da tarefa
   - Depois: executa ações na ordem do arquivo
2. Se múltiplas ações no mesmo instante:
   - Executadas na ordem do arquivo de configuração

## Desempates (Requisito 1.3)

Ordem de critérios para PRIOPEnv:

1. **Maior prioridade dinâmica** (com envelhecimento)
2. Tarefa que está executando (evitar troca de contexto)
3. Maior prioridade estática
4. Menor instante de chegada
5. Menor duração
6. Sorteio

## Arquivos Compiláveis

Atualizações necessárias:

- Adicionar `actions.c` e `simulator_ext.c` ao Makefile
- Adicionar dependências de headers

## Notas de Implementação

1. **Alpha configurável**: Pode ser 0 (desabilita envelhecimento) ou qualquer valor positivo
2. **Mutexes**: Máximo 32 (MAX_MUTEXES), numerados 0-31
3. **Ações**: Máximo 32 por tarefa (MAX_ACTIONS_PER_TASK)
4. **Tarefas**: Podem ter múltiplas ações de mutex e I/O
5. **Ordem determinística**: Ações executadas na ordem do arquivo

## Testes Recomendados

Arquivo `PriorpEnv_test.txt`:

```
PRIOPEnv;5;2;1
1;FF4444;0;10;3
2;44FF44;2;6;5;ML0:3;MU0:5
3;4444FF;4;8;1;IO:2-3
4;FFAA00;6;4;4
```

Neste teste:

- Tarefa 2 solicita mutex 0 em t=5 (global), libera em t=7 (global)
- Tarefa 3 tem E/S de duração 3 começando em t=6 (global), termina em t=9 (global)
