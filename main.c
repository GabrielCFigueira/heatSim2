#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "util.h"
#include "matrix2d.h"
#include "thread.h"




/*flag que indica quando lançar um processo filho para escrever para ficheiro */
int fileFLAG;

/*intervalo de tempo entre cada salvaguarda*/
int periodoS;

/*flag que indica que é necessário salvaguardar a matrix e terminar o processo*/
int terminateFLAG;



/*funcao que trata o sinal ctrl+c (SIGINT) */
void sigintHandler() {
  terminateFLAG = 1;
}

/*funcao que trata o sinal SIGALRM */
void sigalrmHandler() {
  alarm(periodoS);
  fileFLAG = 1;
}






/*--------------------------------------------------------------------
| Function: theThread
---------------------------------------------------------------------*/

void* theThread(void * a) {


	Thread_Arg arg = (Thread_Arg) a;
	DoubleMatrix2D* tmp;


  /*a thread 0 vai ser a thread que trata as interrupções SIGINT e SIGALRM.
  Isto permite o nao uso de mutex dentro dos handlers visto que esta thread
  vai ser a unica a mexer nas flags que decidem a escrita da matrix para
  ficheiro*/
  /*aqui tambem e' definido como tratar o SIGALRM */
  if(getId(arg) == 0) {

    sigset_t set;

    if(sigemptyset(&set) == -1) die("\nErro ao definir a mascara\n");
    if(sigaddset(&set, SIGALRM) == -1) die("\nErro ao definir a mascara\n");
    if(sigaddset(&set, SIGINT) == -1) die("\nErro ao definir a mascara\n");
    if(pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0)
       die("\nErro ao definir a mascara\n");


    struct sigaction act;


    memset(&act, '\0', sizeof act);
    act.sa_handler = sigalrmHandler;
  	if(sigaction(SIGALRM, &act, NULL) == -1)
      die("\nErro ao definir a funcao de tratamento do sinal SIGALRM\n");
  }


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


    /*se for o processo filho a funcao retorna 1 e ele comeca a escrever a
    matrix para um ficheiro */
		if(barreira_espera_por_todos(arg, total_trab, &localFlag)) {

			char *filename = getFilename(arg);


      /*criacao do nome do ficheiro temporario, com o '~' no fim */
			char *temporaryFilename = (char*) malloc (2 + strlen(filename));
			temporaryFilename[1 + strlen(filename)] = '\0';
      temporaryFilename[strlen(filename)] = '~';
			memcpy((char*) temporaryFilename, filename, strlen(filename));


			dm2dPrintToFile(getMatrixAux(arg), temporaryFilename);

      /*se a escrita tiver sucesso, entao muda o nome do ficheiro para o
      suposto */
			if(rename(temporaryFilename, filename) == -1)
        die("\nErro ao mudar o nome do ficheiro\n");
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
	periodoS = parse_integer_or_exit(argv[10], "periodoS");



	if (N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 ||
trab < 1 || N % trab != 0 || maxD < 0|| periodoS < 0)
 		die("\nArgumentos invalidos\n");


	fprintf(stderr, "\nArgumentos:\nN=%d tEsq=%.1f tSup=%.1f tDir=%.1f"
" tInf=%.1f iteracoes=%d\nthreads=%d maxD=%lf fichS=%s periodoS=%d\n",
N, tEsq, tSup, tDir, tInf, iter, trab, maxD, fichS, periodoS);


	DoubleMatrix2D *matrix = NULL;
	DoubleMatrix2D *matrix_aux = NULL;


	FILE *fp = fopen(fichS, "r");

  /* se officheiro existir e for possivel abrir, entao o programa le a
  matrix do ficheiro e comeca as iteracoes a partir dessa matrix */
	if(fp != NULL) {

		matrix = readMatrix2dFromFile(fp, N + 2, N + 2);
		matrix_aux = dm2dNew(N+2, N+2);

		if(matrix_aux == NULL)
			die("\nErro ao criar as matrizes\n");

		if(fclose(fp) != 0)
			fprintf(stderr, "\nErro ao fechar o ficheiro %s\n", fichS);

    /* se ler do ficheiro der erro, a matrix vai ter valor NULL e entao
    o programa deve criar matrizes de inicio */
		if (matrix == NULL)
			free(matrix_aux);
		else
			dm2dCopy(matrix_aux, matrix);
	}


  /*se nao existia ficheiro ou ler do ficheiro deu erro, entao o programa
  comeca normalmente */
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



	/*alocacao dos threads e um buffer para a main thread*/
	pthread_t *threads = (pthread_t*) malloc(trab *  sizeof(pthread_t));
	int *under_maxD_vec = (int*) malloc (sizeof(int) * trab);


	if (threads == NULL ||  under_maxD_vec == NULL)
		die("\nErro ao alocar memoria para os threads\n");


	/*inicializacao do mutex e variavel de condicao */
	init_mutex_cond();


/*inicializacao das flags e outras variaveis globais*/

  /*numero de threads bloqueadas na barreira */
	int blocked_trab = 0;

  /*flag para sincronizar as threads na barreira */
	int barrierFLAG = 1;

  /*id do ultimo processo filho criado. É inicialmente posto a 0 */
  pid_t pid = 0;

  /*ver inicio deste ficheiro */
	fileFLAG = 0;
  terminateFLAG = 0;



	/*vector de ponteiros para todos os argumentos das threads*/
	Thread_Arg arguments[trab];


  /* associacao da rotina 'sigintHandler' 'a interrupcao SIGINT */
  struct sigaction act;
  memset(&act, '\0', sizeof act);
  act.sa_handler = sigintHandler;
  if(sigaction(SIGINT, &act, NULL) == -1)
     die("\nErro ao definir a funcao de tratamento do sinal SIGALRM\n");


  /* antes de criar as threads, define-se que as interrupcoes SIGINT e SIGALRM
  serão bloqueadas. Assim, as threads criadas herdam esta definicao, sendo
  possivel depois controlar que thread e' que trata as interrupcoes */
  sigset_t set;

  if(sigemptyset(&set) == -1) die("\nErro ao definir a mascara\n");
  if(sigaddset(&set, SIGALRM) == -1) die("\nErro ao definir a mascara\n");
  if(sigaddset(&set, SIGINT) == -1) die("\nErro ao definir a mascara\n");
  if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
     die("\nErro ao definir a mascara\n");




	int i; /*iterador*/
	for (i = 0; i < trab; i++) {

    /*criacao das threads e seus argumentos */

		Thread_Arg arg = createThreadArg(i, N + 2, (N / trab) + 2, iter, maxD,
&blocked_trab, under_maxD_vec, &barrierFLAG, &fileFLAG, &terminateFLAG,
&pid, fichS);

		arguments[i] = arg;

		if(arg == NULL)
			die("\nErro ao alocar memoria para as threads\n");

		setMatrix(arg, matrix);
		setMatrixAux(arg, matrix_aux);

		under_maxD_vec[i] = 1;

		if (pthread_create(&threads[i], NULL, theThread, arg) != 0)
      die("\nErro ao criar uma thread.\n");

  }

  /*inicializacao do alarm */
  alarm(periodoS);




	for (i = 0; i < trab; i++)
    if (pthread_join(threads[i], NULL) != 0)
    	die("\nErro ao esperar por uma thread\n");


	destroy_mutex_cond();

  /*impressao da matrix final resultante */
	dm2dPrint(getMatrix(arguments[0]));

  /*se houver ainda algum processo filho em execucao, e necessario esperar
  por ele antes de eliminar o ficheiro de salvaguarda */
	if(waitpid(pid, NULL, 0) == -1)
    fprintf(stderr, "\nErro ao esperar pelo processo filho\n");

  /*condicao que determina se ha algum ficheiro para eliminar */
	if(pid != 0 || fp != NULL) {
		if(unlink(fichS) != 0)
			fprintf(stderr, "\nErro ao eliminar o ficheiro de salvaguarda\n");
	}


  /*libertar toda a memoria ocupada e sair*/
  free(under_maxD_vec);
	free(threads);
	dm2dFree(matrix);
	dm2dFree(matrix_aux);
	for(i = 0; i < trab; i++)
		freeThreadArg(arguments[i]);


	return 0;
}
