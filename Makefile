#
# Makefile de EXEMPLO
#
# OBRIGATÓRIO ter uma regra "all" para geração da biblioteca e de uma
# regra "clean" para remover todos os objetos gerados.
#
# É NECESSARIO ADAPTAR ESSE ARQUIVO de makefile para suas necessidades.
#  1. Cuidado com a regra "clean" para não apagar o "support.o"
#
# OBSERVAR que as variáveis de ambiente consideram que o Makefile está no diretótio "cthread"
#

CC=gcc
LIB_DIR=lib/
INC_DIR=include/
BIN_DIR=bin/
SRC_DIR=src/

all: cthread
	ar crs libcthread.a $(BIN_DIR)lib.o $(BIN_DIR)support.o
	mv libcthread.a $(LIB_DIR)

cthread: #gera arquivo objeto do cthread.c
	$(CC) -c $(SRC_DIR)lib.c -Wall
	mv lib.o $(BIN_DIR)

clean:
	rm -rf $(LIB_DIR)*.a $(BIN_DIR)*.o $(SRC_DIR)*~ $(INC_DIR)*~ *~