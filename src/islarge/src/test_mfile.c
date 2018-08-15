/*
 * Test multiple data tables files
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <table_ext.h>

typedef enum
{
   table_count = 8 ,
   test_count = 100000
} constant_t ;

static const char * table_names[] = {
   "test_0", "test_1", "test_2", "test_3",
   "test_4", "test_5", "test_6", "test_7"
} ;

static void fail_test(const char * text, int n1, int n2)
{
   fprintf(stderr,"%s %i %i\n", text, n1, n2) ;
}

/*
 * This reads a record from the given table and checks that it is as
 * expected
 */
static void test_table(const char * table_name, int table_number)
{
   int * my_record ;
   int my_resp ;
   int * actual_rid ;
   int exact ;
   int my_length ;

   EXEC_PAC_READ_TABLE
     FILE(table_name)
     RIDFLD(&table_number)
     SET(my_record)
     RESP(my_resp)
     FLENGTH(my_length)
     ACTUAL_RID(actual_rid)
     EXACT_RID(exact)
   END_EXEC_PAC_READ_TABLE ;

   /* fprintf(stderr,"Data located at address 0x%08x\n", my_record) ; */
   if (0 != my_resp )
   {
      fail_test("Bad response code", my_resp, table_number) ;
   } /* endif */
   if ( 0 == exact)
   {
      fail_test("Record not found", 0, table_number) ;
   } /* endif */
   if (*actual_rid != table_number)
   {
      fail_test("Wrong RID found", *actual_rid, table_number) ;
   } /* endif */
   if (my_length != sizeof(int))
   {
      fail_test("Wrong data length", my_length, sizeof(int)) ;
   } /* endif */
   if (*my_record != table_number)
   {
      fail_test("Wrong data", *my_record, table_number) ;
   } /* endif */
}

/*
 * This reads from all tables in random sequence. 'drand48()' is the system
 * random number generator which generates numbers between 0 and 1.
 *
 * This randomly tests 'constant' and 'constructed' table names
 */
static void test_all_tables(void)
{
   int test_index ;
   for (test_index = 0 ; test_index < test_count ; test_index += 1 )
   {
     int table_to_test = drand48() * table_count ;
     /* fprintf(stderr,"Testing table %i\n", table_to_test) ; */
     if (drand48() > 0.5 )
     {
       char table_name[100] ;
       sprintf(table_name, "test_%i", table_to_test) ;
       test_table(table_name, table_to_test) ;
     }
     else
     {
        test_table(table_names[table_to_test], table_to_test) ;
     } /* endif */
   } /* endfor */
}

int main(void) {
   test_all_tables() ;
   return 0 ;
   }
