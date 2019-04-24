/*
 *  Copyright 1993, 2019 IBM Corporation
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
/*
 * Test multiple data tables files
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */
#include <stdio.h>
#include <table_create.h>

typedef enum
{
   table_count = 8 ,
   test_count = 100000
} constant_t ;

static const char * table_names[] = {
   "test_0", "test_1", "test_2", "test_3",
   "test_4", "test_5", "test_6", "test_7"
} ;

/*
 * Load 1 record into the table, with key=data=record-number
 */
static void load_table(const char * table_name, int table_number)
{
   PAC_TABLE_WRITER c ;
   int r0 = table_open(&c, table_name, sizeof(int)) ;
   int r1 ;
   int r2 ;

   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD(&table_number)
      FLENGTH(sizeof(int))
      FROM(&table_number)
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

   r2 = table_close(&c) ;


   printf("Responses %i %i %i\n",
                 r0, r1, r2
                 ) ;

}

static void load_all_tables(void)
{
   int table_number ;
   for (table_number = 0 ; table_number < table_count ; table_number += 1 )
   {
      load_table(table_names[table_number], table_number) ;
   } /* endfor */
}

int main(void) {
   load_all_tables() ;
   return 0 ;
   }
