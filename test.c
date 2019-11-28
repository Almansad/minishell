#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "parser.h"
#include <fcntl.h>

int ejecuta(char* command, char **argv);
int ficheroE;
int
main(void) {
	char buf[1024];
	tline * line;
	int i,j;
	printf("==> ");
	while (fgets(buf, 1024, stdin)) {
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}
		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
			ficheroE = open(line->redirect_input, O_RDONLY);
		}
		if (line->redirect_output != NULL) {
			printf("redirección de salida: %s\n", line->redirect_output);
		}
		if (line->redirect_error != NULL) {
			printf("redirección de error: %s\n", line->redirect_error);
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		}
		for (i=0; i<line->ncommands; i++) {
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			for (j=0; j<line->commands[i].argc; j++) {
				printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
			}
			ejecuta(line->commands[i].filename, line->commands[i].argv);
		}
		printf("==> ");
	}
	return 0;
}
int ejecuta(char* command, char **argv){
	pid_t  pid;
	int status;

	pid = fork();
	if (pid < 0) { /* Error */
		fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
		exit(1);
	}
	else if (pid == 0) { /* Proceso Hijo */
		if (ficheroE == -1) {
			fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de entrada\n", ficheroE);
			exit(1);
		} else {
			dup2(ficheroE,0);
		}
		execv(command, argv);
		printf("Error al ejecutar el comando: %s\n", strerror(errno));
		exit(1);

	}
	else { /* Proceso Padre.
				- WIFEXITED(estadoHijo) es 0 si el hijo ha terminado de una manera anormal (caida, matado con un kill, etc).
		Distinto de 0 si ha terminado porque ha hecho una llamada a la función exit()
				- WEXITSTATUS(estadoHijo) devuelve el valor que ha pasado el hijo a la función exit(), siempre y cuando la
		macro anterior indique que la salida ha sido por una llamada a exit(). */
		wait (&status);
		if (WIFEXITED(status) != 0)
			if (WEXITSTATUS(status) != 0)
				printf("El comando no se ejecutó correctamente\n");
	}
	return 0;
}
