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
   int k0;
   int k1 ;
   short k2 ;
} dbkeys ;

int read_test(int * tag, int k0, int k1, short k2 )
{
   char * my_record ;
   int my_length ;
   int my_resp ;
   dbkeys * actual_rid ;
   int x ;
   int r ;
   dbkeys dbk ;

   dbk.k0 = k0 ;
   dbk.k1 = k1 ;
   dbk.k2 = k2 ;

   r = 0 ;

     EXEC_PAC_READ_TABLE
       FILE("abcde")
       FILE_TAG(tag)
       RIDFLD(&dbk)
       SET(my_record)
       RESP(my_resp)
       FLENGTH(my_length)
       ACTUAL_RID(actual_rid)
     END_EXEC_PAC_READ_TABLE ;

     printf("resp=%i rid=%i %i %i\n",
            my_resp, actual_rid -> k0,actual_rid -> k1, actual_rid -> k2) ;

  return my_resp ;
}

int main(int argc, char **argv) {
   int opt_tag ;
   int result ;
   int x ;
   opt_tag=0 ;

   read_test(&opt_tag, atoi(argv[1]),atoi(argv[2]), atoi(argv[3])) ;

   return 0 ;
}
