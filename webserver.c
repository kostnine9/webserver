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

#define BADFILE "ERROR FILE NAME"

struct sockaddr_in 	server_addr, //сокет для подключения сервера
			client_addr; //сокет для подключения клиента

socklen_t sin_len = sizeof (client_addr);

int fd_server,//Фд сервера 
fd_client;// Фд клиента

char buf[BUFSIZ], 
filename[80], 
filetype[80], 
content[80], 
q_s[80];

int filefd, logfile;

int on = 1, i, j;

struct stat sb;//Состояние файла
struct tm *gtime;//Структура времени
time_t curtime;//Текущее время


// отдельная функция для отправки заголовков и самого файла
void http_head (int fd_client, int filefd, char *filename, char *filetype);


int main (int argc, char **argv) {
    ////Если не удалось открыть сокет
    if ((fd_server = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	printf ("socket error\n");
	exit (1);
    }
    ////Установим флаг so_reuseaddr на файловый дескриптор сервера
    setsockopt (fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (int));
    ////Установим порт 8080
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons (8080);
    ////Если не удалось присвоить сокету имя
    if (bind (fd_server, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
	printf ("bind error\n");
	exit (1);
    }
    ////Если не удалось "прослушать" сокет
    if (listen (fd_server, 10) < 0) {
	printf ("listen error\n");
	exit (1);
    }


    while (1) {
	////Если не удалось принять связь на сокет
	if ((fd_client = accept (fd_server, (struct sockaddr *) &client_addr, &sin_len)) < 0) {
	    printf ("accept error\n");
	    continue;
	}
	//Открыть дескриптор лог файла
	logfile = open ("log.txt", O_WRONLY | O_CREAT, 0666);

	if (!fork ()) {//Потомок
	    close (fd_server);			// читаем запрос от клиента в buf
	    memset (buf, 0, BUFSIZ);
	    read (fd_client, buf, BUFSIZ);
	    printf ("%s\n", buf);
	    write (logfile, buf, strlen (buf));


	    if (buf[5] == ' ') {		// если запрос пустой, то отправляем ls.txt
		if (fork () == 0) {//Внук выполняет команду ls
		    filefd = open ("ls.txt", O_WRONLY | O_CREAT, 0666);
		    dup2 (filefd, 1);
		    execlp ("ls", "ls", NULL);
		}
		//Сын 
		wait (NULL);
		filefd = open ("ls.txt", O_RDONLY, 0666);
		http_head (fd_client, filefd, "ls.txt", "txt");
		close (fd_client);
		close (filefd);

		return 0;
	    }


	    for (i = 5; (buf[i] != ' ') && (buf[i] != '/'); i++);	// выделяем ресурс
	    buf[i] = 0;
	    sprintf (filename, "%s", buf+5);
	    for (i = 5; buf[i] != '.'; i++);				// выделяем тип ресурса
	    i++;
	    for (j = 0; buf[i] != ' '; i++, j++)
		filetype[j] = buf[i];
	    filetype[j] = 0;

	    if (!strcmp (filename, "cgi-bin")) {			// Если программа cgi
		for (i = 13; (buf[i] != '/') && (buf[i] != '?') && (buf[i] != ' '); i++);
		buf[i] = 0;
		sprintf (filename, "cgi-bin/%s", buf+13);
		buf[i] = '?';

		for (i = 0; buf[i] != '?'; i++);
		int j = i;
		for (i = ++j; buf[i] != ' '; i++);
		buf[i] = 0;

		sprintf (q_s, "QUERY_STRING=%s", buf+j);
		putenv (q_s);

		char fname[80];
		sprintf (fname, "%d.txt", getpid ());

		if (!fork ()) {
		    filefd = open (fname, O_WRONLY | O_CREAT, 0666);
		    dup2 (filefd, 1);
		    execlp (filename, filename, NULL);
		}
		wait (NULL);
		filefd = open (fname, O_RDONLY, 0666);
		http_head (fd_client, filefd, fname, "txt");

		close (fd_client);
		close (filefd);

		if (unlink (fname) < 0)
		    printf ("unlink error\n");

		return 0;
	    }


	    if ((filefd = open (filename, O_RDONLY)) < 0) {		// открываем его
		write (fd_client, BADFILE, strlen (BADFILE));
		return 1;
	    }

	    http_head (fd_client, filefd, filename, filetype);		// отправляем клиенту вместе со всеми заголовками
	    close (filefd);
	}
	close (fd_client);
    }

    while (wait (NULL) != -1);//Родитель ожидает окончания потомка
    return 0;
}



void http_head (int fd_client, int filefd, char *filename, char *filetype) {
	    lstat (filename, &sb);
	    write (fd_client, "HTTP/1.1 200 OK\n", 16);

	    curtime = time (NULL);
	    gtime = gmtime (&curtime);
	    strftime (content, 80, "Date: %a, %d %b %Y %X GMT\n", gtime);
	    write (fd_client, content, strlen (content));

	    sprintf (content, "Content-Length: %ld\n", sb.st_size);
	    write (fd_client, content, strlen (content));

	    if (!strcmp (filetype, "txt")) {
		sprintf (content, "Content-Type: text/plain\n");
		write (fd_client, content, strlen (content));
	    }
	    if (!strcmp (filetype, "jpeg") || !strcmp (filetype, "jpg")) {
		sprintf (content, "Content-Type: image/jpeg\n");
		write (fd_client, content, strlen (content));
	    }
	    if (!strcmp (filetype, "png")) {
		sprintf (content, "Content-Type: image/png\n");
		write (fd_client, content, strlen (content));
	    }
	    if (!strcmp (filetype, "html")) {
		sprintf (content, "Content-Type: text/html\n");
		write (fd_client, content, strlen (content));
	    }

	    curtime = sb.st_mtime;
	    gtime = gmtime (&curtime);
	    strftime (content, 80, "Last-Mofified: %a, %d %b %Y %X GMT\n", gtime);
	    write (fd_client, content, strlen (content));

  	    write (fd_client, "\n", 1);
	    int len;
	    while (len = read(filefd, &buf, BUFSIZ))
		if (write (fd_client, buf, len) < 0)
		{
		    printf("error writing socket!");
		    return;
		}


	    close (fd_client);
	    exit (0);
}




