#
# Makefile para programas exemplo
#

CC=gcc
LIB_DIR=../lib

all: teste1cidentify teste2ccreate teste3support teste4ccreate teste5cjoin teste6cyield teste7csetprio

teste1cidentify: teste1cidentify.c $(LIB_DIR)/libcthread.a
	$(CC) -g -o teste1cidentify teste1cidentify.c -L$(LIB_DIR) -lcthread -Wall

teste2ccreate: teste2ccreate.c $(LIB_DIR)/libcthread.a
	$(CC) -g -o teste2ccreate teste2ccreate.c -L$(LIB_DIR) -lcthread -Wall

teste3support: teste3support.c $(LIB_DIR)/libcthread.a
	$(CC) -g -o teste3support teste3support.c -L$(LIB_DIR) -lcthread -Wall
	
teste4ccreate: teste4ccreate.c $(LIB_DIR)/libcthread.a
	$(CC) -g -o teste4ccreate teste4ccreate.c -L$(LIB_DIR) -lcthread -Wall
	
teste5cjoin: teste5cjoin.c $(LIB_DIR)/libcthread.a
	$(CC) -g -o teste5cjoin teste5cjoin.c -L$(LIB_DIR) -lcthread -Wall

teste6cyield: teste6cyield.c $(LIB_DIR)/libcthread.a
	$(CC) -g -o teste6cyield teste6cyield.c -L$(LIB_DIR) -lcthread -Wall	

teste7csetprio: teste7csetprio.c $(LIB_DIR)/libcthread.a
	$(CC) -g teste7csetprio.c -L$(LIB_DIR) -lcthread -Wall	

clean:
	rm -rf teste1cidentify teste2ccreate teste3support teste4ccreate teste5cjoin teste6cyield teste7csetprio *.o *~
