#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "parser.h"
#include <fcntl.h>

int ficheroE;
int** creatPipes(int N);
void closePipes(int ** pipes, int i, int N);
void comandCD(tline * line);
void comands(tline * line);

int
main(void) {
	char buf[1024];
	tline * line;
	char * ruta;
	ruta = getcwd(ruta,1024);

	printf("%s==> ",ruta);
	while (fgets(buf, 1024, stdin)) {

		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}
		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
			ficheroE = open(line->redirect_input, O_RDONLY);
			if (ficheroE == -1) {
					fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de entrada\n", ficheroE);
					continue;
				}
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
		if (line->ncommands != 0) {
			if(strcmp(line->commands[0].argv[0],"cd") == 0){
				comandCD(line);
				ruta = getcwd(ruta,1024);
			}else {
				comands(line);
			}
		}
		printf("%s==> ",ruta);
	}
}
int** creatPipes(int N) {
	int **pipes;
	pipes = (int **) malloc((N) * sizeof(int *));
	for(int p=0; p<N; p++) {
		pipes[p] = (int *) malloc (2*sizeof(int));
		if(pipe(pipes[p]) < 0) {
			fprintf(stderr, "ERROR,No se pudo crear las pipes %s/n" , strerror(errno));
		}
	}
	return pipes;
}
void closePipes(int ** pipes, int i, int N) {
		for (int c = 0; c < N; c++) {
			if(c != i ) {
				close(pipes[c][0]);//cierro las salidas de los pipes menos la que puede que utilize
				// printf("%d-Cierro la pipe %d =>0\n",i,c);
			}
			if (c != (i+1)) {
				close(pipes[c][1]);//cierro las entradas de los pipes menos la que puede que utilize
				// printf("%d-Cierro la pipe %d =>1\n",i,c);
			}
		}
}
void comandCD(tline * line){
		if (line->commands[0].argv[1] == NULL) { //y no tiene argumentos
			chdir(getenv("HOME")); //nos lleva a HOME
		}

		else {
			int error = chdir(line->commands[0].argv[1]); //si tiene argumentos nos lleva al destino solicitado

			if (error == -1) { //a menos que no exista
				printf("Lo siento pero la ruta no existe: %s \n", line->commands[0].argv[1]);
			}
		}
}
void comands(tline * line) {
	pid_t *pidHijos;
	pid_t  pid;
	int i,j,numPipes;


	pidHijos = malloc(line->ncommands * sizeof(pid));
	//Creando pipes
	numPipes = line->ncommands+1;
	int **pipes = creatPipes(numPipes);
	for (i=0; i<line->ncommands; i++) {
		//Creando procesos
		pid = fork();
		if (pid < 0) {
			printf("Algo ha ido mal al crear el hijo numero %dº\n",i);
		}else if(pid == 0){//hijo
			// printf("Soy el hijo numero-%d\n",i);
			closePipes(pipes, i,numPipes);

			if(i == 0) {//EL primero
				// printf("%d-Cierro la pipe 0 =>0\n",i);
				close(pipes[0][0]);

				if(line->redirect_input != NULL) {// Entrada por fichero
							dup2(ficheroE,0);
				}


				if(line->ncommands == 1){
					close(pipes[i+1][1]);
				}else{
					dup2(pipes[i+1][1],1);
				}
			}else if(i == numPipes-2){//EL ultimo
				// printf("%d-Cierro la pipe %d =>1\n",i,i+1);
				close(pipes[i+1][1]);

				dup2(pipes[i][0],0);
			}else{
				dup2(pipes[i+1][1],1);
				dup2(pipes[i][0],0);
			}

			//Ejecutando comando hijo
			execv(line->commands[i].filename, line->commands[i].argv);
			fprintf(stderr, "ERROR,No se pudo ejecutar el comando %s\n" , strerror(errno));
			exit(1);
		}else{//Padre
			// printf("Soy el padre voy a esperar al hijo %d\n",pid);
			pidHijos[i] = pid;
		}
	}
	for(int f=0; f<numPipes; f++){ //Cerramos todos los pipes del padre
		close(pipes[f][1]);
		close(pipes[f][0]);
	}
	// printf("voy a esperar a estos chavales\n");
	for(int n=0; n<line->ncommands; n++){// EL padre se queda esperando por cada hijo
		waitpid(pidHijos[n],NULL,0);
	}
	for(int s=0; s<numPipes; s++){ //Liberamos memoria
		free(pipes[s]);
	}
	free(pipes);
	free(pidHijos);
}
