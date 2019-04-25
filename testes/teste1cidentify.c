#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {

	int	id0;
	int tam = 57;
	char *name = (char *)malloc(tam * sizeof(char));

	id0 = cidentify(name, tam);

	printf("Result = %d\n%s\n", id0, name);
}
