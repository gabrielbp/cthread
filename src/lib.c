/*
 * Desenvolvimento de um escalonador COM PRIORIDADES E NÃO PREEMPTIVO
 * Política de Múltiplas filas SEM realimentação (cada fila - FIFO)
 */

#include <stdlib.h>
#include <ucontext.h>
#include <stdio.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define ALGO_DEU_ERRADO -1
#define TUDO_CERTO 0
#define ERRO_CRIACAO_FILA -1
#define ERRO_INSERCAO_FILA -1
#define ERRO_AO_DELETAR_DA_FILA -1;

#define ALTA_PRIORIDADE 0
#define MEDIA_PRIORIDADE 1
#define BAIXA_PRIORIDADE 2

/* - Strings - */
//Criações
#define STRING_ERRO_CRIACAO_FILA_APTOS "ERROR: Erro ao criar a fila dos aptos.\n"
#define STRING_ERRO_CRIACAO_FILA_APTOS_BAIXA_PRIORIDADE "ERROR: Erro ao criar a fila dos aptos de baixa prioridade.\n"
#define STRING_ERRO_CRIACAO_FILA_APTOS_MEDIA_PRIORIDADE "ERROR: Erro ao criar a fila dos aptos de media prioridade.\n"
#define STRING_ERRO_CRIACAO_FILA_APTOS_ALTA_PRIORIDADE "ERROR: Erro ao criar a fila dos aptos de alta prioridade.\n"
#define STRING_ERRO_CRIACAO_FILA_BLOQUEADOS "ERROR: Erro ao criar a fila dos bloqueados.\n"
#define STRING_ERRO_AO_DELETAR_DA_FILA "ERROR: Erro ao deletar da fila."

//Inserções
#define STRING_ERRO_INSERCAO_FILA_APTOS "ERROR: Erro ao inserir na fila dos aptos.\n"
#define STRING_ERRO_INSERCAO_FILA_APTOS_BAIXA_PRIORIDADE "ERROR: Erro ao inserir na fila dos aptos de baixa prioridade.\n"
#define STRING_ERRO_INSERCAO_FILA_APTOS_MEDIA_PRIORIDADE "ERROR: Erro ao inserir na fila dos aptos de media prioridade.\n"
#define STRING_ERRO_INSERCAO_FILA_APTOS_ALTA_PRIORIDADE "ERROR: Erro ao inserir na fila dos aptos de alta prioridade.\n"
#define STRING_ERRO_INSERCAO_FILA_BLOQUEADOS "ERROR: Erro ao inserir na fila dos bloqueados.\n"
#define STRING_ERRO_INSERCAO_FILA_SEMAFORO "ERROR: Erro ao inserir na fila do semaforo.\n"


ucontext_t context_yield;
ucontext_t *contextoFinal = NULL;
TCB_t *threadExecutando = NULL;
int globalThreadsTid = 0;
int firstThread=0;

FILA2   filaApto; // contém todos os aptos
FILA2   filaAptoBaixaPrioridade; // apenas os aptos com prioridade igual a 2
FILA2   filaAptoMediaPrioridade; // apenas os aptos com prioridade igual a 1
FILA2   filaAptoAltaPrioridade; // apenas os aptos com prioridade igual a 0
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
        firstThread = 1;
    }

    // Cria e inicializa TCB_t
    TCB_t *thread = InitializeThread(globalThreadsTid, prio);

    if(thread == NULL)
    {
        printf("Erro: ccreate - nao foi possivel criar nova thread\n");
        return -1;
    }

    makecontext(&(thread->context), (void (*) (void)) start, 1, arg);

    // COLOCAR O SWITCH???
     switch(prio){
	    case 0:
		if(AppendFila2(&filaAptoAltaPrioridade,(void*)(thread)) != 0){
		  printf(STRING_ERRO_INSERCAO_FILA_APTOS_ALTA_PRIORIDADE);
		  return ERRO_INSERCAO_FILA;
		}
		break;
	    case 1:
		if(AppendFila2(&filaAptoMediaPrioridade,(void*)(thread)) != 0){
		  printf(STRING_ERRO_INSERCAO_FILA_APTOS_MEDIA_PRIORIDADE);
		  return ERRO_INSERCAO_FILA;
		}
		break;
	    case 2:
		if( AppendFila2(&filaAptoBaixaPrioridade,(void*)(thread)) != 0){
		   printf(STRING_ERRO_INSERCAO_FILA_APTOS_BAIXA_PRIORIDADE);
		   return ERRO_INSERCAO_FILA;
		}
		break;
	    default:
		return -1;
		break;
    }


    //ideia para substituir as 5 linhas abaixo --> colocar o if:
	//if(AppendFila2(&filaApto, (void *)(thread)) != 0) return ERRO_INSERCAO_FILA;
    testeFila = AppendFila2(&filaApto, (void *)(thread)); //LINHA 1
    if (testeFila){ //linha 2
        printf ("Erro: ccreate - fila apto\n"); //linha 3
        return -1; //linha 4
    }//linha 5

    testeFila = 1;

    globalThreadsTid++;

    return (*thread).tid;
}

/*
 * Altera a prioridade de uma thread dado o seu tid e sua nova prioridade.
 * tid = sempre NULL (especificação 2019.01)
 * prio = 0,1,2
 */
int csetprio(int tid, int prio) {
     if(prio >= 0 && prio<= 2){ //prioridades válidas
         //tid = NULL;
        if(threadExecutando->tid == tid){
            threadExecutando->prio = prio;
            return TUDO_CERTO; // executou corretamente
        }         
    }

	return ALGO_DEU_ERRADO;
}

int cyield(void) {
	 TCB_t *yieldingThread;

    int testeFila;

	if(!firstThread){
        InitializeQueues();
        InitializeFinalContext();
        InitializeMainThread();
        firstThread=1;
	}

    if(FirstFila2(&filaApto) != 0){
        printf("ERRO - cyeld - FILA VAZIA");
        return -1;
    }

    yieldingThread = threadExecutando;
    yieldingThread->state= PROCST_APTO;

    switch(threadExecutando->prio){
	    case 0:
		testeFila = AppendFila2(&filaAptoAltaPrioridade, (void *)(threadExecutando));

		break;
	    case 1:
		testeFila = AppendFila2(&filaAptoMediaPrioridade, (void *)(threadExecutando));

		break;
	    case 2:
		testeFila = AppendFila2(&filaAptoBaixaPrioridade, (void *)(threadExecutando));

		break;
	    default:
		return -1;
		break;
    }

    if(testeFila){
        printf("ERRO Inserindo fila aptos cyield\n");
        return -1;
    }
    else
    {
        printf("colocou de volta na baixa = %d\n", threadExecutando->tid);
    }


    threadExecutando= 0;
    swapcontext(&yieldingThread->context, &context_yield);
	return 0;
}

int cjoin(int tid) {
	int testeFila = 1;
    TCB_t * threadFilaApto;
    TCB_t * threadFilaBloqueado;

    testeFila = FirstFila2(&filaApto);

    //posiciona o iterador no primeiro elemento da fila - FILA2
    if (testeFila) {
        printf ("Erro: cjoin - FirstFila2 fila vazia\n");
        return -1;
    }

    threadFilaApto = SearchThreadTid(filaApto, tid);
    threadFilaBloqueado = SearchThreadTid(filaBloqueado, tid);

    if(threadFilaApto == NULL && threadFilaBloqueado == NULL){
        return -1;
    }

    if(SearchThreadWaiting(filaBloqueado, tid) != NULL){
        return -1;
    }

    threadExecutando->state = PROCST_BLOQ;
    Scheduler();
    return 0;
}

int csem_init(csem_t *sem, int count) {
	int testefila = 1;

	sem->count = count;
	sem->fila = malloc(sizeof(FILA2));

	testefila = CreateFila2(sem->fila);

	if(testefila){
        printf("ERRO, csem_init : FILA");
	}

	return 0;
}

/*
 * Solicitação de recurso
 */
int cwait(csem_t *sem) {

    //thread colocada no estado bloqueado
    if(sem->count <= 0){
        threadExecutando->state = PROCST_BLOQ;
        if(AppendFila2(&filaBloqueado, (void*)threadExecutando) != 0){
            printf(STRING_ERRO_INSERCAO_FILA_BLOQUEADOS);
	    return ERRO_INSERCAO_FILA;
	}
        if(AppendFila2(sem->fila, (void *)threadExecutando->tid) != 0){
            printf(STRING_ERRO_INSERCAO_FILA_SEMAFORO);
	   return ERRO_INSERCAO_FILA;
	}
        sem->count--; //a cada chamada de cwait decrementa
        swapcontext(&threadExecutando->context, &context_yield);
        return TUDO_CERTO;

    }else{ // recurso está livre thread continua execução normalmente
        sem->count--; //a cada chamada de cwait decrementa
        return TUDO_CERTO;
    }

	return ALGO_DEU_ERRADO;
}

int csignal(csem_t *sem) {
    if(sem != NULL){      
        if(FirstFila2(sem->fila) != 0 && NextFila2(sem->fila) != -NXTFILA_VAZIA){ //ERRO: porque não pegou o primeiro e a fila não é vazia
            return ALGO_DEU_ERRADO;
        }else if (FirstFila2(sem->fila) != 0 && NextFila2(sem->fila) == -NXTFILA_VAZIA){ //CERTO: não pegou o primeiro pq a fila e vazia
            return TUDO_CERTO;
        }else{ //conseguiu pegar o primeiro 
            TCB_t *thread = NULL;
            thread = GetAtAntIteratorFila2(sem->fila); 
            if(thread == NULL) return ALGO_DEU_ERRADO;
            
            
            for(int i=ALTA_PRIORIDADE; i<=BAIXA_PRIORIDADE; i++){ //percorre todas as prioridades
                int flag =0;
                FirstFila2(sem->fila);
                while(flag == 0 && NextFila2(sem->fila) != -NXTFILA_ENDQUEUE){
                    
                    thread = (TCB_t*) GetAtAntIteratorFila2(sem->fila);
                    if(thread->prio == i) flag = 1;
                }

                if(flag == 1){ //encontrou de alta prioridade

                    thread->state = PROCST_APTO;
                    if(AppendFila2((&filaApto), thread) != 0){ //insere na fila nque tem todos os aptos
                        printf(STRING_ERRO_INSERCAO_FILA_APTOS);
                        return ERRO_INSERCAO_FILA;
                    }

                    switch(i){
                        case ALTA_PRIORIDADE:
                                if(AppendFila2((&filaAptoAltaPrioridade), thread) != 0){
                                    printf(STRING_ERRO_INSERCAO_FILA_APTOS_ALTA_PRIORIDADE);
                                    return ERRO_INSERCAO_FILA;
                                }
                                break;
                        case MEDIA_PRIORIDADE:
                                if(AppendFila2((&filaAptoMediaPrioridade), thread) != 0){
                                    printf(STRING_ERRO_INSERCAO_FILA_APTOS_MEDIA_PRIORIDADE);
                                    return ERRO_INSERCAO_FILA;
                                }
                                break;
                        case BAIXA_PRIORIDADE:
                                if(AppendFila2((&filaAptoBaixaPrioridade), thread) != 0){
                                    printf(STRING_ERRO_INSERCAO_FILA_APTOS_BAIXA_PRIORIDADE);
                                    return ERRO_INSERCAO_FILA;
                                }
                                break;
                        default:
                                break;
                                return ALGO_DEU_ERRADO;
                        
                    }

                    if(DeleteAtIteratorFila2(sem->fila) !=0){
                        printf(STRING_ERRO_AO_DELETAR_DA_FILA);
                        return ERRO_AO_DELETAR_DA_FILA;
                    }
                    sem->count++;
                    return TUDO_CERTO;
                }
            }

        }
    }
	return ALGO_DEU_ERRADO;
}

int cidentify (char *name, int size) {
	strncpy(name, "Gabriel Barros de Paula - 240427 - Carine Bertagnolli Bathaglini- 274715 - Henrique da Silva Barboza  - 272730", size);
    return 0;
}

/**************** Funções auxiliares ***************************************************/

void *scheduleAfterYield(){

    TCB_t *threadEscalonada = NULL;

    if( (FirstFila2(&filaAptoAltaPrioridade)) == 0) 
    {  //Se fila de alta prioridade não está vazia
        threadEscalonada = (TCB_t *)GetAtIteratorFila2(&filaAptoAltaPrioridade); //Pega Primeiro
        DeleteAtIteratorFila2(&filaAptoAltaPrioridade);  //Deleta ele da fila de aptos
    }
    else if((FirstFila2(&filaAptoMediaPrioridade)) == 0)
    {
        threadEscalonada = (TCB_t *)GetAtIteratorFila2(&filaAptoMediaPrioridade); //Pega primeiro
        DeleteAtIteratorFila2(&filaAptoMediaPrioridade);
    }
    else if((FirstFila2(&filaAptoBaixaPrioridade)) == 0)
    {
        threadEscalonada = (TCB_t *)GetAtIteratorFila2(&filaAptoBaixaPrioridade);
        DeleteAtIteratorFila2(&filaAptoBaixaPrioridade);
        printf("pegou da baixa, id = %d \n", threadEscalonada->tid);
    }
    
    if(threadEscalonada==NULL){
        printf("ERRO - ao selecionar thread dos aptos\n");
    }        

    threadExecutando = threadEscalonada;
    threadExecutando->state = PROCST_EXEC;
    setcontext(&threadExecutando->context);
    return 0;
}

/** Função auxiliar 1
*** Função: Inicializa as filas - FILA2
*** Ret: == 0, se conseguiu
***   !=0, caso contrário (erro ou fila vazia) **/
int InitializeQueues(){
    int testeFila = 1;

   /** Filas de prioridades dos Aptos **/

    //TODOS OS APTOS
    testeFila = CreateFila2(&filaApto);

    if (testeFila) {
        return -1;
    }

    // BAIXA PRIORIDADE
     if(CreateFila2(&filaAptoBaixaPrioridade) != 0) {
	 printf(STRING_ERRO_CRIACAO_FILA_APTOS_BAIXA_PRIORIDADE);
	 return ERRO_CRIACAO_FILA;
     }

    //MEDIA PRIORIDADE
     if(CreateFila2(&filaAptoMediaPrioridade) != 0){
	printf(STRING_ERRO_CRIACAO_FILA_APTOS_MEDIA_PRIORIDADE);
	return ERRO_CRIACAO_FILA;
     }

    //ALTA PRIORIDADE
     if(CreateFila2(&filaAptoAltaPrioridade)!= 0){
	printf(STRING_ERRO_CRIACAO_FILA_APTOS_ALTA_PRIORIDADE);
	return ERRO_CRIACAO_FILA;
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

     //Inicia contexto de yield a ser utilizado na cyeld
    getcontext(&context_yield);
    context_yield.uc_link = NULL; // Terminate context
    context_yield.uc_stack.ss_sp = (char*) malloc(SIGSTKSZ); // Stack's beginning
    context_yield.uc_stack.ss_size = SIGSTKSZ; // Stack's size
    makecontext(&context_yield, (void (*) (void)) scheduleAfterYield, 0);
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
        //yeldingThread = threadExecutando;

        switch(threadExecutando->prio){
            case 0:
            testeFila = AppendFila2(&filaAptoAltaPrioridade, (void *)(threadExecutando));

            break;
            case 1:
            testeFila = AppendFila2(&filaAptoMediaPrioridade, (void *)(threadExecutando));

            break;
            case 2:
            testeFila = AppendFila2(&filaAptoBaixaPrioridade, (void *)(threadExecutando));

            break;
            default:
            //return -1;
            break;
        }
    }
    else if(threadExecutando->state == PROCST_TERMINO)
    {
        free(&(threadExecutando->context));
        free(threadExecutando);
        threadExecutando = NULL;
    }
        
    //testeFila=1
    // getcontext(&(threadExecutando->context));
    
    if(trocaContexto == 0)
    {
         trocaContexto = 1;

        testeFila = FirstFila2(&filaApto);

        //posiciona o iterador no primeiro elemento da fila - FILA2
        if (testeFila) {
            printf ("Erro: Scheduler - FirstFila2\n");
        }

        //I >THINK< THE PRIORITIES SHOULD BE LIKE THIS
        if( (FirstFila2(&filaAptoAltaPrioridade)) == 0)
        { 
            threadEscalonada = (TCB_t *)GetAtIteratorFila2(&filaAptoAltaPrioridade);
        }
        else if((FirstFila2(&filaAptoMediaPrioridade)) == 0)
        {
            threadEscalonada = (TCB_t *)GetAtIteratorFila2(&filaAptoMediaPrioridade);
        }
        else
        {
            threadEscalonada = (TCB_t *)GetAtIteratorFila2(&filaAptoBaixaPrioridade);
        }

        // Lista vazia
        if(threadEscalonada == NULL){
            printf(("ALL QUEUES EMPTY /n /n"));
            return;
            }

        threadExecutando = threadEscalonada;
        threadExecutando->state = PROCST_EXEC;

       /* if(yeldingThread != NULL)
            swap(yeldingThread->context,threadEscalonada->context);
        else */
        setcontext(&(threadExecutando->context));
    }
}

/** Função auxiliar 5 **/
void InitializeMainThread(){
    int testeFila = 1;
    ucontext_t *contextoPrincipal = (ucontext_t *) malloc(sizeof(ucontext_t));

    getcontext(contextoPrincipal);

    ccreate((void*)CreateMainThread, contextoPrincipal, 2);

    testeFila = FirstFila2(&filaAptoBaixaPrioridade);

    //posiciona o iterador no primeiro elemento da fila - FILA2
    if (testeFila) {
        printf ("Erro: inicializaThreadPrincipal - FirstFila2\n");
    }


    threadExecutando = (TCB_t *)GetAtIteratorFila2(&filaAptoBaixaPrioridade);

    DeleteAtIteratorFila2(&filaAptoBaixaPrioridade);

    //coloca em execução contexto principal
    //   printf("executando %d \n",threadExecutando->tid);
    //printf("Deletou da baixa\n");

    threadExecutando->state= PROCST_EXEC;

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