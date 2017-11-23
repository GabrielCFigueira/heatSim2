#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "matrix2d.h"
#include "thread.h"


/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
	int value;

	if(sscanf(str, "%d", &value) != 1) {
		fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
		 exit(1);
  	}
  	return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
	double value;

	if(sscanf(str, "%lf", &value) != 1) {
		fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    		exit(1);
  	}
  	return value;
}


/*--------------------------------------------------------------------
| Function: theThread
---------------------------------------------------------------------*/

void* theThread(void * a) {

	Thread_Arg arg = (Thread_Arg) a;
	DoubleMatrix2D* tmp;


	/*numero total de threads*/
	int total_trab = (getSizeLine(arg) - 2) / (getNLine(arg) - 2);

	int i; /*iterador*/

	/*flag local 'a thread usada na barreira para sincronizar todas as threads */
	int localFlag;
	int *under_maxD_vec = getUnderMaxDVec(arg);



	for (i = 0; i < getIter(arg); i++) {
		under_maxD_vec[getId(arg)] = calc_values(getMatrix(arg), getMatrixAux(arg), getId(arg) * (getNLine(arg) - 2), getId(arg) * (getNLine(arg) - 2) + getNLine(arg) - 1, getSizeLine(arg), getMaxD(arg));

		/*troca dos ponteiros matrix e matrix_aux*/
		tmp = getMatrix(arg);
		setMatrix(arg, getMatrixAux(arg));
		setMatrixAux(arg, tmp);

		barreira_espera_por_todos(getBlockedTrab(arg), total_trab, under_maxD_vec, &localFlag, getFlag(arg));


		if(under_maxD_vec[getId(arg)])
			break;

	}


	return 0;
}










int main (int argc, char** argv) {

	if (argc != 11) {
    fprintf(stderr, "Utilizacao: ./heatSim N tEsq tSup"
"tDir tInf iter trab maxD fichS periodoS\n\n");
    die("Numero de argumentos invalido");
  }


	int N = parse_integer_or_exit(argv[1], "N");
	double tEsq = parse_double_or_exit(argv[2], "tEsq");
	double tSup = parse_double_or_exit(argv[3], "tSup");
	double tDir = parse_double_or_exit(argv[4], "tDir");
	double tInf = parse_double_or_exit(argv[5], "tInf");
	int iter = parse_integer_or_exit(argv[6], "iter");
	int trab = parse_integer_or_exit(argv[7], "trab");
	double maxD = parse_double_or_exit(argv[8], "maxD");
	int periodoS = parse_integer_or_exit(argv[10], "periodoS");

	const char *fichS = argv[9];
	if(fichS == NULL)
		die("\nNome de ficheiro invalido\n");


	if (N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 ||
trab < 1 || N % trab != 0 || maxD < 0|| periodoS < 0)
 		die("\nArgumentos invalidos\n");


/*	fprintf(stderr, "\nArgumentos:\nN=%d tEsq=%.1f tSup=%.1f tDir=%.1f"
" tInf=%.1f iteracoes=%d\nthreads=%d maxD=%.1f fichS=%s periodoS=%d\n",
N, tEsq, tSup, tDir, tInf, iter, trab, maxD, fichS, periodoS); */


	DoubleMatrix2D *matrix;
	DoubleMatrix2D *matrix_aux;
	FILE *fp = fopen(fichS, "r");
	if(fp == NULL){


		matrix = dm2dNew(N+2, N+2);
 		matrix_aux = dm2dNew(N+2, N+2);

		if (matrix == NULL || matrix_aux == NULL)
			die("\nErro ao criar as matrizes\n");


		/*valores iniciais da matrix*/
		dm2dSetLineTo (matrix, 0, tSup);
		dm2dSetLineTo (matrix, N+1, tInf);
		dm2dSetColumnTo (matrix, 0, tEsq);
		dm2dSetColumnTo (matrix, N+1, tDir);
		dm2dSetLineTo (matrix_aux, 0, tSup);
		dm2dSetLineTo (matrix_aux, N+1, tInf);
		dm2dSetColumnTo (matrix_aux, 0, tEsq);
		dm2dSetColumnTo (matrix_aux, N+1, tDir);
		
	}
	else {
		matrix = readMatrix2dFromFile(fp, N + 2, N + 2);
		matrix_aux = dm2dNew(N+2, N+2);
		dm2dCopy(matrix_aux, matrix);
		if(fclose(fp) != 0)
			fprintf(stderr, "\nErro ao fechar o ficheiro %s\n", fichS);
		if (matrix == NULL || matrix_aux == NULL)
			die("\nErro ao ler as matrizes do ficheiro\n");
	}




	/*alocacao dos threads, seus argumentos e um buffer para a main thread*/
	pthread_t *threads = (pthread_t*) malloc(trab *  sizeof(pthread_t));
	Thread_Arg arguments = (Thread_Arg) malloc(trab * sizeof(struct thread_arg));
	int *under_maxD_vec = (int*) malloc (sizeof(int) * trab);


	if (threads == NULL || arguments == NULL || under_maxD_vec == NULL)
		die("\nErro ao alocar memoria para os threads\n");


	/*inicializacao do mutex e variavel de condicao */
	init_mutex_cond();



	int blocked_trab = 0;
	int FLAG = 1;

	int i; /*iterador*/
	for (i = 0; i < trab; i++) {
		setId(&arguments[i], i);
		setSizeLine(&arguments[i], N + 2);
		setNLine(&arguments[i], N / trab + 2);
		setIter(&arguments[i], iter);
		setMaxD(&arguments[i], maxD);
		setMatrix(&arguments[i], matrix);
		setMatrixAux(&arguments[i], matrix_aux);
		setBlockedTrab(&arguments[i], &blocked_trab);
		under_maxD_vec[i] = 1;
		setUnderMaxDVec(&arguments[i], under_maxD_vec);
		setFlag(&arguments[i], &FLAG);
		if (pthread_create(&threads[i], NULL, theThread, &arguments[i]) != 0)
      die("\nErro ao criar uma thread.\n");
  }


	for (i = 0; i < trab; i++)
    if (pthread_join(threads[i], NULL) != 0)
    	die("\nErro ao esperar por uma thread\n");


	destroy_mutex_cond();


	free(under_maxD_vec);
	dm2dPrint(getMatrix(arguments));
	free(threads);
	dm2dFree(matrix);
	dm2dFree(matrix_aux);
	free(arguments);


	return 0;
}
