#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char **argv)
{
    pid_t pid;
    int p_padre[2]; //Tubería en la que escribe el padre
    int p_hijo[2];  //Tubería en la que escribe el hijo
    char buf[1024];
    
    FILE *p_p, *p_h;  //variables para manejar las tuberías con fputs y fgets
    
    pipe(p_padre);
    pipe(p_hijo);
    
    pid = fork();
    if (pid < 0) { /* Error */
		fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
		exit(1);
    } else if (pid == 0) { //Hijo 1
    	//Cierro los extremos de las tuberías que no usa el hijo
    	close(p_padre[1]);
    	close(p_hijo[0]);
    	p_p = fdopen(p_padre[0], "r");  
    	p_h = fdopen(p_hijo[1], "w");
    	
    	while( fgets(buf, 1024, p_p) != NULL )
    	{
    		int longitud = strlen(buf);
    		int i;
    		for(i = 0; i < longitud; i++) {
    			buf[i] = toupper(buf[i]);
		}
		fputs(buf, p_h);
		fflush(p_h);
	}
    }
    else
    {
	//Cierro los extremos de las tuberías que no usa el padre
    	close(p_padre[0]);
    	close(p_hijo[1]);
    	p_p = fdopen(p_padre[1], "w");
    	p_h = fdopen(p_hijo[0], "r");
    	
    	while( fgets(buf, 1024, stdin) != NULL )
    	{
    		fputs(buf, p_p);
    		fflush(p_p);
    		fgets(buf, 1024, p_h);
    		fputs(buf, stdout);    	
    	}
    
    }
    
    return 0;
    
}

