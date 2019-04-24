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
 * Internals relevant when reading a table
 */

#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>

#include <table_int.h>

typedef enum {
   attached_file_associative_bits = 2,
   attached_file_associative_width = 1 << attached_file_associative_bits,
   open_file_associative_bits = 2,
   open_file_associative_width = 1 << open_file_associative_bits,
   open_file_index_bits = 6,
   open_file_index_depth = 1 << open_file_index_bits,
   all_file_index_bits = 10,
   all_file_index_depth = 1 << all_file_index_bits,

   open_table_file_index_bits = all_file_index_bits - open_file_index_bits,

   average_name_length = 10,
   name_table_depth = all_file_index_depth*(average_name_length+1) ,
   name_hash_depth = 1024
   } ;

typedef enum {
   not_attached_file_mask = 0x0fffffff /* Entry in attached file table signifying 'not currently used' */
   } ;

typedef struct {
   unsigned int cluster_number ;
   unsigned int segment_number_and_file_number ;
   } attached_file ;

typedef struct {
   attached_file line[attached_file_associative_width] ;
   } attached_file_line ;

typedef struct {
   attached_file_line page[1] ;
   } attached_file_page ;

typedef struct {
   unsigned int cluster_number ;
   unsigned int file_descriptor_and_file_index_hi ;
   } open_file ;

typedef struct {
   open_file line[open_file_associative_width] ;
   } open_file_line ;

typedef struct {
   open_file_line page[open_file_index_depth] ;
   } open_file_page ;

typedef struct {
   char * name ;
   unsigned int next_hash ;
   unsigned int file_has_no_secondaries ;
   unsigned int spare ;
   } all_file ;

typedef struct {
   all_file page[all_file_index_depth] ;
   } all_file_page ;

typedef struct {
   unsigned int page[name_hash_depth] ;
   } name_hash_page ;

typedef struct {
   char page[name_table_depth] ;
   } name_page ;

/*
 * Altogether, we know ...
 *  attached file table
 *  open file table
 *  all file table
 *  name-to-file hash table
 *  name table
 *  lowest unused entry in all-file table
 *  lowest unused entry in name table
 * These are 'uninitialised data', which means they will be constructed as
 *  zero the first time they are referred to.
 */
typedef struct {
   attached_file_page attached ;
   open_file_page     opened ;
   all_file_page      all ;
   name_hash_page     name_hash ;
   name_page          name ;

   unsigned int       free_file_index ;
   unsigned int       free_name_index ;

   } table_track_t ;

extern table_track_t table_track_ext ;

/*
 * Types for optimising interface usage
 */

typedef struct {
  unsigned  int   table_file_tag ;
  const    char * table_file_name ;
} _TABLE_STATIC_T ;

typedef struct {
           void * field_addr ;
  unsigned  int   field_length ;
           void * actual_rid ;
  unsigned  int   exact_rid ;
  unsigned  int   version ;
  unsigned  int   next_record_index ;
  } _TABLE_RESPONSE_T ;

