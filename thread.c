
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "util.h"
#include "matrix2d.h"
#include "thread.h"



/*mutex e condicao usados na sincronizacao dos threads */
pthread_cond_t 		wait_for_all_threads;
pthread_mutex_t		barrier_mutex;




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

void cond_broadcast() {

	if(pthread_cond_broadcast(&wait_for_all_threads) != 0)
		die("\nErro ao desbloquear variável de condição\n");

}

void cond_wait() {

	if(pthread_cond_wait(&wait_for_all_threads, &barrier_mutex) != 0)
		die("\nErro ao esperar pela variável de condição\n");

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
void verificar_maxD(int *vec, int n) {
	int i, res = 1;
	for (i = 0; i < n; i++)
		if((res = res && vec[i]) == 0)
			break;
	if(res == 0)
		for (i = 0; i < n; i++)
			vec[i] = res;
}



/*barreira que sincroniza as threads */
int barreira_espera_por_todos (Thread_Arg arg, int FULL, int *localFlag) {

	int *threads = getBlockedTrab(arg);
	int *barrierFLAG = getBarrierFlag(arg);
	int *fileFLAG = getFileFlag(arg);
	int *terminateFLAG = getTerminateFlag(arg);
	int *under_maxD_vec = getUnderMaxDVec(arg);
	pid_t *pid = getPid(arg);

	mutex_lock();

	(*threads)++;
	*localFlag = *barrierFLAG; /*assim a condicao dentro do while vai ser verdadeira
											 * ate o ultimo thread mudar a variavel global "FLAG" */

	if(getId(arg) == 0) {
		int end = 0;
		if(*fileFLAG || *terminateFLAG) {
			end = *terminateFLAG;
			if(waitpid(*pid, NULL, WNOHANG)) {
				*pid = fork();
				if (*pid == 0)
					return 1;
			}
			*fileFLAG = 0;
		}
		if(end) {

			sigset_t set;
      sigemptyset(&set);
      sigaddset(&set, SIGALRM);
      sigaddset(&set, SIGINT);
      pthread_sigmask(SIG_BLOCK, &set, NULL);
			
			waitpid(*pid, NULL, 0);
			exit(-1);
		}
	}



	/* se for a ultima thread a chegar, esta condicao vai ser verdadeira "*threads = FULL"
	 * esta ultima thread vai repor a "FLAG", determinar se e necessario continuar
	 * as iteracoes e acordar as restantes threads */
	if(*threads == FULL ) {
		(*threads) = 0;
		*barrierFLAG = !(*barrierFLAG);
		verificar_maxD(under_maxD_vec, FULL);
		cond_broadcast();
	}

	while(*barrierFLAG == *localFlag)
		cond_wait();

	mutex_unlock();

	return 0;
}



/*constructor do argumento da thread, Thread_Arg */
Thread_Arg createThreadArg(int id, int size_line, int n_line, int iter,
double maxD, int *blocked_trab, int *under_maxD_vec, int *barrierFLAG,
int *fileFLAG, int *terminateFLAG, pid_t *pid, const char *filename) {

	Thread_Arg arg = (Thread_Arg) malloc(sizeof(struct thread_arg));
	if (arg == NULL)
		return NULL;

	arg->id = id;
	arg->size_line = size_line;
	arg->n_line = n_line;
	arg->iter = iter;
	arg->maxD = maxD;
	arg->blocked_trab = blocked_trab;
	arg->under_maxD_vec = under_maxD_vec;
	arg->barrierFLAG = barrierFLAG;
	arg->fileFLAG = fileFLAG;
	arg->terminateFLAG = terminateFLAG;
	arg->pid = pid;
	arg->filename = (char*) malloc(strlen(filename) + 1);
	memcpy (arg->filename, filename, strlen(filename) + 1);
	return arg;

}


void freeThreadArg(Thread_Arg arg) {
	free(getFilename(arg));
	free(arg);
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
int *getTerminateFlag(Thread_Arg arg) {return arg->terminateFLAG;}
char *getFilename(Thread_Arg arg) {return arg->filename;}
pid_t *getPid(Thread_Arg arg) {return arg->pid;}
DoubleMatrix2D *getMatrix(Thread_Arg arg) {return arg->matrix;}
DoubleMatrix2D *getMatrixAux(Thread_Arg arg) {return arg->matrix_aux;}

void setMatrix(Thread_Arg arg, DoubleMatrix2D *matrix) {arg->matrix = matrix;}
void setMatrixAux(Thread_Arg arg, DoubleMatrix2D *matrix) {arg->matrix_aux = matrix;}
