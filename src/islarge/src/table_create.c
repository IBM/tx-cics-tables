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
 * Table write function for PAC data tables
 */

#include <table_create_int.h>
#include <table_rcode.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include <table_create.h>
#include <hibit.h>

/* #define DB(x) printf x */
#define DB(x)

/*
 * DIAG and the following enums are used to build diagnostics into the code,
 * and to turn them off for production builds
 */

#define DIAG(x,y) if (x) { printf y; fflush(stdout) ; }

/*
 * Flags indicating diagnostic builds
 */
typedef enum {
     diag_api    = 0 /* Diagnose application interface */
   ,diag_file    = 0 /* Diagnose 'files' (open/attach etc.) */
   , diag_vmm    = 0 /* Diagnose the virtual memory manager */
   ,diag_scan    = 0 /* Diagnose scanning for a key */
   ,diag_cluster = 0 /* Diagnosing goings-on for cluster support */
   ,diag_show    = 0 /* Diagnosing the structure of an index at completion */
   } ;

/*
 * Open a table file for creation
 */

static int table_open_file_primary ( const char * file_name )
{
   char full_path_name[1000] ;

   /*
    * Construct the proper file name
    */
   strcpy(full_path_name,TABLE_DIRECTORY) ;
   strcat(full_path_name, file_name) ;
   /*
    * See if it can be opened
    */
   return open(full_path_name, O_RDWR | O_TRUNC | O_CREAT , 0644 ) ;
}

static int table_open_file_secondary ( const char * file_name, unsigned int cluster_number )
{
   char full_path_name[1000] ;

   /*
    * Construct the directory
    */
   sprintf(full_path_name, TABLE_DIRECTORY "%s" TABLE_CLUSTER_DIRECTORY, file_name) ;
   mkdir(full_path_name, 0777) ;
   /*
    * Construct the name of the secondary file
    */
   sprintf(full_path_name, TABLE_DIRECTORY "%s" TABLE_CLUSTER_DIRECTORY "%s.%08x", file_name, file_name, cluster_number) ;
   /*
    * See if it can be opened
    */
   return open(full_path_name, O_RDWR | O_TRUNC | O_CREAT , 0644 ) ;
}

#define ROUND_UP(x,y) ( ( (x)+((y)-1) ) & ~ ((y) - 1) )
/*
 * Initiate
 */
int table_open (
  PAC_TABLE_WRITER * table ,
  const char * file_name ,
  unsigned int rid_length
 ) {
    unsigned int nl = strlen(file_name) ;
    table -> table_name[0] = 0 ; /* Make it apparent if the name is bad and someone tries to use it */
    if (nl > name_length-1) return PAC_TABLE_NAME_TOO_LONG ;
    table -> state = table_just_defined ;
    table -> rid_length = rid_length ;
    memcpy(table -> table_name, file_name, nl+1) ;
    return PAC_TABLE_NOERROR ;
}

static int table_open_primary(
  cluster_descriptor * cluster ,
  const char * file_name ,
  unsigned int rid_length ,
  int actual_version
) {
   int table_fd ;
   table_shmat_head_t * shmat_address ;
   unsigned int data_offset ;
   /*
    * Open the UNIX file
    */
   table_fd = table_open_file_primary ( file_name ) ;
   cluster -> fd = table_fd ;
   if ( table_fd < 0 ) return PAC_TABLE_DISABLED ; /* File error ... */

   shmat_address = (table_shmat_head_t * ) shmat(table_fd, (void *) 0, SHM_MAP) ;
   if ((int) shmat_address == -1 )
   {
      close(table_fd) ;
      return PAC_TABLE_SHMAT_FAILED ;
   }
   cluster -> shmat_address = shmat_address ;

   /*
    * Initialise the UNIX file data
    */
   shmat_address -> magic = 0 ; /* 'not complete yet' */
   shmat_address -> version = actual_version ;
   shmat_address -> data_offset = sizeof(table_shmat_data_t) + ROUND_UP(rid_length, sizeof(int) ) ;
   shmat_address -> rid_size = rid_length ;
   shmat_address -> root_node = 0 ; /* 'empty' */
   cluster -> next_free_offset = sizeof(table_shmat_head_t) ;
   cluster -> record_count = 0 ;

   return PAC_TABLE_NOERROR ;
}

static int table_open_secondary(
  cluster_descriptor * cluster ,
  const char * file_name ,
  unsigned int rid_length ,
  unsigned int cluster_number
) {
   int table_fd ;
   table_shmat_head_t * shmat_address ;
   unsigned int data_offset ;
   /*
    * Open the UNIX file
    */
   table_fd = table_open_file_secondary ( file_name, cluster_number ) ;
   cluster -> fd = table_fd ;
   if ( table_fd < 0 ) return PAC_TABLE_DISABLED ; /* File error ... */

   shmat_address = (table_shmat_head_t * ) shmat(table_fd, (void *) 0, SHM_MAP) ;
   if ((int) shmat_address == -1 )
   {
      close(table_fd) ;
      return PAC_TABLE_SHMAT_FAILED ;
   }
   cluster -> shmat_address = shmat_address ;

   /*
    * Initialise the UNIX file data
    */
   shmat_address -> magic = 0 ; /* indicates 'not complete yet' */
   shmat_address -> version = table_correct_version ;
   shmat_address -> data_offset = sizeof(table_shmat_data_t) + ROUND_UP(rid_length, sizeof(int) ) ;
   shmat_address -> rid_size = rid_length ;
   cluster -> next_free_offset = sizeof(table_shmat_head_t) ;
   cluster -> record_count = 0 ;

   return PAC_TABLE_NOERROR ;
}


/*
 * Add an element
 */
static int table_write_x(
  cluster_descriptor * cluster ,
  const void * rid ,
  const void * record ,
  unsigned int record_length
) {
   void * shmat_address ;
   table_shmat_head_t *t_h ;
   unsigned int next_free_offset ;
   unsigned int rid_size ;
   unsigned int data_offset ;
   void * next_free_address ;

   unsigned int intended_next_free_offset ;
   unsigned int intended_record_count ;

   shmat_address = cluster -> shmat_address ;
   t_h = ( table_shmat_head_t * ) shmat_address ;

   next_free_offset = cluster -> next_free_offset ;
   rid_size = t_h -> rid_size ;
   data_offset = t_h -> data_offset ;

   next_free_address = (char *) shmat_address + next_free_offset ;
   intended_next_free_offset =
         ( next_free_offset + data_offset + ROUND_UP(record_length,4)) ;
   intended_record_count = cluster -> record_count + 1 ;

   /*
    * Check that the data and index will still fit within the 256M segment
    */
   if (ROUND_UP(intended_next_free_offset,16) + intended_record_count*sizeof(table_shmat_index_t) < 0x10000000)
   {
      ( (table_shmat_data_t *) next_free_address ) -> data_length = record_length ;
      memcpy(&(((table_shmat_rid_t *) next_free_address) ->rid_field), rid, rid_size) ;
      memcpy((char *) next_free_address + data_offset ,
             record,
             record_length ) ;

      cluster -> next_free_offset = intended_next_free_offset ;
      cluster -> record_count = intended_record_count ;

      return PAC_TABLE_NOERROR ;
   }
   else
   {
      return PAC_TABLE_CLUSTER_OVERFLOW ;
   } /* endif */

}

int table_write(
  PAC_TABLE_WRITER * table ,
  const void * rid ,
  const void * record ,
  unsigned int record_length
) {
   if (table_just_defined == table -> state)
   {
      int open_rc = table_open_primary(& (table -> primary), table -> table_name,
                         table -> rid_length, table_correct_version ) ;
      if (PAC_TABLE_NOERROR == open_rc)
      {
         table -> state = table_one_file ;
         return table_write_x(& (table -> primary), rid, record, record_length) ;
      }
      else
      {
         return open_rc ;
      } /* endif */
   }
   else if ( table_clustered == table -> state)
   {
      return table_write_x(& (table -> current_secondary), rid, record, record_length) ;
   }
   else if ( table_one_file == table -> state)
   {
      return table_write_x(& (table -> primary), rid, record, record_length) ;
   }
   else
   {
      return PAC_TABLE_NYI ; /* autocluster not yet implemented */
   } /* endif */
}

/*
 * Terminate, construct index,  and report statistics
 */
#define PTR(type, base, offset) ((type) (((char *) base) + offset ))
#define VAL(type, base, field) (((type *) base) -> field)
static void build_index (
  unsigned int * free_node_ofs_p ,
  unsigned int current_record_offset ,
  void * shmat_address ,
  unsigned int index_start ,
  unsigned int rid_size
)
{
   unsigned int cand_offset ;
   char * new_rid ;
   char * cand_rid ;
   table_shmat_index_t * cand_index ;
   unsigned int rid_x ;
   unsigned int cand_byte ;
   unsigned int new_byte ;
   unsigned int rid_xor ;
   unsigned int rid_y ;
   unsigned int split_ofs ;
   unsigned int * split_ofs_ptr ;
   table_shmat_index_t * split_ptr ;
   unsigned int split_key_byte ;
   unsigned int split_key_shift ;
   unsigned int free_node_ofs ;
   table_shmat_index_t * free_node_p ;

   cand_offset = VAL(table_shmat_head_t,shmat_address,root_node) ;

   if (0 == cand_offset)
   {
      /*
       * First node in the tree
       */
     VAL(table_shmat_head_t,shmat_address,root_node) = current_record_offset ;
     PTR(table_shmat_data_t *,shmat_address,current_record_offset) -> succ_key = 0 ;
     PTR(table_shmat_data_t *,shmat_address,current_record_offset) -> pred_key = 0 ;
   }
   else
   {
     /*
      * Scan the tree to find the appropriate data node
      */
     new_rid = PTR(char *,shmat_address,current_record_offset + sizeof(table_shmat_data_t)) ;

     while (cand_offset >= index_start)
     {
        cand_index = PTR(table_shmat_index_t *,shmat_address,cand_offset) ;
        DIAG(diag_scan,("examining byte %i bit %i\n",
             cand_index -> key_byte ,
             cand_index -> key_shift)) ;
        cand_offset = cand_index -> next_key [
          ( new_rid [ cand_index -> key_byte ] >> cand_index -> key_shift )
            & 1
          ] ;
     } /* endwhile */
     /*
      * We have now descended to a data node. Find which is the
      * most significant difference bit.
      */
     cand_rid = PTR(table_shmat_rid_t *,shmat_address,cand_offset) -> rid_field ;
     for (rid_x = 0; rid_x < rid_size; rid_x += 1)
     {
        cand_byte = cand_rid [ rid_x ] ;
        new_byte = new_rid [ rid_x ] ;
        if (cand_byte != new_byte) break ;
     } /* endfor */

     if (rid_x < rid_size) /* A new key ... */
     {
       rid_xor = cand_byte ^ new_byte ;
       rid_y = hibit[rid_xor] ;
       DIAG(diag_scan,("Split at byte %i bit %i from %i\n",rid_x, rid_y, cand_byte)) ;
       /*
        * Scan from the root again to find the split point
        */
       split_ofs_ptr = & VAL(table_shmat_head_t,shmat_address,root_node) ;
       split_ofs = VAL(table_shmat_head_t,shmat_address,root_node) ;

       while (split_ofs >= index_start)
       {
         split_ptr = PTR(table_shmat_index_t *,shmat_address, split_ofs) ;
         split_key_byte = split_ptr -> key_byte ;
         split_key_shift = split_ptr -> key_shift ;
         if (split_key_byte > rid_x
             ||
            (split_key_byte == rid_x && split_key_shift < rid_y)
         ) break ;

         split_ofs_ptr = & (
                      split_ptr -> next_key [
          ( new_rid [ split_key_byte ] >> split_key_shift )
            & 1
          ] ) ;
         split_ofs = * split_ofs_ptr ;
       } /* endwhile */
       /*
        * All being well, we should now be pointing at the index to split !
        */
       free_node_ofs = * free_node_ofs_p ;
       free_node_p = PTR(table_shmat_index_t *,shmat_address,free_node_ofs) ;
       free_node_p -> key_byte = rid_x ;
       free_node_p -> key_shift = rid_y ;
       if ((new_byte >> rid_y) & 1)
       {
          free_node_p -> next_key[0] = split_ofs ;
          free_node_p -> next_key[1] = current_record_offset ;
       }
       else
       {
          free_node_p -> next_key[1] = split_ofs ;
          free_node_p -> next_key[0] = current_record_offset ;
       } /* endif */
       * split_ofs_ptr = free_node_ofs ;
       * free_node_ofs_p = free_node_ofs + sizeof(table_shmat_index_t) ;
     }
     else
     { /* A duplicate key. Ignore for the moment. */

     } /* endif */
   } /* endif */
}

static void indent ( unsigned int x )
{
   unsigned int a ;
   putchar('\n') ;
   for (a = 0 ; a < x  ; a += 1)
   {
      putchar(' ') ;
   } /* endfor */
}

static void show_key ( table_shmat_rid_t * key , unsigned int rid_size)
{
   unsigned int x ;
   for (x = 0 ; x < rid_size ; x += 1)
   {
      printf("%02x ", key -> rid_field[x]) ;
   } /* endfor */
}

static void show_split ( table_shmat_index_t * ix )
{

   unsigned int x ;
   unsigned int key_byte = ix -> key_byte ;
   /* printf("%i %i", ix -> key_byte, ix -> key_shift ) ;*/
   for ( x = 0 ; x < key_byte ; x += 1 )
   {
      printf("00 ") ;
   } /* endfor */
   printf("%02x", 1 << ix -> key_shift)  ;
}

static void table_show (
  void * shmat_address ,
  unsigned int rid_size ,
  unsigned int level ,
  unsigned int node ,
  unsigned int index_start
  )
{
   if (node < index_start)
   {
      indent(level) ;
      printf("0x%08x (0x%08x 0x%08x) ",node,
         PTR(table_shmat_data_t *, shmat_address, node) -> pred_key,
         PTR(table_shmat_data_t *, shmat_address, node) -> succ_key
         ) ;
      show_key(PTR(table_shmat_rid_t *, shmat_address, node), rid_size) ;
   }
   else
   {
      indent(level) ; putchar('{') ;
      table_show(
         shmat_address,
         rid_size ,
         level+1,
         PTR(table_shmat_index_t *,shmat_address, node) -> next_key[0] ,
         index_start
         ) ;
      indent(level) ; putchar('}') ;
      indent(level) ; show_split(PTR(table_shmat_index_t *, shmat_address, node)) ;
      indent(level) ; putchar('{') ;
      table_show(
         shmat_address,
         rid_size ,
         level+1,
         PTR(table_shmat_index_t *,shmat_address, node) -> next_key[1] ,
         index_start
         ) ;
      indent(level) ; putchar('}') ;

   } /* endif */
}

/*
 * Walk the table recursively to fill in 'next' and 'previous' pointers
 *
 * This becomes important for tables with secondaries, becuase it is how we
 * navigate from one secondary to the next in the case where there is no
 * 'next' record in the current secondary.
 */
static void table_fill_succ_pred (
  void * shmat_address,
  unsigned int node,
  unsigned int index_start,
  /* IN */ unsigned int * where_to_put_node,
  /* IN */ unsigned int what_node_is_pred,
  /* OUT */ unsigned int ** where_to_put_succ,
  /* OUT */ unsigned int * what_node_is_this
  )
{
   if (node < index_start)
   {
      *where_to_put_node=node ;
      PTR(table_shmat_data_t *,shmat_address,node) -> pred_key = what_node_is_pred ;
      *where_to_put_succ=&(PTR(table_shmat_data_t *,shmat_address,node) -> succ_key) ;
      *what_node_is_this=node ;
   }
   else
   {
      table_fill_succ_pred(
        shmat_address,
         PTR(table_shmat_index_t *,shmat_address, node) -> next_key[0] ,
         index_start,
         where_to_put_node,
         what_node_is_pred,
         where_to_put_succ,
         what_node_is_this
         ) ;
      table_fill_succ_pred(
        shmat_address,
         PTR(table_shmat_index_t *,shmat_address, node) -> next_key[1] ,
         index_start,
         *where_to_put_succ,
         *what_node_is_this,
         where_to_put_succ,
         what_node_is_this
         ) ;
   } /* endif */
}

static int table_close_x(
  cluster_descriptor * cluster
) {
   void * shmat_address ;
   table_shmat_head_t *t_h ;
   unsigned int current_record_offset ;
   unsigned int index_start ;
   unsigned int data_offset ;
   void * current_record ;
   unsigned int record_length ;
   unsigned int free_node_offset ;
   unsigned int end_of_data ;

   shmat_address = cluster -> shmat_address ;
   t_h = (table_shmat_head_t *) shmat_address ;
   /*
    * Arrange the index so that an index element does not cross cache-line boundaries;
    * this way each turn around the scanning loop when reading the table will touch 1
    * cache line only. This relies on sizeof(table_shmat_index_t) being a power of 2
    * not greater than 2**4=16, but this restriction is difficult to express well to
    * the compiler.
    */
   end_of_data = cluster -> next_free_offset ;
   index_start = ROUND_UP ( end_of_data, 16 )  ;
   t_h -> index_start = index_start ;

   data_offset = t_h -> data_offset ;

   /*
    * Walk the table in entry order, to build the index
    */
   free_node_offset = index_start ;

   current_record_offset = sizeof(table_shmat_head_t) ;
   if (current_record_offset < end_of_data)
   {
      /*
       * There are some records in the file. Build the index and walk to link it.
       */
      do
      {
         current_record = (char *) shmat_address + current_record_offset ;
         record_length = ( (table_shmat_data_t *) current_record )-> data_length ;
         build_index (
         & free_node_offset, current_record_offset, shmat_address , end_of_data
         , t_h -> rid_size
         ) ;
         current_record_offset += data_offset + ROUND_UP(record_length,4) ;

      } while (current_record_offset < end_of_data) /* enddo */ ;

      /*
       * Walk the index to fill in the prev/succ record fields
       */
      {
        unsigned int place_for_first ;
        unsigned int * place_for_last ;
        unsigned int where_is_last ;
        table_fill_succ_pred(shmat_address, t_h -> root_node, index_start, &place_for_first, 0xffffffff , &place_for_last, &where_is_last) ;
        *place_for_last = 0xffffffff ;
      }

   } /* endif */

   /*
    * Fill in the 'magic number' in the table header, to indicate a complete file
    */
   t_h -> magic = table_correct_magic ;

   /*
    * Walk the index to display the structure
    */

  if (diag_show)
  {
   table_show(
     shmat_address,
     t_h -> rid_size,
     0,
     t_h -> root_node,
     index_start
     ) ;
  } /* endif */



   close ( cluster -> fd ) ;
   return PAC_TABLE_NOERROR ;
}

int table_cluster_define(
  PAC_TABLE_WRITER * table,
  unsigned int cluster_number,
  const void * highest_rid
) {
   if (table_just_defined == table -> state)
   {
     int open_rc = table_open_primary(& (table -> primary), table -> table_name,
            table -> rid_length, table_correct_version_primary ) ;
     if (PAC_TABLE_NOERROR != open_rc)
     {
        return open_rc ;
     } /* endif */
     table -> state = table_clustered ;
   }
   else if ( table_clustered != table -> state ) return PAC_TABLE_NOT_CLUSTERED ;
   else {
      int table_close_rc = table_close_x( &(table -> current_secondary) ) ;
      if (PAC_TABLE_NOERROR != table_close_rc)
      {
         return table_close_rc ;
      } /* endif */
   }
   {
     int open_sec_rc = table_open_secondary(&(table -> current_secondary), table -> table_name,
          table -> rid_length, cluster_number) ;
     if (PAC_TABLE_NOERROR != open_sec_rc)
     {
        return open_sec_rc ;
     } /* endif */
   }
   return table_write_x(&( table -> primary), highest_rid, &( cluster_number ) , sizeof(cluster_number) ) ;

}

int table_close(
  PAC_TABLE_WRITER * table
) {
   switch (table -> state)
   {
      case table_just_defined : return PAC_TABLE_NOERROR ; /* no file created, though ... */
      case table_one_file :
         return table_close_x(&(table -> primary)) ;
      default : /* i.e. clustered or autoclustered */
         {
           int close_rc = table_close_x(&(table -> current_secondary)) ;
           if (PAC_TABLE_NOERROR != close_rc) return close_rc ;
         }
         return table_close_x(&(table -> primary)) ;
   } /* endswitch */
}

