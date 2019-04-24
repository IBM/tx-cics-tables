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
 * Externals for 'tables' functions
 */
#include <string.h>
#include <table_rcode.h>
#include <table_read_int.h>

int table_read (
        _TABLE_RESPONSE_T * table_response,
   const void *  rid,
   const char *  file_name,
        _TABLE_STATIC_T * table_static,
         int     match_fails
  ) ;

#define EXEC_PAC_READ_TABLE                                  \
  {                                                           \
    static _TABLE_STATIC_T _TABLE_STATIC =  { 0 ,  NULL  } ;  \
    _TABLE_RESPONSE_T _TABLE_RESPONSE ;                       \
    const char * _TABLE_FILE ;                                \
    const void * _TABLE_RID ;                                 \
    unsigned int _EXPECTED_TAG ;                           \
    const char * _EXPECTED_NAME ; \
    int _TABLE_RESP ;
/*
 * int _TABLE_RECORD_LENGTH ; \
 *   void * _TABLE_SET ; \
 *   void * _TABLE_ACTUAL_RID ; \
 *   static int _TABLE_FILE_TAG = 0 ;                          \
 */
#define FILE(x) \
    _TABLE_FILE = (x) ;

/*
 * #define FILE_TAG(x) \
 *    _TABLE_FILE_TAG = (x) ;
 */
#define FILE_TAG(x)

#define RIDFLD(x) \
    _TABLE_RID = (x) ;

#define HASH_FUNCTION(x)

#define SET(x)                                                            \
    _EXPECTED_TAG = _TABLE_STATIC.table_file_tag ;                       \
    _EXPECTED_NAME = _TABLE_STATIC.table_file_name ;                     \
    _TABLE_RESP = table_read(                                             \
      & _TABLE_RESPONSE                                                   \
     ,_TABLE_RID                                                          \
     ,_TABLE_FILE                                                         \
     , & _TABLE_STATIC                                                    \
     , (_TABLE_STATIC.table_file_name != _TABLE_FILE ) &&                 \
       strcmp( table_track_ext.all.page[_EXPECTED_TAG].name \
        , _TABLE_FILE )                                                  \
     ) ;                                                                  \
    x = _TABLE_RESPONSE.field_addr ;

#define ACTUAL_RID(x) \
    x = _TABLE_RESPONSE.actual_rid ;

#define RESP(x) \
   x = _TABLE_RESP ;

#define FLENGTH(x) \
   x = _TABLE_RESPONSE.field_length ;

#define EXACT_RID(x) \
   x = _TABLE_RESPONSE.exact_rid ;

#define END_EXEC_PAC_READ_TABLE \
   }

