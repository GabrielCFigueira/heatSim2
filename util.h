#ifndef _UTIL_H_
#define _UTIL_H_


/*funcao de erro e termino */
void die(char* reason);


/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/
int parse_integer_or_exit(char const *str, char const *name);


/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/
double parse_double_or_exit(char const *str, char const *name);



#endif
