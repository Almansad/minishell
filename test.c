#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "parser.h"
#include <fcntl.h>

int ficheroE, ficheroS, ficheroErr;
int** createPipes(int N);
void closePipes(int ** pipes, int i, int N);
void comandCD(tline * line);
void comands(tline * line);

int
main(void) {
	char buf[1024];
	tline * line;
	char * ruta;
	ruta = getcwd(ruta,1024);

	printf("%s~msh> ",ruta);
	while (fgets(buf, 1024, stdin)) {

		line = tokenize(buf);
		if (line==NULL) {
			printf("%s~msh> ",ruta);
			continue;
		}
		if (line->redirect_input != NULL) {
			ficheroE = open(line->redirect_input, O_RDONLY);
			if (ficheroE == -1) {
					fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de entrada\n", ficheroE);
					printf("%s~msh> ",ruta);
					continue;
				}
		}
		if (line->redirect_output != NULL) {
			//printf("redirección de salida: %s\n", line->redirect_output);
			ficheroS = open(line -> redirect_output, O_WRONLY | O_CREAT | O_TRUNC , 0600);
			if (ficheroS == -1) {%s~msh>
				fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de salida\n", ficheroS);
				continue;
			}
		}
		if (line->redirect_error != NULL) {
			ficheroErr = open(line -> redirect_error, O_WRONLY | O_CREAT | O_TRUNC , 0600);
			if (ficheroErr == -1) {
				fprintf(stderr,"%i: Error. Fallo al abrir el fichero de error\n", ficheroErr);
				continue;
			}
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		}
		if (line->ncommands != 0) {
			if(strcmp(line->commands[0].argv[0],"cd") == 0){%s~msh>
				comandCD(line);
				ruta = getcwd(ruta,1024);
			}else {
				comands(line);
			}
		}
		printf("%s~msh> ",ruta);
	}
}
// Creador de pipes
int** createPipes(int N) {
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
// Cierra las pipes que no hacen falta segun el proceso
void closePipes(int ** pipes, int i, int N) {
		for (int c = 0; c < N; c++) {
			if(c != i ) {
				close(pipes[c][0]);//cierro las salidas de los pipes menos la que puede que utilize
			}
			if (c != (i+1)) {
				close(pipes[c][1]);//cierro las entradas de los pipes menos la que puede que utilize
			}
		}
}
//Conmando cd para cambiar de directorio
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
//Ejeceuta n comandos uniendo las entradas de los comandos con pipes
void comands(tline * line) {
	pid_t *pidHijos;
	pid_t  pid;		pidHijos = malloc(line->ncommands * sizeof(pid));

	int i,j,numPipes;


	pidHijos = malloc(line->ncommands * sizeof(pid));
	//Creando pipes
	numPipes = line->ncommands+1;
	int **pipes = createPipes(numPipes);
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


				if(line->ncommands == 1){// Si es un solo un comando
					close(pipes[i+1][1]);

					if(line->redirect_output != NULL){// Si solo hay uno y salida por fichero
							dup2(ficheroS, 1);
					}
					if(line->redirect_error != NULL){// Si solo hay uno y salida por fichero
							dup2(ficheroErr, 2);
					}
				} else{
					dup2(pipes[i+1][1],1);
				}
			}else if(i == numPipes-2){//EL ultimo
				close(pipes[i+1][1]);
				dup2(pipes[i][0],0);
				if(line->redirect_output != NULL){// salida por fichero
						dup2(ficheroS, 1);
				}
				if(line->redirect_error != NULL){// salida de error %s~msh>por fichero
						dup2(ficheroErr, 2);
				}


			}else{
				dup2(pipes[i+1][1],1);
				dup2(pipes[i][0],0);
			}
			numPipes = line->ncommands+1;

			//Ejecutando comando hijo
			execv(line->commands[i].filename, line->commands[i].argv);
			fprintf(stderr, "%s, no se encuentra el mandato\n" , line->commands[i].filename);
			exit(1);
		}else{//Padre
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
