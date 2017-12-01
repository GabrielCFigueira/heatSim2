#ifndef _THREAD_H_
#define _THREAD_H_



typedef struct thread_arg{
	int 				id;
  int 		   	size_line;
 	int 		   	n_line;
	int 				iter;
	double			maxD;
	int					*blocked_trab;
	int					*under_maxD_vec;
	int					*barrierFLAG;
	int 				*fileFLAG;
	char				*filename;
	pid_t				*pid;
	DoubleMatrix2D 		*matrix;
	DoubleMatrix2D 		*matrix_aux;
}*Thread_Arg;



/*funcoes para inicializar e destruir mutex e variavel de condicao */
void init_mutex_cond();
void destroy_mutex_cond();

void mutex_lock();
void mutex_unlock();

void cond_broadcast();
void cond_wait();




int calc_values(DoubleMatrix2D *matrix, DoubleMatrix2D *matrix_aux, int from_line, int to_line,  int size_line, double maxD);
void verificar_maxD(int *vec, int n);
int barreira_espera_por_todos (Thread_Arg arg, int FULL, int *localFlag);



Thread_Arg createThreadArg(int id, int size_line, int n_line, int iter,
double maxD, int *blocked_trab, int *under_maxD_vec, int *barrierFLAG,
int *fileFLAG, pid_t *pid, const char *filename);


void freeThreadArg(Thread_Arg arg);



/*funcoes de abstracao */
int getId(Thread_Arg arg);
int getSizeLine(Thread_Arg arg);
int getNLine(Thread_Arg arg);
int getIter(Thread_Arg arg);
double getMaxD(Thread_Arg arg);
int *getBlockedTrab(Thread_Arg arg);
int *getUnderMaxDVec(Thread_Arg arg);
int *getBarrierFlag(Thread_Arg arg);
int *getFileFlag(Thread_Arg arg);
char *getFilename(Thread_Arg arg);
pid_t *getPid(Thread_Arg arg);
DoubleMatrix2D *getMatrix(Thread_Arg arg);
DoubleMatrix2D *getMatrixAux(Thread_Arg arg);

void setMatrix(Thread_Arg arg, DoubleMatrix2D *matrix);
void setMatrixAux(Thread_Arg arg, DoubleMatrix2D *matrix);

#endif
