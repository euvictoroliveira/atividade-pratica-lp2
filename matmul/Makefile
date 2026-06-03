CC      = gcc
CFLAGS  = -O2 -Wall -Wextra
LDFLAGS = -lpthread

SRCDIR  = src
BINDIR  = bin
RESDIR  = results

SEQ_SRC = $(SRCDIR)/matmul_seq.c
PAR_SRC = $(SRCDIR)/matmul_par.c
SEQ_BIN = $(BINDIR)/matmul_seq
PAR_BIN = $(BINDIR)/matmul_par

.PHONY: all clean bench

all: $(BINDIR) $(RESDIR) $(SEQ_BIN) $(PAR_BIN)

$(BINDIR):
	mkdir -p $(BINDIR)

$(RESDIR):
	mkdir -p $(RESDIR)

$(SEQ_BIN): $(SEQ_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(PAR_BIN): $(PAR_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

bench: all
	bash benchmark.sh

clean:
	rm -rf $(BINDIR) $(RESDIR)
