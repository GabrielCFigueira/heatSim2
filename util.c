#include <stdio.h>
#include <stdlib.h>
#include "util.h"



/*funcao de erro e termino */
void die(char* reason) {
	fprintf(stderr, "%s\n", reason);
	exit(-1);
}


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
