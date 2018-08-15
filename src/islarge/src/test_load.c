/*
 * IBM Internal Use Only           T J C Ward       28 September 1993
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */
#include <table_create.h>
#include <stdio.h>
int main(void) {
   PAC_TABLE_WRITER c ;
   int r0, r1, r2 ;
   int key = 100 ;
   char data [ 100 ] ;
   int count ;

   r0 = table_open( &c, "abcde", 4 ) ;


   for (count = 0 ; count < 40 ; count += 1 )
   {
     printf("%i\n",key) ;
      sprintf(data, "<%i %i>", count, key ) ;

   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD(&key)
      FLENGTH(strlen(data)+1)
      FROM(data)
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

     key = ( key & 0x8000 ) ?
              ( ( key << 1 ) ^ 0x10011 ) :
              ( ( key << 1 ) ) ;
    /* key += 4 ; */
   } /* endfor */

   r2 = table_close ( &c ) ;

   printf("Responses %i %i %i\n",
                 r0, r1, r2
                 ) ;
   return 0 ;


}
