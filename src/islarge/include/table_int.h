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
 *  Internals of 'table' package
 */
typedef enum {
  table_correct_magic=0x01234567 ,
  table_correct_version=1 ,
  table_correct_version_primary=2
  } table_constants ;

#define TABLE_DIRECTORY "/var/pac_tables/"
#define TABLE_CLUSTER_DIRECTORY ".clusters/"


/*
 * The layout of a 'shmat' attached file is ...
 * +0: Magic value
 * +4: PAC-table version
 * +8: RID field size
 * +12: Data offset (d)
 * +16: pointer to 'root' node (relative to file start)
 * +20: pointer to start of index table (relative to file start)
 * +24: maximum tree depth in this file
 * +28: First record
 *
 * Record layout is
 * +0: pointer to 'next' record (relative to file start)
 * +4: pointer to 'previous' record (relative to file start)
 * +8: length of data for this record
 * +12: RID field
 * +d: Data
 *
 * Index layout is
 * +0: byte number of key causing index split
 * +4: bit index within byte causing index split
 * +8: pointer to next node when bit is '0'
 * +12: pointer to next node when bit is '1'
 *
 * Index nodes are distinguished from data nodes since the index occurs after
 * all of the data for the file.
 *
 * Records are placed on word alignment, and there may be padding after the
 * RID field to keep word alignment
 */
typedef struct {
  unsigned int magic ;
  unsigned int version ;
  unsigned int rid_size ;
  unsigned int data_offset ;
  unsigned int root_node ;
  unsigned int index_start ;
  unsigned int max_tree_depth ;
} table_shmat_head_t ;

typedef struct {
  table_shmat_head_t header ;
  unsigned int hash_table[1];
} table_shmat_hash_t ;

typedef struct {
  unsigned int succ_key ;
  unsigned int pred_key ;
  unsigned int data_length ;
} table_shmat_data_t ;

typedef struct {
  unsigned int key_byte ;
  unsigned int key_shift ;
  unsigned int next_key[2] ;
} table_shmat_index_t ;

typedef struct {
  table_shmat_data_t header ;
  char rid_field[1] ;
} table_shmat_rid_t ;

