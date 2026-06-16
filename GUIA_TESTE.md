# GUIA DE TESTE E VALIDAÇÃO

## 🚀 Compilação

```bash
cd 'c:\Users\Julio\Desktop\novo soooo\ProgetoSO'
make clean
make
```

## 🧪 Testes Recomendados

### Teste 1: PRIOPEnv Simples (sem mutex/IO)

**Arquivo:** `PRIOP.txt` (modificado)

```
PRIOPEnv;4;2;1
1;FF4444;0;10;3
2;44FF44;2;6;5
3;4444FF;4;8;1
4;FFAA00;6;4;4
```

**Esperado:**

- Tarefa 1 (prioridade 3) executa primeiro
- Prioridades aumentam com envelhecimento
- Após alguns ticks, tarefas de baixa prioridade ganham prioridade dinâmica

### Teste 2: Mutex Básico

**Arquivo:** `TEST_MUTEX.txt`

```
PRIOP;5;2;0
1;FF4444;0;10;3
2;44FF44;2;6;5;ML0:3;MU0:5
```

**Esperado:**

- T2 solicita mutex 0 em t=5 (global: 2+3)
- Se T1 tiver mutex, T2 fica suspensa
- T2 liberada quando T1 libera o mutex
- Status: SUSPENDED com suspend_reason = MUTEX

### Teste 3: E/S Básica

**Arquivo:** `TEST_IO.txt`

```
PRIOP;5;2;0
1;FF4444;0;10;3
3;4444FF;4;8;1;IO:2-3
```

**Esperado:**

- T3 inicia em t=6 (4+2 global)
- E/S começa, dura 3 ticks
- T3 retorna em t=9
- Status: SUSPENDED com suspend_reason = IO

### Teste 4: Combinado (Recomendado)

**Arquivo:** `PriorpEnv_test.txt` (já fornecido)

```
PRIOPEnv;5;2;1
1;FF4444;0;10;3
2;44FF44;2;6;5;ML0:3;MU0:5
3;4444FF;4;8;1;IO:2-3
4;FFAA00;6;4;4
```

**Esperado:**

- ✅ PRIOPEnv ativo com alpha=1
- ✅ T2 suspensa por mutex
- ✅ T3 suspensa por I/O
- ✅ Envelhecimento aumenta prioridades
- ✅ Múltiplas tarefas concorrentes

## 🔍 O Que Procurar

### Logs do Simulador

```
[Tick   0] Tarefa 1 chegou no sistema
[Tick   2] Tarefa 2 chegou no sistema
[Tick   5] Tarefa 2 → CPU 0 (remaining=6)
[Tick   5] Tarefa 2 solicita MUTEX 0
         → Tarefa 2 SUSPENSA aguardando MUTEX 0
[Tick   7] Tarefa 2 libera MUTEX 0
         → Tarefa 2 acordada (conseguiu MUTEX 0)
[Tick   6] Tarefa 3 inicia E/S com duração 3
         → Tarefa 3 SUSPENSA em E/S (será acordada em tick 9)
[Tick   9] Tarefa 3 acordada (E/S terminou)
```

### Métrica de Tempo de Espera

- Mutex: quanto tempo a tarefa ficou suspensa
- E/S: sempre duração especificada
- Ambos devem ser mostrados no relatório final

## 📊 Verificações Importantes

### ✅ Estruturas de Dados

- [ ] `TaskAction` criado com tipo, tempo, mutex_id, io_duration
- [ ] `Mutex` com owner_task_id e waiting_tasks[]
- [ ] `TCB` com dynamic_priority e suspend_reason

### ✅ PRIOPEnv

- [ ] Alpha lido do arquivo (campo 4)
- [ ] Dynamic priority = static + (tick - arrival) / alpha
- [ ] Prioridades aumentam com o tempo (envelhecimento)
- [ ] Desempate funciona corretamente

### ✅ Mutex

- [ ] MLxx:tt reconhecido
- [ ] MUxx:tt reconhecido
- [ ] Tarefa suspensa se mutex ocupado
- [ ] FIFO: primeira na fila acordada primeiro
- [ ] EVENT_MUTEX_LOCK registrado
- [ ] EVENT_MUTEX_UNLOCK registrado

### ✅ E/S

- [ ] IO:tt-dd reconhecido
- [ ] Duração validada (>= 1)
- [ ] Tarefa suspensa com io_end_tick correto
- [ ] IRQ no tick correto
- [ ] EVENT_IO_START e EVENT_IO_END registrados

### ✅ Simulador

- [ ] `sim_init_mutexes()` chamado
- [ ] `sim_update_dynamic_priority()` chamado para PRIOPEnv
- [ ] `sim_execute_actions()` chamado
- [ ] `sim_check_suspended_tasks()` chamado
- [ ] Alpha passado no SchedContext

## 🐛 Debug

### Modo Verbose

Se houver opção de debug, verificar:

```
[DEBUG] Parsing ação: ML0:3
[DEBUG] Tarefa 2: ação 0 = MUTEX_LOCK no tempo 3
[DEBUG] Tick 5: executando ações de T2
[DEBUG] T2 solicita MUTEX 0
```

### Breakpoints Sugeridos

1. `sim_init()` - Verificar se mutexes inicializados
2. `parse_action()` - Validar parsing correto
3. `sim_execute_actions()` - Ver ações sendo executadas
4. `sim_handle_mutex_lock()` - Verificar lógica de lock
5. `sim_check_suspended_tasks()` - Ver wake-ups

## 📈 Métricas Esperadas

Para `PriorpEnv_test.txt`:

| Tarefa | Turnaround | Wait Time | Events                                             |
| ------ | ---------- | --------- | -------------------------------------------------- |
| 1      | ~10        | 0         | ARRIVAL, START, FINISH                             |
| 2      | ~11        | 3-5       | ARRIVAL, START, **SUSPEND(MUTEX)**, RESUME, FINISH |
| 3      | ~13        | 3-4       | ARRIVAL, START, **SUSPEND(IO)**, RESUME, FINISH    |
| 4      | ~14        | 8-10      | ARRIVAL, READY, START, FINISH                      |

## 🎯 Casos de Teste Manuais

### Caso 1: Mutex Simples

```
T1 começa com MUTEX 0
T2 pede MUTEX 0 → SUSPENDE
T1 libera MUTEX 0
T2 acorda imediatamente
```

### Caso 2: E/S Simples

```
T1 começa E/S durando 5 ticks em t=2 → t=7
Enquanto T1 em E/S:
  - T1 estado = SUSPENDED (IO)
  - Outras tarefas continuam
Em t=7:
  - IRQ acordado T1 → READY
  - T1 volta a competir
```

### Caso 3: PRIOPEnv Aging

```
T1: priority=1, arrival=0
T2: priority=2, arrival=0

t=0: dyn_pri(T1) = 1 + 0/1 = 1
     dyn_pri(T2) = 2 + 0/1 = 2  ← T2 executa

t=5: dyn_pri(T1) = 1 + 5/1 = 6  ← T1 executa!
     dyn_pri(T2) = 2 + 5/1 = 7  (mas T1 já tem CPU)
```

## ✨ Resultado Esperado Final

- ✅ Código compila sem erros
- ✅ PRIOPEnv reconhecido e funciona
- ✅ Alpha parseado e usado
- ✅ Mutex parseado e funciona
- ✅ E/S parseado e funciona
- ✅ Tarefas suspensas aparecem no histórico
- ✅ Eventos de mutex/IO aparecem no histórico
- ✅ Métricas refletem tempo de espera correto

---

## 📋 Checklist Final

Após compilar e testar:

- [ ] Arquivo compila com `make`
- [ ] PRIOPEnv test executa
- [ ] Mutex test mostra suspensão
- [ ] E/S test mostra suspensão
- [ ] Combinado mostra tudo junto
- [ ] Histórico registra todos os eventos
- [ ] Métricas estão corretas
- [ ] Sem warnings ou erros

**Se tudo passou: ✅ IMPLEMENTAÇÃO VALIDADA!**
