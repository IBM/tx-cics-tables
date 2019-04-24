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
 * Externals for data table construction
 */

typedef enum {
   name_length = 100
   } ;

typedef enum {
   table_just_defined,
   table_one_file,
   table_clustered,
   table_autoclustered
   } descriptor_state ;

typedef struct {
  int fd /* File descriptor for table file */ ;
  void * shmat_address /* Address where the table is attached */ ;
  unsigned int next_free_offset /* Offset where new data can be placed */ ;
  unsigned int record_count /* How many records are in the cluster */ ;
   } cluster_descriptor ;

typedef struct {
  descriptor_state state ;
  cluster_descriptor primary ;
  cluster_descriptor current_secondary ;
  unsigned int rid_length ;
  char table_name[name_length] ;
} PAC_TABLE_WRITER ;

int table_open (
  PAC_TABLE_WRITER * table ,
  const char * file_name ,
  unsigned int rid_length
  ) ;

int table_cluster_define (
  PAC_TABLE_WRITER * table ,
  unsigned int cluster_number ,
  const void * highest_rid
  ) ;

int table_autocluster (
  PAC_TABLE_WRITER * table
  ) ;

int table_write (
  PAC_TABLE_WRITER * table ,
  const void * rid ,
  const void * record ,
  unsigned int record_length
  ) ;

int table_close (
  PAC_TABLE_WRITER * table
  ) ;

#define EXEC_PAC_WRITE_TABLE \
  { int _TABLE_RECORD_LENGTH = 0 ; \
    PAC_TABLE_WRITER * _TABLE =  (void *) 0 ; \
    const void * _TABLE_RECORD =  (void *) 0 ; \
    const void * _TABLE_RID =  (void *) 0 ; \
    int _TABLE_RESP ; \

#define TABLE(x) _TABLE = (x) ;

#define RIDFLD(x) _TABLE_RID = (x) ;

#define FLENGTH(x) _TABLE_RECORD_LENGTH = (x) ;

#define HASH_FUNCTION(x)

#define FROM(x) \
   _TABLE_RECORD = (x) ; \
   _TABLE_RESP = table_write ( _TABLE, _TABLE_RID, _TABLE_RECORD, _TABLE_RECORD_LENGTH ) ;

#define RESP(x) x = _TABLE_RESP ;

#define END_EXEC_PAC_WRITE_TABLE \
   }
