/*
 * IBM Internal Use Only           T J C Ward       28 September 1993
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */
#include <table_create.h>
int main(void) {
   PAC_TABLE_WRITER c ;
   int r0, r1, r2 ;

   r0 = table_open( &c, "abcde", 6 ) ;

   table_cluster_define(&c, 0, "llllll" ) ;

   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD("keykey")
      HASH_FUNCTION(rid_hash)
      FLENGTH(9)
      FROM("datadata")
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD("keyzzz")
      HASH_FUNCTION(rid_hash)
      FLENGTH(8)
      FROM("datazee")
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

   table_cluster_define(&c, 1, "zzzzzz" ) ;

   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD("zzzkey")
      HASH_FUNCTION(rid_hash)
      FLENGTH(8)
      FROM("datazkk")
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

   r2 = table_close ( &c ) ;

   printf("Responses %i %i %i\n",
                 r0, r1, r2
                 ) ;
   return 0 ;


}
