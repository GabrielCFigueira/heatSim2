
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "matrix2d.h"
#include "thread.h"



/*mutex e condicao usados na sincronizacao dos threads */
pthread_cond_t 		wait_for_all_threads;
pthread_mutex_t		barrier_mutex;




/*funcao de erro e termino */
void die(char* reason) {
	fprintf(stderr, "%s\n", reason);
	exit(1);
}



/*inicializa o mutex e variável de condicao */
void init_mutex_cond() {

	if(pthread_cond_init(&wait_for_all_threads, NULL) != 0)
  	die("\nErro ao inicializar variável de condição\n");

 	if(pthread_mutex_init(&barrier_mutex, NULL) != 0)
		die("\nErro ao inicializar mutex\n");

}

void mutex_lock() {

	if(pthread_mutex_lock(&barrier_mutex) != 0)
		die("\nErro ao bloquear mutex\n");
}

void mutex_unlock() {

	if(pthread_mutex_unlock(&barrier_mutex) != 0)
		die("\nErro ao desbloquear mutex\n");

}



/*destroi o mutex e variavel de condicao */
void destroy_mutex_cond() {

	if(pthread_mutex_destroy(&barrier_mutex) != 0)
		die("\nErro ao destruir mutex\n");

	if(pthread_cond_destroy(&wait_for_all_threads) != 0)
		die("\nErro ao destruir variável de condição\n");

}



/* calcula os valores da matrix da linha "from_line" a linha "to_line";
 * tbm verifica se todos os valores são inferiores a maxD e devolve 1 se sim,
 * 0 caso contrário */
int calc_values(DoubleMatrix2D *matrix, DoubleMatrix2D *matrix_aux, int from_line, int to_line,  int size_line, double maxD) {
	int i, j; /*iteradores*/
	int too_small = 1;
	for (i = from_line + 1; i < to_line; i++)
		for(j = 1; j < size_line - 1; j++) {
			/*calculo dos valores segundo o algoritmo do enunciado*/
			dm2dSetEntry(matrix_aux, i, j, (dm2dGetEntry(matrix, i - 1, j) +
dm2dGetEntry(matrix, i, j - 1) + dm2dGetEntry(matrix, i + 1, j) +
dm2dGetEntry(matrix, i, j + 1)) / 4);
		too_small = too_small && (dm2dGetEntry(matrix_aux, i, j) - dm2dGetEntry(matrix, i, j) < maxD);
		}
	return too_small;
}



/*verifica se todos os valores em vec sao 1;
  se nao for o caso, coloca todos os valores a 0*/
int verificar_maxD(int *vec, int n) {
	int i, res = 1;
	for (i = 0; i < n; i++)
		if((res = res && vec[i]) == 0)
			break;
	if(res == 0)
		for (i = 0; i < n; i++)
			vec[i] = res;
	return res;
}



/*barreira que sincroniza as threads */
int barreira_espera_por_todos (Thread_Arg arg, int FULL, int *localFlag, int end) {

	int *threads = getBlockedTrab(arg);
	int *barrierFLAG = getBarrierFlag(arg);
	int *fileFLAG = getFileFlag(arg);
	int *under_maxD_vec = getUnderMaxDVec(arg);
	pid_t *pid = getPid(arg);

	if(pthread_mutex_lock(&barrier_mutex) != 0)
		die("\nErro ao bloquear mutex\n");

	(*threads)++;
	*localFlag = *barrierFLAG; /*assim a condicao dentro do while vai ser verdadeira
											 * ate o ultimo thread mudar a variavel global "FLAG" */



	/* se for a ultima thread a chegar, esta condicao vai ser verdadeira "*threads = FULL"
	 * esta ultima thread vai repor a "FLAG", determinar se e necessario continuar
	 * as iteracoes e acordar as restantes threads */
	if(*threads == FULL ) {
		(*threads) = 0;
		*barrierFLAG = !(*barrierFLAG);
		if(*fileFLAG && (*getPid(arg) == 0 || waitpid(*getPid(arg), NULL, WNOHANG))) {
			*fileFLAG = 0;
			*pid = fork();
			if (*pid == 0)
				return 1;
		}
		if(verificar_maxD(under_maxD_vec, FULL) || end)
			*fileFLAG = -1;
		if(pthread_cond_broadcast(&wait_for_all_threads) != 0)
			die("\nErro ao desbloquear variável de condição\n");
	}

	while(*barrierFLAG == *localFlag)
		if(pthread_cond_wait(&wait_for_all_threads, &barrier_mutex) != 0)
			die("\nErro ao esperar pela variável de condição\n");

	if(pthread_mutex_unlock(&barrier_mutex) != 0)
		die("\nErro ao desbloquear mutex\n");

	return 0;
}







/*funcoes de abstracao*/

int getId(Thread_Arg arg) {return arg->id;}
int getSizeLine(Thread_Arg arg) {return arg->size_line;}
int getNLine(Thread_Arg arg) {return arg->n_line;}
int getIter(Thread_Arg arg) {return arg->iter;}
double getMaxD(Thread_Arg arg) {return arg->maxD;}
int *getBlockedTrab(Thread_Arg arg) {return arg->blocked_trab;}
int *getUnderMaxDVec(Thread_Arg arg) {return arg->under_maxD_vec;}
int *getBarrierFlag(Thread_Arg arg) {return arg->barrierFLAG;}
int *getFileFlag(Thread_Arg arg) {return arg->fileFLAG;}
char *getFilename(Thread_Arg arg) {return arg->filename;}
DoubleMatrix2D *getMatrix(Thread_Arg arg) {return arg->matrix;}
DoubleMatrix2D *getMatrixAux(Thread_Arg arg) {return arg->matrix_aux;}
pid_t *getPid(Thread_Arg arg) {return arg->pid;}

void setMatrix(Thread_Arg arg, DoubleMatrix2D *matrix) {arg->matrix = matrix;}
void setMatrixAux(Thread_Arg arg, DoubleMatrix2D *matrix) {arg->matrix_aux = matrix;}
void setIter(Thread_Arg arg, int iter) {arg->iter = iter;}
void setId(Thread_Arg arg, int id) { arg->id = id;}
void setSizeLine(Thread_Arg arg, int size_line) {arg->size_line = size_line;}
void setNLine(Thread_Arg arg, int n_line) {arg->n_line = n_line;}
void setMaxD(Thread_Arg arg, double maxD) {arg->maxD = maxD;}
void setBlockedTrab(Thread_Arg arg, int *px) {arg->blocked_trab = px;}
void setUnderMaxDVec(Thread_Arg arg, int *px) {arg->under_maxD_vec = px;}
void setBarrierFlag(Thread_Arg arg, int *px) {arg->barrierFLAG = px;}
void setFileFlag(Thread_Arg arg, int *px) {arg->fileFLAG = px;}
void setFilename(Thread_Arg arg, char* filename) {arg->filename = filename;}
void setPid(Thread_Arg arg, pid_t *pid) {arg->pid = pid;}
