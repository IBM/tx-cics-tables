/*
 * IBM Internal Use Only           T J C Ward       28 March 1996
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */
#include <table_create.h>
#include <stdio.h>
typedef struct
{
  int key_major ;
  int key_minor ;
} db_key_t ;

void write_group(PAC_TABLE_WRITER *c, int key_major, int minor_count )
{
   int count ;
   db_key_t key ;
   char data [ 100 ] ;
   int r1 ;

   for (count = 0 ; count < minor_count ; count += 1 )
   {
     key.key_major = key_major ;
     key.key_minor = count ;
     sprintf(data, "<%i %i>", key.key_major, key.key_minor ) ;

     EXEC_PAC_WRITE_TABLE
        TABLE(c)
        RIDFLD(&key)
        FLENGTH(strlen(data)+1)
        FROM(data)
        RESP(r1)
     END_EXEC_PAC_WRITE_TABLE ;

   } /* endfor */
}

int main(void) {
   PAC_TABLE_WRITER c ;
   int count ;
   int r0, r2 ;

   r0 = table_open( &c, "abcde", sizeof(db_key_t) ) ;

   write_group(&c,0,0x234) ;
   write_group(&c,2,10) ;
   write_group(&c,227,100) ;

   r2 = table_close ( &c ) ;

   printf("Responses %i %i\n",
                 r0, r2
                 ) ;
   return 0 ;


}
