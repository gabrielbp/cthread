#include "../include/cdata.h"
//#include "../include/cthread.h"
#include "../include/support.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>

void* func0(void *arg) {
	printf("Eu sou a thread ID0 imprimindo %d\n", *((int *)arg));
}

int main(int argc, char *argv[]) {

	int	thread;
	int i;
    int prioridadeAnterior = -1;

	thread = ccreate(func0, (void *)&i, 1); // cria uma thread com prioridade 1
    prioridadeAnterior = thread->prio;
    printf("Prioridade da thread %d: %d .",thread->tid, thread->prio);
    csetprio(thread->tid,2); //setou a prioridade da thread de 1 para 2
    printf("Atualização de prioridade da thread %d, da prioridade %d para %d.",thread->tid,prioridadeAnterior, thread->prio);
    cyield(); // thread libera a cpu

}
