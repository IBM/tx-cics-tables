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

#define DEBUG

void try_table(const char * table_name, int * x)
{
   char * my_record ;
   int my_length ;
   int my_resp ;
   int * actual_rid ;
   int r ;
   int exact ;
   r = 0 ;

     EXEC_PAC_READ_TABLE
       FILE(table_name)
       RIDFLD(x)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

    if (0 == my_resp)
    {
#if defined(DEBUG)
       printf("Search for key 0x%08x "
              "Found key 0x%08x "
              "Data length is %i "
              "Data \"%s\"\n",

              *x,*actual_rid,my_length,my_record

              ) ;
#endif
       if (exact)
       {
#if defined(DEBUG)
          printf("Exact match for %i %i\n",*x,*actual_rid) ;
#endif
       }
       if (*actual_rid >= *x && *actual_rid >= r )
       {
          r = *actual_rid ;
       }
       else
       {
          fprintf(stderr,"Looked for %08x, found %08x, prev %08x\n",
                               *x , *actual_rid, r ) ;
       } /* endif */
    }
}

int read_test(int my_rid )
{
   int x ;

   for (x = 0 ; x < 65536; x += 1 )
   {
      try_table("table_4_a",&x) ;
      try_table("table_4_b",&x) ;
      try_table("table_4_c",&x) ;
      try_table("table_4_d",&x) ;
      try_table("table_4_e",&x) ;
   } /* endfor */


  return 0 ;
}

int main(int argc, char **argv) {
   int opt_tag ;
   int result ;
   int x ;
   if (argc <= 1)
   {
      result = read_test(0) ;
   }
   else
   {
     for (x = 1 ; x < argc ; x += 1 )
     {
       result = read_test( atoi(argv[x]) ) ;
     } /* endfor */
   } /* endif */
   return 0 ;
}
