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
