/*
 * IBM Internal Use Only           T J C Ward       28 September 1993
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */
#include <table_create.h>

void build_table(const char * table_name)
{
   PAC_TABLE_WRITER c ;
   int r0, r1, r2 ;
   unsigned int clusterlimit = 0x00000004 ;
   unsigned int key ;

   r0 = table_open( &c, table_name, sizeof(int) ) ;

   table_cluster_define(&c, 0, &clusterlimit ) ;


   key = 0x00000001 ;
   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD(&key)
      FLENGTH(9)
      FROM("datadata")
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

   key = 0x00000003 ;
   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD(&key)
      FLENGTH(8)
      FROM("datazee")
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

   clusterlimit = 0xffffffff ;
   table_cluster_define(&c, 1, &clusterlimit ) ;

   key = 0x00000005 ;
   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD(&key)
      FLENGTH(8)
      FROM("datazkk")
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

   r2 = table_close ( &c ) ;

   printf("Responses %i %i %i\n",
                 r0, r1, r2
                 ) ;
}

int main(void) {
   build_table("table_4_a") ;
   build_table("table_4_b") ;
   build_table("table_4_c") ;
   build_table("table_4_d") ;
   build_table("table_4_e") ;
   return 0 ;


}
