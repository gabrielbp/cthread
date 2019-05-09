#include "../include/cdata.h"
#include "../include/cthread.h"
#include "../include/support.h"
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>

csem_t semaforo;

void* func00()
{
    cwait(&semaforo);
    printf("FUNC00 \n Thread01 - Contador do semaforo após solicitação de recurso: %d.\n", semaforo->count);
    csignal(&semaforo);
    printf("Contador do semaforo após liberação de recurso: %d.\n", semaforo->count);
    return NULL;
}

void* func01()
{
    cwait(&semaforo);
    printf("FUNC01 \n Thread02 - Contador do semaforo após solicitação de recurso: %d.\n", semaforo->count);
    csignal(&semaforo);
    printf("Contador do semaforo após liberação de recurso: %d.\n", semaforo->count);
    return NULL;
}

void* func02()
{
    cwait(&semaforo);
    printf("FUNC02 \n Thread03 - Contador do semaforo após solicitação de recurso: %d.\n", semaforo->count);
    csignal(&semaforo);
    printf("Contador do semaforo após liberação de recurso: %d.\n", semaforo->count);
    return NULL;
}

int main()
{   int thread1,thread2,thread3;

    if(csem_init(&semaforo, 1) == 0){
        cwait(&semaforo);
        thread1 = ccreate(func00, 0, 0);
        thread2 = ccreate(func01, 0, 1);
        thread3 = ccreate(func02, 0, 1);
        cyield();
        printf("Esta na main apos a criacao da thread1, thread2 e thread3.\n");
        csignal(&semaforo);
        cyield();
        cyield();
        return 0;
    }
    return -1; // Erro ao criar semáforo
}
