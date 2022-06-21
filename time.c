
#include <stdio.h>
#include <time.h>
int main () {
   time_t tim;
   struct tm *detl;
   char buf[80];
   time( &tim );
   detl = localtime( &tim );
   strftime(buf, 20, "%x - %I:%M%p", detl);
   printf("Date & time after formatting : %s", buf );
   return(0);
}