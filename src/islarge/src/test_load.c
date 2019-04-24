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
#include <stdio.h>
int main(void) {
   PAC_TABLE_WRITER c ;
   int r0, r1, r2 ;
   int key = 100 ;
   char data [ 100 ] ;
   int count ;

   r0 = table_open( &c, "abcde", 4 ) ;


   for (count = 0 ; count < 40 ; count += 1 )
   {
     printf("%i\n",key) ;
      sprintf(data, "<%i %i>", count, key ) ;

   EXEC_PAC_WRITE_TABLE
      TABLE(&c)
      RIDFLD(&key)
      FLENGTH(strlen(data)+1)
      FROM(data)
      RESP(r1)
   END_EXEC_PAC_WRITE_TABLE ;

     key = ( key & 0x8000 ) ?
              ( ( key << 1 ) ^ 0x10011 ) :
              ( ( key << 1 ) ) ;
    /* key += 4 ; */
   } /* endfor */

   r2 = table_close ( &c ) ;

   printf("Responses %i %i %i\n",
                 r0, r1, r2
                 ) ;
   return 0 ;


}
