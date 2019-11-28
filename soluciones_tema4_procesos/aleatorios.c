#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

int p[2];  //Para almacenar los descriptores de la tubería


//Manejador de señales del hijo
//SIGUSR1 => generar un numero aleatorio y ponerlo en la tubería
//SIGUSR2 => terminar el proceso hijo
void manejador_hijo(int sig)
{
	int num;
	
	if(sig == SIGUSR2)
		exit(0);
	else if(sig == SIGUSR1)
	{		
	  //num = rand() % 10;
		num = 1+(int) (10.0*rand()/(RAND_MAX+1.0));
		write(p[1], &num, sizeof(int));
	}
}

int main()
{
	pid_t pid;
	
	char buf[1024];
	int num;
	
	
	//Activo el manejador del hijo para SIGUSR1 y SIGUSR2
	signal(SIGUSR1, manejador_hijo);
	signal(SIGUSR2, manejador_hijo);
			
	pipe(p);
	srand(time(NULL));  //Inciar el generador de numeros aleatorios con una nueva semilla
		
	pid = fork();
	
	if(pid == 0)
	{
		//Proceso hijo
		close(p[0]); //Cierro el extremo de lectura de la tubería
		while(1);  //Bucle infinito. El hijo solo hará algo cuando le llegue una señal
	}
	else
	{
		//Proceso padre
	        close(p[1]); // cierro el extremo de escritura en el pipe
		while(fgets(buf,1024,stdin) != NULL)
		{			
			//Cada vez que se lee una linea por el teclado se envia una señal al hijo
			//Y se lee de la tubería
			kill(pid, SIGUSR1);			
			read(p[0], &num, sizeof(int));
			printf("%d\n", num);
		}
		
		//Cuando se recibe un EOF (Ctrl+D por el teclado), se sale del bucle y se indica al hijo que termine
		kill(pid, SIGUSR2);
		wait(NULL);

		exit(0);
	}
}
