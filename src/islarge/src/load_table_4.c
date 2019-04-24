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
