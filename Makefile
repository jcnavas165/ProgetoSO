# projeto A SO
# Autores: Julio Cesar Navas e Nathálya Chaves 

CC     = gcc
CFLAGS = -Wall -Wextra -std=c99 -g `pkg-config --cflags gtk+-3.0`
LIBS   = `pkg-config --libs gtk+-3.0` -lpthread

TARGET = simulador

SRCS = main.c \
       task.c \
       config.c \
       config_ext.c \
       scheduler.c \
       simulator.c \
       gantt.c \
       gui_gantt.c \
       input_dialog.c \
       modify_dialog.c

OBJS = $(SRCS:.c=.o)

all: checar_deps $(TARGET)
	@clear
	@echo ""
	@echo "  Compilado com sucesso!  Execute: ./simulador"
	@echo ""

checar_deps:
	@echo "Verificando dependências..."
	@if ! pkg-config --exists gtk+-3.0 2>/dev/null; then \
		echo "GTK nao encontrado. Instalando..."; \
		sudo apt-get update -qq && sudo apt-get install -y libgtk-3-dev -qq; \
	else \
		echo "GTK: OK"; \
	fi
	@if ! command -v pdftotext >/dev/null 2>&1; then \
		echo "pdftotext nao encontrado. Instalando..."; \
		sudo apt-get install -y poppler-utils -qq; \
	else \
		echo "pdftotext: OK"; \
	fi
	@echo ""

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@ 2>/dev/null

main.o:          main.c          task.h config.h scheduler.h simulator.h gantt.h gui_gantt.h
task.o:          task.c          task.h
config.o:        config.c        config.h task.h
config_ext.o:    config_ext.c    config_ext.h config.h
scheduler.o:     scheduler.c     scheduler.h task.h config.h
simulator.o:     simulator.c     simulator.h task.h config.h scheduler.h
gantt.o:         gantt.c         gantt.h simulator.h
gui_gantt.o:     gui_gantt.c     gui_gantt.h simulator.h task.h input_dialog.h modify_dialog.h
input_dialog.o:  input_dialog.c  input_dialog.h config.h config_ext.h task.h
modify_dialog.o: modify_dialog.c modify_dialog.h simulator.h task.h

run: all
	@if [ ! -f config_exemplo.txt ]; then ./simulador --gerar-config; fi
	@clear
	@./$(TARGET) config_exemplo.txt

clean:
	@clear
	@rm -f $(OBJS) $(TARGET) *.svg
	@echo "Limpo!"

instalar:
	@clear
	@sudo apt-get update -qq
	@sudo apt-get install -y libgtk-3-dev poppler-utils -qq

.PHONY: all checar_deps run clean instalar