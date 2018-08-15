/*
 * IBM Internal Use Only           T J C Ward       28 March 1996
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */
#include <table_ext.h>

#include <stdio.h>

typedef struct
{
  int key_major ;
  int key_minor ;
} db_key_t ;



int main(int argc, char **argv) {
   int opt_tag ;
   int result ;

   char * my_record ;
   int my_length ;
   int my_resp ;
   db_key_t * actual_rid ;
   int exact ;

   int key_major = atoi(argv[1]) ;
   int key_minor=atoi(argv[2]) ;
   db_key_t db_key ;
   db_key.key_major = key_major ;
   db_key.key_minor = key_minor ;

     EXEC_PAC_READ_TABLE
       FILE("abcde")
       RIDFLD(&db_key)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

       printf("Search for key (%i,%i) "
              "Response is %i "
              "Found key (%i,%i) "
              "Data length is %i "
              "Exact is %i "
              "Data \"%s\"\n",

              db_key.key_major, db_key.key_minor,
              my_resp,
              actual_rid -> key_major, actual_rid -> key_minor,
              my_length,
              exact,
              my_record
              ) ;
   return 0 ;
}
