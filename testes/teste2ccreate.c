#include "../include/cdata.h"
//#include "../include/cthread.h"
#include "../include/support.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>

/******************************************************************************
Filas
******************************************************************************/
FILA2	filaApto;
FILA2	filaAptoSuspenso;
FILA2	filaBloqueado;
FILA2	filaBloqueadoSuspenso;

/******************************************************************************
Variáveis globais
******************************************************************************/
ucontext_t *contextoFinal = NULL;
TCB_t *threadExecutando = NULL;
int globalThreadsTid = 0;

/******************************************************************************
Estrutura de Dados auxiliar
******************************************************************************/
typedef struct s_WaitingTid {
    int waitingTid;
} WaitingTid_t;

/******************************************************************************
Funçoes auxiliares
******************************************************************************/
int inicializaFilas();                  						/** Função auxiliar 1 **/
void inicializaContextoFinal();         						/** Função auxiliar 2 **/
void conclusaoThread();                 						/** Função auxiliar 3 **/
void escalonador();                     						/** Função auxiliar 4 **/
void inicializaThreadPrincipal();								/** Função auxiliar 5 **/
void criaThreadPrincipal(ucontext_t * contextoPrincipal);		/** Função auxiliar 6 **/
TCB_t * inicializaThread(int threadTid);						/** Função auxiliar 7 **/
void* func0(void *arg);

/** Função para teste **/
void* func0(void *arg) {
	printf("Eu sou a thread ID0 imprimindo %d\n", *((int *)arg));
	return;
}

void *func1(void *arg) {
    printf("Eu sou a thread ID1 imprimindo %d\n", *((int *) arg));
}

/** Função auxiliar 1 **/
/**
Função:	Inicializa as filas - FILA2
Ret:	==0, se conseguiu
	!=0, caso contrário (erro ou fila vazia)
**/
int inicializaFilas(){
	int testeFila = 1;

	testeFila = CreateFila2(&filaApto);

    if (testeFila) {
		return -1;
	}
	
	testeFila = 1;
	
	testeFila = CreateFila2(&filaAptoSuspenso);

	if (testeFila) {
		return -1;
	}
	
	testeFila = 1;
	
	testeFila = CreateFila2(&filaBloqueado);

	if (testeFila) {
		return -1;
	}
	
	testeFila = 1;
	
	testeFila = CreateFila2(&filaBloqueadoSuspenso);

	if (testeFila) {
		return -1;
	}
	
	testeFila = 1;

	//sucesso
	return 0;
}

/** Função auxiliar 2 **/
void inicializaContextoFinal(){
    contextoFinal = (ucontext_t *) malloc(sizeof(ucontext_t));

    getcontext(contextoFinal);

    contextoFinal->uc_link = NULL; // Terminate context
    contextoFinal->uc_stack.ss_sp = (char*) malloc(SIGSTKSZ); // Stack's beginning
    contextoFinal->uc_stack.ss_size = SIGSTKSZ; // Stack's size

    makecontext(contextoFinal, (void (*) (void)) conclusaoThread, 0);
}

/** Função auxiliar 3 **/
void conclusaoThread(){
    threadExecutando->state = PROCST_TERMINO;

    //TO DO: verificar semáforo
    //waitingThread = searchWaitingThreads(runningThread->tid);

    // Verify blocked list
//    if(waitingThread != NULL)
//    {
//        waitingThread->waitingTid = INVALID_TID;
//        unblockThread(waitingThread);
//    }

    // Call again schedule
    escalonador();
}

/** Função auxiliar 4 **/
void escalonador(){
    int trocaContexto = 0;
    int testeFila = 1;

    // Pega a primeira thread da fila de apto
    TCB_t *threadEscalonada = NULL;

    if(threadExecutando->state == PROCST_BLOQ)
    {
        testeFila = AppendFila2(&filaBloqueado, (void *)(threadExecutando));
        
        if (testeFila)
        {
			printf ("Erro: escalonador - fila bloqueado\n");
		}
		
		testeFila = 1;

        getcontext(&(threadExecutando->context));
    }
    else if(threadExecutando->state == PROCST_BLOQ_SUS)
    {
        testeFila = AppendFila2(&filaBloqueadoSuspenso, (void *)(threadExecutando));
        
        if (testeFila)
        {
			printf ("Erro: escalonador - fila bloqueado suspenso\n");
		}
		
		testeFila = 1;
    }
    else if(threadExecutando->state == PROCST_APTO)
    {
        testeFila = AppendFila2(&filaApto, (void *)(threadExecutando));
        
        if (testeFila)
        {
			printf ("Erro: escalonador - fila apto\n");
		}
		
		testeFila = 1;

        getcontext(&(threadExecutando->context));
    }
    else if(threadExecutando->state == PROCST_APTO_SUS)
    {
        testeFila = AppendFila2(&filaAptoSuspenso, (void *)(threadExecutando));
        
        if (testeFila)
        {
			printf ("Erro: escalonador - fila apto suspenso\n");
		}
		
		testeFila = 1;
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
            printf ("Erro: escalonador - FirstFila2\n");
        }

        threadEscalonada = (TCB_t *)GetAtIteratorFila2(&filaApto);

        // Empty list, do not schedule any process ( precisa pegar processos da lista de bloqueado )
        if(threadEscalonada == NULL)
            return;

        threadExecutando = threadEscalonada;
        threadExecutando->state = PROCST_EXEC;

        setcontext(&(threadExecutando->context));
    }
}

/** Função auxiliar 5 **/
void inicializaThreadPrincipal(){
    int testeFila = 1;
    ucontext_t *contextoPrincipal = (ucontext_t *) malloc(sizeof(ucontext_t));
    
    getcontext(contextoPrincipal);

    ccreate((void*)criaThreadPrincipal, contextoPrincipal, 0);

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
void criaThreadPrincipal(ucontext_t * contextoPrincipal){
    
    threadExecutando->context = *contextoPrincipal;
    setcontext(&(threadExecutando->context));
}

/** Função auxiliar 7 **/
TCB_t * inicializaThread(int threadTid){

    // SIGSTKSZ = system default number of bytes that would be used to cover
    // an usual case when manually allocating and stack area

    TCB_t * thread = (TCB_t *) malloc(sizeof(TCB_t));

    thread->context = *(ucontext_t *) malloc(sizeof(ucontext_t));

    if (thread == 0)
        return NULL;
    
    getcontext(&(thread->context));

    thread->context.uc_link = contextoFinal;
    thread->context.uc_stack.ss_sp = (char*) malloc(SIGSTKSZ); // Stack's beginning
    thread->context.uc_stack.ss_size = SIGSTKSZ; // Stack's size

    thread->tid = threadTid;
    thread->state = PROCST_APTO;
    thread->prio = 0;
    
    return thread;
}

/******************************************************************************
Parâmetros:
	start:	ponteiro para a função que a thread executará.
	arg:	um parâmetro que pode ser passado para a thread na sua criação.
	prio:	NÃO utilizado neste semestre, deve ser sempre zero.
Retorno:
	Se correto => Valor positivo, que representa o identificador da thread criada
	Se erro	   => Valor negativo.
******************************************************************************/
int ccreate (void* (*start)(void*), void *arg, int prio){
	int testeFila = 1;

    if(contextoFinal == NULL)
    {
        inicializaFilas();
        inicializaContextoFinal();
        inicializaThreadPrincipal();
    }
    
    // Cria e inicializa TCB_t
    TCB_t *thread = inicializaThread(globalThreadsTid);

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

/******************************************************************************
Parâmetros:
    tid:    identificador da thread cujo término está sendo aguardado.
Retorno:
    Se correto => 0 (zero)
    Se erro    => Valor negativo.
******************************************************************************/
int cjoin(int tid){
    int testeFila = 1;
    TCB_t * threadFilaApto;
    TCB_t * threadFilaAptoSuspenso;
    TCB_t * threadFilaBloqueado;
    TCB_t * threadFilaBloqueadoSuspenso;
    WaitingTid_t * AuxWaitingTid;

    testeFila = FirstFila2(&filaApto);
        
    //posiciona o iterador no primeiro elemento da fila - FILA2
    if (testeFila) {
        printf ("Erro: cjoin - FirstFila2 fila vazia\n");
        return -1;
    }
    
    threadFilaApto = procuraThreadTid(filaApto, tid);
    threadFilaAptoSuspenso = procuraThreadTid(filaAptoSuspenso, tid);
    threadFilaBloqueado = procuraThreadTid(filaBloqueado, tid);
    threadFilaBloqueadoSuspenso = procuraThreadTid(filaBloqueadoSuspenso, tid);
    
    if(threadFilaApto == NULL && threadFilaAptoSuspenso == NULL && threadFilaBloqueado == NULL && threadFilaBloqueadoSuspenso == NULL){
        return -1;
    }
        
    if(procuraThreadAguardando(filaBloqueado, tid) != NULL)
        return -1;
        
    if(procuraThreadAguardando(filaBloqueadoSuspenso, tid) != NULL)
        return -1; 

    AuxWaitingTid = (WaitingTid_t *) malloc(sizeof(WaitingTid_t));
    AuxWaitingTid->waitingTid = tid;

    threadExecutando->data = AuxWaitingTid;
    threadExecutando->state = PROCST_BLOQ;
    
    escalonador();

    return 0;
}

int main(int argc, char *argv[]) {

	int	id0, id1;
	int tam = 25;
	int i;
	char *name = (char *)malloc(tam * sizeof(char));

	id0 = ccreate(func0, (void *)&i, 0);
    id1 = ccreate(func1, (void *) &i, 0);
	
	printf("Eu sou a main após a criação de ID0 e ID1\n");

    cjoin(id0);
    cjoin(id1);

    printf("Após o join\n");
}

