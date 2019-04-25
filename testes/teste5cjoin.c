/******************************************************************************
Includes
******************************************************************************/
#include "../include/cdata.h"
#include "../include/cthread.h"
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
Funções auxiliares
******************************************************************************/
int inicializaFilas();                  						/** Função auxiliar 1 **/
void inicializaContextoFinal();         						/** Função auxiliar 2 **/
void conclusaoThread();                 						/** Função auxiliar 3 **/
void escalonador();                     						/** Função auxiliar 4 **/
void inicializaThreadPrincipal();								/** Função auxiliar 5 **/
void criaThreadPrincipal(ucontext_t * contextoPrincipal);		/** Função auxiliar 6 **/
TCB_t * inicializaThread(int threadTid);						/** Função auxiliar 7 **/
TCB_t * procuraThreadTid(FILA2 fila, int tid);					/** Função auxiliar 8 **/
TCB_t * procuraThreadAguardando(FILA2 fila, int tid);			/** Função auxiliar 9 **/
int retiraThreadFila(FILA2 fila, TCB_t * thread);				/** Função auxiliar 10 **/

void* func0(void *arg);
void* func1(void *arg);



/******************************************************************************
*******************************************************************************
******************** FUNÇÕES PRINCIPAIS ***************************************
*******************************************************************************
******************************************************************************/

/******************************************************************************
Parâmetros:
	name:	ponteiro para uma área de memória onde deve ser escrito um string que contém os nomes dos componentes do grupo e seus números de cartão.
		Deve ser uma linha por componente.
	size:	quantidade máxima de caracteres que podem ser copiados para o string de identificação dos componentes do grupo.
Retorno:
	Se correto => 0 (zero)
	Se erro	   => Valor negativo.
******************************************************************************/
int cidentify (char *name, int size){
    strncpy(name, "Gabriel Barros - 240427", (size_t)size);

    return 0;
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
	Sem parâmetros
Retorno:
	Se correto => 0 (zero)
	Se erro	   => Valor negativo.
******************************************************************************/
int cyield(void){
    
    if(threadExecutando == NULL)
        return -1;

    threadExecutando->state = PROCST_APTO;
    escalonador();

    return 0;
}

/******************************************************************************
Parâmetros:
	tid:	identificador da thread cujo término está sendo aguardado.
Retorno:
	Se correto => 0 (zero)
	Se erro	   => Valor negativo.
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

/******************************************************************************
Parâmetros:
	tid:	identificador da thread a ser suspensa.
Retorno:
	Se correto => 0 (zero)
	Se erro	   => Valor negativo.
******************************************************************************/
int csuspend(int tid){
	TCB_t *threadFilaApto;
	TCB_t *threadFilaBloqueado;
	int resultado = 1;
    
    if(threadExecutando->tid == tid)
		return -1;
		
	threadFilaApto = procuraThreadTid(filaApto, tid);    
    threadFilaBloqueado = procuraThreadTid(filaBloqueado, tid);
    	
	if(threadFilaApto == NULL && threadFilaBloqueado == NULL){
        return -1;
    }
		
	if(threadFilaApto != NULL && threadFilaApto->state == PROCST_APTO){
		threadFilaApto->state = PROCST_APTO_SUS;
		//retira da lista de apto
		resultado = retiraThreadFila(filaApto, threadFilaApto);
		//coloca na lista apto suspenso
		resultado = AppendFila2(&filaAptoSuspenso, (void *)(threadFilaApto));
	}
	
	if(threadFilaBloqueado != NULL && threadFilaApto->state == PROCST_BLOQ){
		threadFilaApto->state = PROCST_BLOQ_SUS;
		//retira da lista de bloqueado
		resultado = retiraThreadFila(filaBloqueado, threadFilaBloqueado);
		//coloca na lista bloqueado suspenso
		resultado = AppendFila2(&filaBloqueadoSuspenso, (void *)(threadFilaBloqueado));
	}
	
	if(resultado == 0)
		return resultado;
	else
		return -1;
}

/******************************************************************************
Parâmetros:
	tid:	identificador da thread que terá sua execução retomada.
Retorno:
	Se correto => 0 (zero)
	Se erro	   => Valor negativo.
******************************************************************************/
int cresume(int tid){
    
    TCB_t *threadFilaAptoSuspenso;
	TCB_t *threadFilaBloqueadoSuspenso;
	int resultado = 1;
    
    if(threadExecutando->tid == tid)
		return -1;
		
	threadFilaAptoSuspenso = procuraThreadTid(filaAptoSuspenso, tid);    
    threadFilaBloqueadoSuspenso = procuraThreadTid(filaBloqueadoSuspenso, tid);
    	
	if(threadFilaAptoSuspenso == NULL && threadFilaBloqueadoSuspenso == NULL){
        return -1;
    }
		
	if(threadFilaAptoSuspenso != NULL && threadFilaAptoSuspenso->state == PROCST_APTO_SUS){
		threadFilaAptoSuspenso->state = PROCST_APTO;
		//retira da lista de apto suspenso
		resultado = retiraThreadFila(filaAptoSuspenso, threadFilaAptoSuspenso);
		//coloca na lista apto
		resultado = AppendFila2(&filaApto, (void *)(threadFilaAptoSuspenso));
	}
	
	if(threadFilaBloqueadoSuspenso != NULL && threadFilaBloqueadoSuspenso->state == PROCST_BLOQ_SUS){
		threadFilaBloqueadoSuspenso->state = PROCST_BLOQ;
		//retira da lista de bloqueado suspenso
		resultado = retiraThreadFila(filaBloqueadoSuspenso, threadFilaBloqueadoSuspenso);
		//coloca na lista bloqueado
		resultado = AppendFila2(&filaBloqueado, (void *)(threadFilaBloqueadoSuspenso));
	}
	
	if(resultado == 0)
		return resultado;
	else
		return -1;
}

/******************************************************************************
Parâmetros:
	sem:	ponteiro para uma variável do tipo csem_t. Aponta para uma estrutura de dados que representa a variável semáforo.
	count: valor a ser usado na inicialização do semáforo. Representa a quantidade de recursos controlados pelo semáforo.
Retorno:
	Se correto => 0 (zero)
	Se erro	   => Valor negativo.
******************************************************************************/
int csem_init(csem_t *sem, int count){
    return -1;
}

/******************************************************************************
Parâmetros:
	sem:	ponteiro para uma variável do tipo semáforo.
Retorno:
	Se correto => 0 (zero)
	Se erro	   => Valor negativo.
******************************************************************************/
int cwait(csem_t *sem){
    return -1;
}

/******************************************************************************
Parâmetros:
	sem:	ponteiro para uma variável do tipo semáforo.
Retorno:
	Se correto => 0 (zero)
	Se erro	   => Valor negativo.
******************************************************************************/
int csignal(csem_t *sem){
    return -1;
}

/******************************************************************************
*******************************************************************************
******************** FUNÇÕES AUXILIARES ***************************************
*******************************************************************************
******************************************************************************/

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
	
	testeFila = CreateFila2(&filaAptoSuspenso);

	if (testeFila) {
		return -1;
	}
		
	testeFila = CreateFila2(&filaBloqueado);

	if (testeFila) {
		return -1;
	}
		
	testeFila = CreateFila2(&filaBloqueadoSuspenso);

	if (testeFila) {
		return -1;
	}

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
    TCB_t * threadAguardandoBloqueado;
    TCB_t * threadAguardandoBloqueadoSuspenso;
    WaitingTid_t * AuxWaitingTid;

    AuxWaitingTid = (WaitingTid_t *) malloc(sizeof(WaitingTid_t));
    AuxWaitingTid->waitingTid = -1;
        
    threadExecutando->state = PROCST_TERMINO;
    
    threadAguardandoBloqueado = procuraThreadAguardando(filaBloqueado, threadExecutando->tid);
    threadAguardandoBloqueadoSuspenso = procuraThreadAguardando(filaBloqueadoSuspenso, threadExecutando->tid);
    
    if(threadAguardandoBloqueado != NULL){
        threadAguardandoBloqueado->data = AuxWaitingTid;
        threadAguardandoBloqueado->state = PROCST_APTO;
        //retira de bloqueado
        retiraThreadFila(filaBloqueado,threadAguardandoBloqueado);
        //coloca em apto
        AppendFila2(&filaApto, (void *)(threadAguardandoBloqueado));       
    }
    
    if(threadAguardandoBloqueadoSuspenso != NULL){
        threadAguardandoBloqueadoSuspenso->state = PROCST_APTO_SUS;
        threadAguardandoBloqueadoSuspenso->data = AuxWaitingTid;
        //retira de bloqueado suspenso
        retiraThreadFila(filaBloqueadoSuspenso,threadAguardandoBloqueadoSuspenso);
        //coloca em apto suspenso
        AppendFila2(&filaAptoSuspenso, (void *)(threadAguardandoBloqueadoSuspenso));
    }
    
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

        // Lista vazia
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
    WaitingTid_t * AuxWaitingTid;

    AuxWaitingTid = (WaitingTid_t *) malloc(sizeof(WaitingTid_t));
    AuxWaitingTid->waitingTid = -1;

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
    thread->data = AuxWaitingTid;
    
    return thread;
}

/** Função auxiliar 8 **/
TCB_t * procuraThreadTid(FILA2 fila, int tid){
    TCB_t * thread;
    int testeFila =1;
    
    //posiciona o iterador no primeiro elemento da fila - FILA2
    testeFila = FirstFila2(&fila);
    
	if (testeFila) {
		//printf ("Erro: procuraThreadTid - FirstFila2 fila vazia\n");
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
TCB_t * procuraThreadAguardando(FILA2 fila, int tid){
    TCB_t * thread;
    int testeFila =1;
    int AuxIntTid = 0;
    WaitingTid_t * AuxWaitingTid;

    AuxWaitingTid = (WaitingTid_t *) malloc(sizeof(WaitingTid_t));
    
    //posiciona o iterador no primeiro elemento da fila - FILA2
    testeFila = FirstFila2(&fila);
    
	if (testeFila) {
		//printf ("Erro: procuraThreadAguardando - FirstFila2 fila vazia\n");
		return NULL;
	}
    
    //pega primeiro elemento da fila
    thread = (TCB_t *)GetAtIteratorFila2(&fila);

	// Lista vazia
	if(thread == NULL)
		return NULL;       

    while(testeFila == 0)
    {
        AuxWaitingTid = thread->data;
        AuxIntTid = AuxWaitingTid->waitingTid;

        if(AuxIntTid == tid)
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
/** Ret:	==0, se conseguiu
	!=0, caso contrário (erro) **/
	
int retiraThreadFila(FILA2 fila, TCB_t * thread){	
    TCB_t * threadAux;
    int testeFila =1;
    int resultado = -1;
    
    //posiciona o iterador no primeiro elemento da fila - FILA2
    testeFila = FirstFila2(&fila);
    
	if (testeFila) {		
		return -1;
	}
    
    //primeiro elemento da fila
    threadAux = (TCB_t *)GetAtIteratorFila2(&filaBloqueadoSuspenso);

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


void* func0(void *arg) {
	printf("Eu sou a thread ID0 imprimindo %d\n", *((int *)arg));
	return;
}

void* func1(void *arg) {
	printf("Eu sou a thread ID1 imprimindo %d\n", *((int *)arg));
}

int main(int argc, char *argv[]) {

	int	id0, id1;
	int i;

	id0 = ccreate(func0, (void *)&i, 0);
	id1 = ccreate(func1, (void *)&i, 0);

	printf("Eu sou a main após a criação de ID0 e ID1\n");

	cjoin(id0);
	cjoin(id1);

	printf("Eu sou a main voltando para terminar o programa\n");
}

