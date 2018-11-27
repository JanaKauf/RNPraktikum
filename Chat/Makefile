# My Makefile
# Author: Janaina Kaufmann (ECS), Torben Kunow (IT)

CC      = gcc
CFLAGS  = -Wall -o2 -ggdb -O0 -g3
LDFLAGS = -pthread -lrt

SRCDIR  = src
OBJDIR  = obj
BINDIR  = bin

SRC     = $(wildcard $(SRCDIR)/*.c)
OBJ     = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SRC:.c=.o))
DEPS    = $(OBJ:.o=.d)
# ifdef cond.c
TARGETS = $(BINDIR)/chat

RED = \033[0;31m
NC = \033[0m

all: $(TARGETS)
-include $(DEPS)

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	@echo -e "${RED}Eine Abhaengigkeit fuer $< erstellt! ${NC}"
	@mkdir -p $(@D)
	$(CC) -MM -MT "$@ $(patsubst %.d,%.o,$@)" -MF $@ $<


$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo -e "${RED}Ein Objekt fuer $< erstellt! ${NC}"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/chat: $(OBJDIR)/chat.o $(OBJDIR)/server.o $(OBJDIR)/thpool.o $(OBJDIR)/task.o $(OBJDIR)/list.o $(OBJDIR)/list_thrsafe.o $(OBJDIR)/client.o

	@echo -e "${RED}EXECUTABLE von $< ${NC}"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install:
	@echo -e "${RED}Installation vorbereitet! ${NC}"
	@mkdir -p $(BINDIR)
	mv $(BINDIR)/

clean:
	@echo -e "${RED}Temporaere Dateien geloescht! ${NC}"
	rm -rf $(OBJDIR)

delete: clean
	@echo -e "${RED}BINDIR geloescht! ${NC}"
	rm -r -f $(BINDIR)

run:
	bin/./chat
