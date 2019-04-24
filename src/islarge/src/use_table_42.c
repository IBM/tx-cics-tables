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
