#include <stdio.h>
#include <stdlib.h>

int main(){
	int a, b;
	char *q_s = getenv("QUERY_STRING"), buf[100];
	sscanf(q_s, "a=%d&b=%d",&a,&b);
	printf("<HTML><HEAD><TITLE>Addition</TITLE></HEAD>\n<BODY>\n<H3>Addition</H3>\n<p>%d + %d = %d</p></HTML>", a, b, a+b);
     	return 0;
}	


