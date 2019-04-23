#include <stdlib.h>
#include <ucontext.h>
#include <stdio.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

ucontext_t *contextoFinal = NULL;
TCB_t *threadExecutando = NULL;
int globalThreadsTid = 0;

FILA2   filaApto;
FILA2   filaBloqueado;

int InitializeQueues();                                        /** Função auxiliar 1 **/
void InitializeFinalContext();                                 /** Função auxiliar 2 **/
void EndOfThread();                                            /** Função auxiliar 3 **/
void Scheduler();                                              /** Função auxiliar 4 **/
void InitializeMainThread();                                   /** Função auxiliar 5 **/
void CreateMainThread(ucontext_t * contextoPrincipal);         /** Função auxiliar 6 **/
TCB_t * InitializeThread(int threadTid, int prio);             /** Função auxiliar 7 **/
TCB_t * SearchThreadTid(FILA2 fila, int tid);                  /** Função auxiliar 8 **/
TCB_t * SearchThreadWaiting(FILA2 fila, int tid);              /** Função auxiliar 9 **/
int RemoveThreadFromQueue(FILA2 fila, TCB_t * thread);         /** Função auxiliar 10 **/

/*************** Estrutura de Dados auxiliar *******************************************/
typedef struct s_WaitingTid {
    int waitingTid;
} WaitingTid_t;

/******************** FUNÇÕES PRINCIPAIS *************************************/

int ccreate (void* (*start)(void*), void *arg, int prio) {
	int testeFila = 1;

    if(contextoFinal == NULL)
    {
        InitializeQueues();
        InitializeFinalContext();
        InitializeMainThread();
    }
    
    // Cria e inicializa TCB_t
    TCB_t *thread = InitializeThread(globalThreadsTid, prio);

    if(thread == NULL)
    {
        printf("Erro: ccreate - nao foi possivel criar nova thread\n");
        return -1;
    }
    
    makecontext(&(thread->context), (void (*) (void)) start, 1, arg);
    
    testeFila = AppendFila2(&filaApto, (void *)(thread));
        
    if (testeFila)
    {
        printf ("Erro: ccreate - fila apto\n");
        return -1;
    }
    
    testeFila = 1;    

    globalThreadsTid++;

    return (*thread).tid;
}

int csetprio(int tid, int prio) {
	return -1;
}

int cyield(void) {
	return -1;
}

int cjoin(int tid) {
	return -1;
}

int csem_init(csem_t *sem, int count) {
	return -1;
}

int cwait(csem_t *sem) {
	return -1;
}

int csignal(csem_t *sem) {
	return -1;
}

int cidentify (char *name, int size) {	
	strncpy(name, "Gabriel Barros de Paula - 240427 - Carine - 000000 - Henrique - 000000", size);
    return 0;
}

/**************** Funções auxiliares ***************************************************/

/** Função auxiliar 1 
*** Função: Inicializa as filas - FILA2
*** Ret: == 0, se conseguiu
***   !=0, caso contrário (erro ou fila vazia) **/
int InitializeQueues(){
    int testeFila = 1;

    testeFila = CreateFila2(&filaApto);

    if (testeFila) {
        return -1;
    }
    
    testeFila = CreateFila2(&filaBloqueado);

    if (testeFila) {
        return -1;
    }

    //sucesso
    return 0;
}

/** Função auxiliar 2 **/
void InitializeFinalContext(){
    contextoFinal = (ucontext_t *) malloc(sizeof(ucontext_t));

    getcontext(contextoFinal);

    contextoFinal->uc_link = NULL; // Terminate context
    contextoFinal->uc_stack.ss_sp = (char*) malloc(SIGSTKSZ); // Stack's beginning
    contextoFinal->uc_stack.ss_size = SIGSTKSZ; // Stack's size

    makecontext(contextoFinal, (void (*) (void)) EndOfThread, 0);
}

/** Função auxiliar 3 **/
void EndOfThread(){
    TCB_t * threadAguardandoBloqueado;    
    WaitingTid_t * AuxWaitingTid;

    AuxWaitingTid = (WaitingTid_t *) malloc(sizeof(WaitingTid_t));
    AuxWaitingTid->waitingTid = -1;
        
    threadExecutando->state = PROCST_TERMINO;
    
    threadAguardandoBloqueado = SearchThreadWaiting(filaBloqueado, threadExecutando->tid);
        
    if(threadAguardandoBloqueado != NULL){
        //threadAguardandoBloqueado->data = AuxWaitingTid;
        threadAguardandoBloqueado->state = PROCST_APTO;
        //retira de bloqueado
        RemoveThreadFromQueue(filaBloqueado,threadAguardandoBloqueado);
        //coloca em apto
        AppendFila2(&filaApto, (void *)(threadAguardandoBloqueado));       
    }      
    
    Scheduler();
}

/** Função auxiliar 4 **/
void Scheduler(){
    int trocaContexto = 0;
    int testeFila = 1;

    // Pega a primeira thread da fila de apto
    TCB_t *threadEscalonada = NULL;

    if(threadExecutando->state == PROCST_BLOQ)
    {
        testeFila = AppendFila2(&filaBloqueado, (void *)(threadExecutando));
        
        if (testeFila)
        {
            printf ("Erro: Scheduler - fila bloqueado\n");
        }
        
        testeFila = 1;

        getcontext(&(threadExecutando->context));
    }
    else if(threadExecutando->state == PROCST_APTO)
    {
        testeFila = AppendFila2(&filaApto, (void *)(threadExecutando));
        
        if (testeFila)
        {
            printf ("Erro: Scheduler - fila apto\n");
        }
        
        testeFila = 1;

        getcontext(&(threadExecutando->context));
    }
    else if(threadExecutando->state == PROCST_TERMINO)
    {
        free(&(threadExecutando->context));
        free(threadExecutando);
        threadExecutando = NULL;
    }

    if(trocaContexto == 0)
    {
        trocaContexto = 1;

        testeFila = FirstFila2(&filaApto);
        
        //posiciona o iterador no primeiro elemento da fila - FILA2
        if (testeFila) {
            printf ("Erro: Scheduler - FirstFila2\n");
        }

        threadEscalonada = (TCB_t *)GetAtIteratorFila2(&filaApto);

        // Lista vazia
        if(threadEscalonada == NULL)
            return;

        threadExecutando = threadEscalonada;
        threadExecutando->state = PROCST_EXEC;

        setcontext(&(threadExecutando->context));
    }
}

/** Função auxiliar 5 **/
void InitializeMainThread(){
    int testeFila = 1;
    ucontext_t *contextoPrincipal = (ucontext_t *) malloc(sizeof(ucontext_t));
    
    getcontext(contextoPrincipal);

    ccreate((void*)CreateMainThread, contextoPrincipal, 0);

    testeFila = FirstFila2(&filaApto);
        
    //posiciona o iterador no primeiro elemento da fila - FILA2
    if (testeFila) {
        printf ("Erro: inicializaThreadPrincipal - FirstFila2\n");
    }

    //coloca em execução contexto principal
    threadExecutando = (TCB_t *)GetAtIteratorFila2(&filaApto);
    
    // Lista vazia -> pegar da lista: filaAptoSuspenso ou filaBloqueado ou filaBloqueadoSuspenso
    if(threadExecutando == NULL)
        return;   
}

/** Função auxiliar 6 **/
void CreateMainThread(ucontext_t * contextoPrincipal){
    
    threadExecutando->context = *contextoPrincipal;
    setcontext(&(threadExecutando->context));
}

/** Função auxiliar 7 **/
TCB_t * InitializeThread(int threadTid, int prio){

    // SIGSTKSZ = system default number of bytes that would be used to cover
    // an usual case when manually allocating and stack area

    TCB_t * thread = (TCB_t *) malloc(sizeof(TCB_t));
    WaitingTid_t * AuxWaitingTid;

    AuxWaitingTid = (WaitingTid_t *) malloc(sizeof(WaitingTid_t));
    AuxWaitingTid->waitingTid = -1;
        
    thread->context = *(ucontext_t *) malloc(sizeof(ucontext_t));

    if (thread == 0)
        return NULL;
    
    getcontext(&(thread->context));

    thread->context.uc_link = contextoFinal;
    thread->context.uc_stack.ss_sp = (char*) malloc(SIGSTKSZ); // Stack's beginning
    thread->context.uc_stack.ss_size = SIGSTKSZ; // Stack's size

    thread->tid = threadTid;
    thread->state = PROCST_APTO;
    thread->prio = prio;
            
    return thread;
}

/** Função auxiliar 8 **/
TCB_t * SearchThreadTid(FILA2 fila, int tid){
    TCB_t * thread;
    int testeFila =1;
    
    //posiciona o iterador no primeiro elemento da fila - FILA2
    testeFila = FirstFila2(&fila);
    
    if (testeFila) {
        //printf ("Erro: SearchThreadTid - FirstFila2 fila vazia\n");
        return NULL;
    }
    
    //pega primeiro elemento da fila
    thread = (TCB_t *)GetAtIteratorFila2(&fila);

    // Lista vazia
    if(thread == NULL)
        return NULL;       

    while(testeFila == 0)
    {
        if(thread->tid == tid)
            return thread;

        //incrementa iterador fila
        testeFila = NextFila2(&fila);
        
        if(testeFila == 0){
            //pega elemento apontado pelo iterador
            thread = (TCB_t *)GetAtIteratorFila2(&fila);
        }        
    }

    return NULL;
}

/** Função auxiliar 9 **/
TCB_t * SearchThreadWaiting(FILA2 fila, int tid){
    TCB_t * thread;
    int testeFila = 1;
            
    //posiciona o iterador no primeiro elemento da fila - FILA2
    testeFila = FirstFila2(&fila);
    
    if (testeFila) {
        //printf ("Erro: SearchThreadWaiting - FirstFila2 fila vazia\n");
        return NULL;
    }
    
    //pega primeiro elemento da fila
    thread = (TCB_t *)GetAtIteratorFila2(&fila);

    // Lista vazia
    if(thread == NULL)
        return NULL;       

    while(testeFila == 0)
    {   
        if(thread->tid == tid)
            return thread;

        //incrementa iterador fila
        testeFila = NextFila2(&fila);
        
        if(testeFila == 0){
            //pega elemento apontado pelo iterador
            thread = (TCB_t *)GetAtIteratorFila2(&fila);
        }        
    }

    return NULL;    
}

/** Função auxiliar 10 **/
/** Ret:    ==0, se conseguiu
    !=0, caso contrário (erro) **/
    
int RemoveThreadFromQueue(FILA2 fila, TCB_t * thread){   
    TCB_t * threadAux;
    int testeFila =1;
    int resultado = -1;
    
    //posiciona o iterador no primeiro elemento da fila - FILA2
    testeFila = FirstFila2(&fila);
    
    if (testeFila) {        
        return -1;
    }
    
    //primeiro elemento da fila
    threadAux = (TCB_t *)GetAtIteratorFila2(&filaBloqueado);

    // Lista vazia
    if(threadAux == NULL)
        return -1;       

    while(testeFila == 0)
    {
        if(threadAux->tid == thread->tid){
            resultado = DeleteAtIteratorFila2(&fila);
            return resultado;
        }            

        //incrementa iterador
        testeFila = NextFila2(&fila);
        
        if(testeFila == 0){
            //pega elemento apontado pelo iterador
            threadAux = (TCB_t *)GetAtIteratorFila2(&fila);
        }        
    }
    
    return -1;
}
