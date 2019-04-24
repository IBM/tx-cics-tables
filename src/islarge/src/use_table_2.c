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

int read_test(int my_rid )
{
   char * my_record ;
   int my_length ;
   int my_resp ;
   int * actual_rid ;
   int x ;
   int r ;
   int exact ;
   r = 0 ;

   for (x = 0 ; x < 65535; x += 1 )
   {
     EXEC_PAC_READ_TABLE
       FILE("abcde")
       RIDFLD(&x)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

     EXEC_PAC_READ_TABLE
       FILE("abcde")
       RIDFLD(&x)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

     EXEC_PAC_READ_TABLE
       FILE("abcdf")
       RIDFLD(&x)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

     EXEC_PAC_READ_TABLE
       FILE("abcdg")
       RIDFLD(&x)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

     EXEC_PAC_READ_TABLE
       FILE("abcde")
       RIDFLD(&x)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

     EXEC_PAC_READ_TABLE
       FILE("abcdh")
       RIDFLD(&x)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

     EXEC_PAC_READ_TABLE
       FILE("abcdi")
       RIDFLD(&x)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
       EXACT_RID(exact)
     END_EXEC_PAC_READ_TABLE ;

    if (0 == my_resp)
    {
#if defined(DEBUG)
       printf("Search for key %i "
              "Found key %i "
              "Data length is %i "
              "Data \"%s\"\n",

              x,*actual_rid,my_length,my_record

              ) ;
#endif
       if (exact)
       {
#if defined(DEBUG)
          printf("Exact match for %i %i\n",x,*actual_rid) ;
#endif
       }
       if (*actual_rid >= x && *actual_rid >= r )
       {
          r = *actual_rid ;
       }
       else
       {
          fprintf(stderr,"Looked for %08x, found %08x, prev %08x\n",
                               x , *actual_rid, r ) ;
       } /* endif */
    }
   } /* endfor */

   printf("Result was %i\n", my_resp) ;

  return my_resp ;
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
