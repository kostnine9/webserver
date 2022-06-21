#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>


// http://localhost:8000
//www.msu.kz/

//при запуске сервер работа с зомби 1 раз
//wait сервер рабоать должен параллельно!!!! 
//огромный файл 
//чиать головин 2 этап: запустить на стороне сервере прогу. получ рез=ты и отправляет нам.Только вывод программы.

#define MAX_CONNECTION 5  
#define WRONG_FILE "ERROR 404 NOT FOUND"


struct sockaddr_in 	server_addr,client_addr; 
socklen_t sin_size = sizeof (client_addr);

int server_d, client_d;

char buffer[1024], filename[256], filetype[100], content[100];

int filefd, testfile;
int opt = 1, i, j;


struct stat st; //Состояние файла


//функция для отправки заголовков и самого файла
void http_head (int client_d, int filefd, char *filename, char *filetype);


int main (int argc, char **argv) {
    
    if ((server_d = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf (stderr,"error create socket\n");
	close(server_d);
	exit (1);
    }
     printf("SERVER is created!\n");

     if (setsockopt(server_d, SOL_SOCKET, SO_REUSEADDR, &opt , sizeof (int)) == -1)
    {
      fprintf(stderr, "error setsockopt\n");
      close(server_d);
      return -1;
    }
   
    server_addr.sin_family = AF_INET;
    server_addr.sin_family = AF_UNSPEC;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons (8000);

   
   
    if (bind (server_d, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
	fprintf (stderr, "bind error\n");
	close(server_d);
	exit (1);
    }
    
   
    if(listen(server_d, MAX_CONNECTION) == -1) //макс кон=5
  	{
    fprintf(stderr, "error listen\n");
    return -2;
  	}


    while (1) { //написать сразу все перехваты зомби. рочкинд
	if ((client_d = accept (server_d, (struct sockaddr *) &client_addr, &sin_size)) < 0) {
	    fprintf (stderr, "accept error\n");
	    continue;
	}
	testfile = open ("test.txt", O_WRONLY | O_CREAT, 0666);
	//работ журнала сделать

	if (!fork ()) {  //Потомок
	    close (server_d);			// читаем запрос от клиента в buf
	    memset (buffer, 0, 1024);  //пустой буфер	
	    read (client_d, buffer, 1024);
	    printf ("%s\n", buffer);
	    write (testfile, buffer, strlen (buffer));


	    if (buffer[5] == ' ') {		// если запрос пустой, то отправляем ls.txt
		if (fork () == 0) {//Внук выполняет команду ls
		    filefd = open ("ls.txt", O_WRONLY | O_CREAT, 0666);
		    dup2 (filefd, 1);
		    execlp ("ls", "ls", NULL);
		}
		//Сын 
		wait (NULL);
		filefd = open ("ls.txt", O_RDONLY, 0666);
		http_head (client_d, filefd, "ls.txt", "txt");
		close (client_d);
		close (filefd);

		return 0; //один раз сделать фок в начале
	    }


	    for (i = 5; (buffer[i] != ' ') && (buffer[i] != '/'); i++);	// выделяем ресурс
	    buffer[i] = 0;
	    sprintf (filename, "%s", buffer+5);

	    for (i = 5; buffer[i] != '.'; i++);				// выделяем тип ресурса
	    i++;

	    for (j = 0; buffer[i] != ' '; i++, j++)
		filetype[j] = buffer[i];
	    filetype[j] = 0;

	   

	    if ((filefd = open (filename, O_RDONLY)) < 0) {		// открываем его
		write (client_d, WRONG_FILE, strlen (WRONG_FILE));
		return 1;
	    }

	    http_head (client_d, filefd, filename, filetype);		// отправляем клиенту вместе со всеми заголовками
	    close (filefd);
	}
	close (client_d);
    }

    while (wait (NULL) != -1);//Родитель ожидает окончания потомка
    return 0;
}



void http_head (int client_d, int filefd, char *filename, char *filetype) {
	    
	    if (lstat(filename, &st) < 0) {
            perror(filename);
            putchar('\n');
        }

        time_t tim; //текущее время
   		struct tm *detl; //структура времени
   		time( &tim );
   		detl = localtime( &tim );
   		strftime(buffer, 20, "%x - %I:%M%p", detl);
   		printf("Date & time : %s", buffer );

	    write (client_d, "HTTP/1.1 200 OK\n", 16);

	    
	    write (client_d, content, strlen (content));

//обавить хтмл каталог
	    sprintf (content, "Content-Length: %ld\n", st.st_size);
	    write (client_d, content, strlen (content));

		if (!strcmp (filetype, "jpeg") || !strcmp (filetype, "jpg")) {
		sprintf (content, "Content-Type: image/jpeg\n");
		write (client_d, content, strlen (content));
	    }

	    if (!strcmp (filetype, "txt")) {
		sprintf (content, "Content-Type: text/plain\n");
		write (client_d, content, strlen (content));
	    }
	    
	    if (!strcmp (filetype, "png")) {
		sprintf (content, "Content-Type: image/png\n");
		write (client_d, content, strlen (content));
	    }

	    if (!strcmp (filetype, "gif")) {
		sprintf (content, "Content-Type: image/gif\n");
		write (client_d, content, strlen (content));
	    }

	    if (!strcmp (filetype, "html")) {
		sprintf (content, "Content-Type: text/html\n");
		write (client_d, content, strlen (content));
	    }

	   
	    write (client_d, content, strlen (content));

  	    write (client_d, "\n", 1);

	    int size;
	    while (size = read(filefd, &buffer, 1024))
		if (write (client_d, buffer, size) < 0)
		{
		    printf("error writing socket!");
		    return;
		}


	    close (client_d);
	    exit (0);
}
