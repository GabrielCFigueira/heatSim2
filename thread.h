#ifndef _THREAD_H_
#define _THREAD_H_



typedef struct thread_arg{
	int 						id;
  int 				   	size_line;
 	int 				   	n_line;
	int 						iter;
	double					maxD;
	int 						periodoS;
	int							*blocked_trab;
	int							*under_maxD_vec;
	int							*FLAG;
	char						*filename;
	DoubleMatrix2D 	*matrix;
	DoubleMatrix2D 	*matrix_aux;
}*Thread_Arg;



/*funcao de erro e termino */
void die(char* reason);


/*funcoes para inicializar e destruir mutex e variavel de condicao */
void init_mutex_cond();
void destroy_mutex_cond();



int calc_values(DoubleMatrix2D *matrix, DoubleMatrix2D *matrix_aux, int from_line, int to_line,  int size_line, double maxD);
void verificar_maxD(int *vec, int n);
void barreira_espera_por_todos (Thread_Arg arg, int FULL, int *localFlag);




/*funcoes de abstracao */
int getId(Thread_Arg arg);
int getSizeLine(Thread_Arg arg);
int getNLine(Thread_Arg arg);
int getIter(Thread_Arg arg);
double getMaxD(Thread_Arg arg);
int getPeriodoS(Thread_Arg arg);
int *getBlockedTrab(Thread_Arg arg);
int *getUnderMaxDVec(Thread_Arg arg);
int *getFlag(Thread_Arg arg);
char *getFilename(Thread_Arg arg);
DoubleMatrix2D *getMatrix(Thread_Arg arg);
DoubleMatrix2D *getMatrixAux(Thread_Arg arg);

void setMatrix(Thread_Arg arg, DoubleMatrix2D *matrix);
void setMatrixAux(Thread_Arg arg, DoubleMatrix2D *matrix);
void setIter(Thread_Arg arg, int iter);
void setId(Thread_Arg arg, int id);
void setSizeLine(Thread_Arg arg, int size_line);
void setNLine(Thread_Arg arg, int n_line);
void setMaxD(Thread_Arg arg, double maxD);
void setPeriodoS(Thread_Arg arg, int periodoS);
void setBlockedTrab(Thread_Arg arg, int *px);
void setUnderMaxDVec(Thread_Arg arg, int *px);
void setFlag(Thread_Arg arg, int *px);
void setFilename(Thread_Arg arg, char *filename);



#endif
