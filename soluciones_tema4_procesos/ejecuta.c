#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
	pid_t  pid;
	int status;

	if(argc < 2)
	{
		printf("Uso: %s programa argumentos\n", argv[0]);
		return 1;
	}

	pid = fork();
	if (pid < 0) { /* Error */
		fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
		exit(1);
	}
	else if (pid == 0) { /* Proceso Hijo */
		printf("hola: %s",argv[1]);
		execvp(argv[1], argv + 1);
		//Si llega aquí es que se ha producido un error en el execvp
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
		exit(0);
	}
}
