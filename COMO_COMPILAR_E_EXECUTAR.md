# INSTRUÇÕES DE COMPILAÇÃO E EXECUÇÃO

## ⚙️ Pré-requisitos

- GCC (compilador C)
- GNU Make (para usar Makefile)
- GTK+-3.0 (para GUI)
- Poppler (pdftotext para recursos adicionais)

### Windows (WSL recomendado)

```bash
sudo apt-get install build-essential libgtk-3-dev poppler-utils
```

### Linux

```bash
sudo apt-get install build-essential libgtk-3-dev poppler-utils
```

### macOS

```bash
brew install gtk+3 poppler
```

---

## 🔨 Compilação

### Opção 1: Usando Makefile (Recomendado)

```bash
cd 'C:\Users\Julio\Desktop\novo soooo\ProgetoSO'
make clean     # Remove arquivos antigos
make           # Compila tudo
```

### Opção 2: Compilação Manual

```bash
gcc -Wall -Wextra -std=c99 -g \
    -o simulador \
    main.c task.c actions.c config.c config_ext.c \
    scheduler.c simulator.c simulator_ext.c \
    gantt.c gui_gantt.c input_dialog.c modify_dialog.c \
    `pkg-config --cflags --libs gtk+-3.0` -lpthread
```

---

## ✅ Verificação de Compilação

Após compilar, deve aparecer:

```
  Compilado com sucesso!  Execute: ./simulador
```

Se houver erros:

1. Verificar se `actions.h` e `actions.c` existem
2. Verificar se `simulator_ext.h` e `simulator_ext.c` existem
3. Verificar se GTK+-3.0 está instalado
4. Executar `make clean` e tentar novamente

---

## 🎮 Execução

### Modo 1: Com Arquivo de Configuração

```bash
./simulador arquivo_config.txt
```

Exemplos:

```bash
./simulador PriorpEnv_test.txt   # PRIOPEnv com mutex e I/O
./simulador PRIOP.txt             # PRIOP tradicional
./simulador SRTF.txt              # SRTF tradicional
```

### Modo 2: Modo Interativo (Passo a Passo)

```bash
./simulador --interativo PRIOP.txt
```

Comandos:

- `ENTER` - Avança um tick
- `b + ENTER` - Retrocede um tick
- `t + ID + ENTER` - Inspeciona tarefa
- `m + ENTER` - Modifica estado
- `c + ENTER` - Ver estado das CPUs
- `q + ENTER` - Sair/Finalizar

### Modo 3: Gerar Configuração de Exemplo

```bash
./simulador --gerar-config
```

Cria `config_exemplo.txt`

---

## 📝 Formato do Arquivo de Configuração

### Linha 1: Parâmetros Gerais

```
algoritmo;quantum;num_cpus;alpha
```

**Valores:**

- `algoritmo`: `SRTF`, `PRIOP`, ou `PRIOPEnv`
- `quantum`: Número inteiro > 0 (ex: 5)
- `num_cpus`: 2 a 100 processadores
- `alpha`: Para PRIOPEnv (padrão 1, use 0 para desabilitar)

**Exemplos:**

```
SRTF;4;2           # SRTF com quantum 4, 2 CPUs
PRIOP;5;2;0        # PRIOP com alpha desabilitado
PRIOPEnv;5;2;1     # PRIOPEnv com alpha=1
```

### Linhas 2+: Definição de Tarefas

```
id;cor;chegada;duração;prioridade[;ações]
```

**Campos:**

- `id`: Identificador (número ou com prefixo, ex: t01)
- `cor`: RGB hexadecimal (ex: FF4444, 44FF44)
- `chegada`: Tick de chegada (>= 0)
- `duração`: Tempo de CPU necessário (> 0)
- `prioridade`: Valor inteiro (maior = mais prioritária)
- `ações`: Opcional (múltiplas permitidas)

**Ações Suportadas:**

- `MLxx:tt` - Lock do mutex xx no tempo tt
- `MUxx:tt` - Unlock do mutex xx no tempo tt
- `IO:tt-dd` - E/S no tempo tt com duração dd

**Exemplos:**

```
1;FF4444;0;10;3              # Tarefa 1, sem ações
2;44FF44;2;6;5;ML0:3;MU0:5   # Tarefa 2, com mutex
3;4444FF;4;8;1;IO:2-3        # Tarefa 3, com E/S
```

---

## 📂 Arquivo de Teste Incluído

### PriorpEnv_test.txt

```
PRIOPEnv;5;2;1
1;FF4444;0;10;3
2;44FF44;2;6;5;ML0:3;MU0:5
3;4444FF;4;8;1;IO:2-3
4;FFAA00;6;4;4
```

**Para executar:**

```bash
./simulador PriorpEnv_test.txt
```

---

## 🖥️ Saída Esperada

### Modo Automático

```
═══ INICIANDO SIMULAÇÃO COMPLETA ═══

[Tick   0] Tarefa 1 chegou no sistema
[Tick   2] Tarefa 2 chegou no sistema
[Tick   4] Tarefa 3 chegou no sistema
[Tick   6] Tarefa 4 chegou no sistema
[Tick   5] Tarefa 2 solicita MUTEX 0
         → Tarefa 2 SUSPENSA aguardando MUTEX 0
[Tick   6] Tarefa 3 inicia E/S com duração 3
         → Tarefa 3 SUSPENSA em E/S (será acordada em tick 9)
...
[Tick  16] ═══ SIMULAÇÃO CONCLUÍDA ═══
```

### Gráfico de Gantt

Arquivo SVG gerado automaticamente (abrir em navegador)

### Métricas Finais

```
Turnaround times:
  Tarefa 1: 10 ticks
  Tarefa 2: 11 ticks
  Tarefa 3: 13 ticks
  Tarefa 4: 14 ticks

Tempos de espera:
  Tarefa 1: 0 ticks
  Tarefa 2: 4 ticks (mutex)
  Tarefa 3: 3 ticks (I/O)
  Tarefa 4: 9 ticks
```

---

## 🔧 Troubleshooting

### Erro: "GTK+-3.0 not found"

```bash
sudo apt-get install libgtk-3-dev
```

### Erro: "No such file or directory: actions.h"

- Verificar se `actions.h` e `actions.c` estão no diretório
- Executar `make clean` e `make` novamente

### Erro: "Undefined reference to..."

- Verificar se todos os .c estão listados no Makefile
- Executar `make clean` antes de `make`

### Programa não encontra o arquivo de config

- Usar caminho absoluto: `./simulador /path/to/config.txt`
- Ou estar no mesmo diretório do arquivo

---

## 📊 Exemplo Completo de Teste

```bash
# Compilar
cd 'C:\Users\Julio\Desktop\novo soooo\ProgetoSO'
make clean
make

# Testar PRIOPEnv
./simulador PriorpEnv_test.txt

# Ver resultado (SVG gerado)
# Abrir gantt.svg em navegador
```

---

## 🎯 Testes Recomendados

### 1. Teste Básico (PRIOP)

```bash
./simulador PRIOP.txt
```

### 2. Teste PRIOPEnv

```bash
./simulador PriorpEnv_test.txt
```

### 3. Teste com Mutex

Criar `mutex_test.txt`:

```
PRIOP;5;2;0
1;FF4444;0;10;3;ML0:3;MU0:8
2;44FF44;2;6;5
```

Executar:

```bash
./simulador mutex_test.txt
```

### 4. Teste com E/S

Criar `io_test.txt`:

```
PRIOP;5;2;0
1;FF4444;0;10;3
2;44FF44;2;6;5;IO:1-2
3;4444FF;4;8;1
```

Executar:

```bash
./simulador io_test.txt
```

---

## 📋 Checklist de Compilação

- [ ] Arquivo compila com `make`
- [ ] Sem errors críticos
- [ ] Sem warnings graves
- [ ] Executável `simulador` criado
- [ ] PRIOPEnv test executa
- [ ] Mutex suspensão funciona
- [ ] E/S suspensão funciona
- [ ] Histórico registra eventos

---

## 🎓 Conceitos Testados

Ao executar com sucesso:

- ✅ Escalonamento com prioridades
- ✅ Envelhecimento dinâmico
- ✅ Sincronização com mutex
- ✅ Entrada/Saída não-bloqueante
- ✅ Suspensão e despertar de tarefas
- ✅ Múltiplos processadores
- ✅ Quantum e preempção

---

## 📞 Suporte Técnico

Caso tenha problemas:

1. Revisar `GUIA_TESTE.md`
2. Revisar `IMPLEMENTACAO.md`
3. Verificar `Makefile` (dependências)
4. Executar `make clean` e tentar novamente

---

**Sucesso na compilação e testes! 🚀**
