#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
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
		under_maxD_vec[getId(arg)] = calc_values(getMatrix(arg),
getMatrixAux(arg), getId(arg) * (getNLine(arg) - 2),
getId(arg) * (getNLine(arg) - 2) + getNLine(arg) - 1, getSizeLine(arg),
getMaxD(arg));


		/*troca dos ponteiros matrix e matrix_aux*/
		tmp = getMatrix(arg);
		setMatrix(arg, getMatrixAux(arg));
		setMatrixAux(arg, tmp);


		if(barreira_espera_por_todos(arg, total_trab, &localFlag, i == getIter(arg) - 1)) {
			char *filename = getFilename(arg);
			char *temporaryFilename = (char*) malloc (2 + strlen(filename));
			temporaryFilename[1 + strlen(filename)] = '\0';
			int i, j, flag = 1;
			for (j = (i = strlen(filename) - 1) + 1; i >= 0; i--, j--) {
				if(flag) {
				 	if (filename[i] == '/') {
						temporaryFilename[j] = '~';
						j--;
						flag = 0;
					}
					else if(i == 0) {
						temporaryFilename[j--] = filename[i];
						temporaryFilename[j] = '~';
						flag = 0;
						break;
					}
				}
				temporaryFilename[j] = filename[i];
			}
			dm2dPrintToFile(getMatrix(arg), temporaryFilename);
			rename(temporaryFilename, filename);
			free(temporaryFilename);

			exit(0);
		}


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


	const int N = parse_integer_or_exit(argv[1], "N");
	const double tEsq = parse_double_or_exit(argv[2], "tEsq");
	const double tSup = parse_double_or_exit(argv[3], "tSup");
	const double tDir = parse_double_or_exit(argv[4], "tDir");
	const double tInf = parse_double_or_exit(argv[5], "tInf");
	const int iter = parse_integer_or_exit(argv[6], "iter");
	const int trab = parse_integer_or_exit(argv[7], "trab");
	const double maxD = parse_double_or_exit(argv[8], "maxD");
	const char *fichS = argv[9];
	const double periodoS = parse_double_or_exit(argv[10], "periodoS");



	if (N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 ||
trab < 1 || N % trab != 0 || maxD < 0|| periodoS < 0)
 		die("\nArgumentos invalidos\n");


	fprintf(stderr, "\nArgumentos:\nN=%d tEsq=%.1f tSup=%.1f tDir=%.1f"
" tInf=%.1f iteracoes=%d\nthreads=%d maxD=%.1f fichS=%s periodoS=%.1f\n",
N, tEsq, tSup, tDir, tInf, iter, trab, maxD, fichS, periodoS);


	DoubleMatrix2D *matrix = NULL;
	DoubleMatrix2D *matrix_aux = NULL;


	FILE *fp = fopen(fichS, "r");


	if(fp != NULL) {

		matrix = readMatrix2dFromFile(fp, N + 2, N + 2);
		matrix_aux = dm2dNew(N+2, N+2);

		if(matrix_aux == NULL)
			die("\nErro ao criar as matrizes\n");

		if(fclose(fp) != 0)
			fprintf(stderr, "\nErro ao fechar o ficheiro %s\n", fichS);
		if (matrix == NULL)
			free(matrix_aux);
		else
			dm2dCopy(matrix_aux, matrix);
	}


	if(matrix == NULL) {


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




	/*alocacao dos threads, seus argumentos e um buffer para a main thread*/
	pthread_t *threads = (pthread_t*) malloc(trab *  sizeof(pthread_t));
	int *under_maxD_vec = (int*) malloc (sizeof(int) * trab);


	if (threads == NULL ||  under_maxD_vec == NULL)
		die("\nErro ao alocar memoria para os threads\n");


	/*inicializacao do mutex e variavel de condicao */
	init_mutex_cond();



	int blocked_trab = 0;
	int barrierFLAG = 1;
	int fileFLAG = 0;
	pid_t pid = 0;


	Thread_Arg arguments[trab];
	int i; /*iterador*/
	for (i = 0; i < trab; i++) {

		Thread_Arg arg = createThreadArg(i, N + 2, (N / trab) + 2, iter, maxD,
&blocked_trab, under_maxD_vec, &barrierFLAG, &fileFLAG, &pid, fichS);

		arguments[i] = arg;

		if(arg == NULL)
			die("\nErro ao alocar memoria para as threads\n");

		setMatrix(arg, matrix);
		setMatrixAux(arg, matrix_aux);

		under_maxD_vec[i] = 1;

		if (pthread_create(&threads[i], NULL, theThread, arg) != 0)
      die("\nErro ao criar uma thread.\n");

  }


	while(1) {
		sleep(periodoS);
		mutex_lock();
		if(fileFLAG == -1) {
			mutex_unlock();
			break;
		}
		fileFLAG = 1;
		mutex_unlock();
	}



	for (i = 0; i < trab; i++)
    if (pthread_join(threads[i], NULL) != 0)
    	die("\nErro ao esperar por uma thread\n");


	destroy_mutex_cond();


	free(under_maxD_vec);
	dm2dPrint(getMatrix(arguments[0]));
	waitpid(pid, NULL, 0);
	if(pid != 0 || fp != NULL) {
		if(unlink(fichS) != 0)
			fprintf(stderr, "\nErro ao eliminar o ficheiro de salvaguarda\n");
	}

	free(threads);
	dm2dFree(matrix);
	dm2dFree(matrix_aux);
	for(i = 0; i < trab; i++)
		freeThreadArg(arguments[i]);


	return 0;
}
