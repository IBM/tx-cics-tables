/*
 * IBM Internal Use Only           T J C Ward       28 September 1993
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */

/*
 * 96/04/04 tjcw Corrected fault whereby keys with first word of 0
 *               would be found when they should not be.
 * 97/02/19 tjcw Corrected fault whereby file name in side-info was not
 *               updated. This would result in trying to read data from the
 *               wrong file. It would affect cases where the file name passed
 *               to "read" could vary from one call to the next
 * 1998/02/17 tjcw Major overhaul to allow "secondary cluster" tables, and
 *               to arrange that a limited number of open AIX files can
 *               support an unlimited number of logically open files
 *
 */

/*
 * The structures involved in reading from a table are ...
 *
 * 1) Name hash table. This maps efficiently from a table name to the internal
 *    table number (i.e. index to the file table)
 *
 * 2) File table. This has one entry for each file which has been quoted to
 *    'PAC_TABLE_READ'. The one entry does duty for a primary and all its
 *    secondaries.
 *
 * 3) Open file table. This has one entry for each file which is currently
 *    open, i.e. has an AIX file descriptor. It is organised as 64 lines
 *    of 4 entries per line. It is an 'inverted' table, in the sense that
 *    it is used to map from the file number (index to table 2) and cluster
 *    number, to the AIX file descriptor; you do this mapping by looking at
 *    all the entries in the 'right' line; and it is possible for the table
 *    to reply 'no' (in which case you conclude that the file is not
 *    currently open).
 *
 * 4) Attached file table. This has one entry for each file which is
 *    currently mapped to memory. It is organised as 1 line of
 *    4 entries. Again, it is an 'inverted' table.
 *
 * Tables (1) and (2) are extended as files are seen for the first time.
 *
 * Lines within table (3) are reorganised as they are used, to keep recently-used
 * files at the front of the line; when a file 'drops off' the end of the line, it
 * is closed, to control the number of file descriptors used.
 *
 * The line of table (4) is reorganised on the same principle.
 *
 * The purpose of table (4) is to minimise the number of 'shmat' and 'shmdt'
 * system calls which are made; these are 'moderately cheap' system calls.
 *
 * The purpose of table (3) is to minimise the number of 'close' and 'open'
 * system calls which are made; these are 'moderately expensive' system calls.
 *
 * There is a hook back, to prevent "close"ing a file which is currently
 * "attach"ed.
 *
 * Successive clusters go into successive lines of the open file table; that is,
 * if you sequential-read the whole of one table which is organised as a
 * primary and 255 secondaries numbered 0..254, then you will fill the whole
 * open file table with file descriptors and you will implicitly close all
 * file descriptors for files associated with other tables.
 *
 * If a table is not split primary/secondary, reading a record is straightforward.
 * If it is split, the code first reads the primary file to see which secondary file
 * should have the record; then it reads the appropriate secondary. The access method
 * supports "READ KEY GTEQ"; so it is possible that the record will actually be in a
 * different (succeeding) secondary, or even in one after that if there are 'empty'
 * secondaries.
 */
/*
 * Read a record from a data table
 *
 */

#include <memory.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ldr.h>

#include <table_rcode.h>
#include <table_read_int.h>

#include <hibit.h>

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
   } ;

static int my_memcmp(const char * s1, const char * s2, unsigned int length)
{
   int d=s1[0]-s2[0] ;
   int x ;
   for (x=0; x<length; x+=1)
   {
      int d=s1[x]-s2[x] ;
      if (0 != d) return d ;
   } /* endfor */
   return 0 ;
}

/*
 * Read from a table when it is correctly SHMATted already
 */
#define PTR(type, base, offset) ((type) (((char *) base) + offset ))
#define VAL(type, base, field) (((type *) base) -> field)

static int table_read_rrn_direct (
              _TABLE_RESPONSE_T  * table_response,
              unsigned int rrn ,
              const void * shmat_address
) {
   const table_shmat_head_t *t_h = (const table_shmat_head_t *)shmat_address ;
   unsigned int data_offset = t_h -> data_offset ;
   table_response -> field_length = PTR(table_shmat_data_t *, shmat_address, rrn) -> data_length ;
   table_response -> actual_rid = PTR(table_shmat_rid_t *,shmat_address,rrn) -> rid_field ;
   table_response -> field_addr = PTR(void *,shmat_address, rrn+data_offset) ;
   table_response -> exact_rid = 1 ;
   table_response -> next_record_index = PTR(table_shmat_data_t *, shmat_address, rrn) -> succ_key ;
   return PAC_TABLE_NOERROR ;
}

static int table_read_direct (
              _TABLE_RESPONSE_T  * table_response,
              const void * rid ,
              const void * shmat_address
) {
   const table_shmat_head_t * t_h ;
   unsigned int version ;
   unsigned int index_start ;
   int rid_size ;
   int data_offset ;
   const char * new_rid ;
   unsigned int root_node ;
   unsigned int cand_node ;
   table_shmat_index_t * cand_ptr ;
   unsigned int rid_x ;
   unsigned int cand_byte ;
   unsigned int new_byte ;
   unsigned int rid_xor ;
   unsigned int rid_y ;
   unsigned int split_ofs ;
   unsigned int succ_split ;
   unsigned int has_split ;
   table_shmat_index_t * split_ptr ;
   char * cand_rid ;
   unsigned int split_key_byte ;
   unsigned int split_key_shift ;
   unsigned int key_byte ;
   unsigned int split_key ;
   unsigned int split_x ;
   unsigned int cand_key_byte ;
   unsigned int cand_key_shift ;

   unsigned int * cand_rid_int ;
   const unsigned int * new_rid_int ;
   unsigned int rid_size_int ;
   unsigned int rid_x_int ;
   unsigned int cand_int ;
   unsigned int new_int ;

   DIAG(diag_vmm,(" Looking for RID address 0x%08x in memory 0x%08x\n",rid,shmat_address)) ;

   new_rid = (const char *) rid ;
   t_h = (const table_shmat_head_t *)shmat_address ;


   index_start = t_h -> index_start ;
   root_node = t_h -> root_node ;
   cand_node = root_node ;
   rid_size = t_h -> rid_size ;
   data_offset = t_h -> data_offset ;

   cand_ptr = PTR(table_shmat_index_t *,shmat_address,cand_node) ;

   version = t_h -> version ;
   table_response -> version = version ;
   if (table_correct_magic != t_h -> magic)
   {
      return PAC_TABLE_NOT_TABLE ;
   } /* endif */
   if (table_correct_version != version && table_correct_version_primary != version)
   {
      return PAC_TABLE_WRONG_VERSION ;
   } /* endif */

   if (0 == root_node)
   {
      /*
       * Seriously degenerate case ... no records in file
       */
      table_response -> field_length = 0 ;
      table_response -> actual_rid = NULL ;
      table_response -> field_addr = NULL ;
      table_response -> exact_rid = 0 ;
      table_response -> next_record_index = 0xffffffff ;
      return PAC_TABLE_NOTFND ;
   } /* endif */

   cand_key_byte = cand_ptr -> key_byte ;
   cand_key_shift = cand_ptr -> key_shift ;
   while (cand_node >= index_start)
   {
        cand_node = cand_ptr -> next_key [
          ( new_rid [ cand_key_byte ] >> cand_key_shift )
            & 1
          ] ;
        cand_ptr = PTR(table_shmat_index_t *,shmat_address,cand_node) ;
        cand_key_byte = cand_ptr -> key_byte ;
        cand_key_shift = cand_ptr -> key_shift ;
   } /* endwhile */
   /*
    * Check to see whether the RID matched, if not where the mismatch was
    */

   cand_rid = PTR(table_shmat_rid_t *,shmat_address,cand_node) -> rid_field ;
   /*
    * This will work with either endianism
    */
   cand_rid_int = (unsigned int *) cand_rid ;
   new_rid_int = (unsigned int *) new_rid ;
   rid_size_int = rid_size / sizeof(int)  ;
   for (rid_x_int = 0 ; rid_x_int < rid_size_int; rid_x_int += 1)
   {
      cand_int = cand_rid_int [ rid_x_int ] ;
      new_int = new_rid_int [ rid_x_int ] ;
      if (cand_int != new_int) break ;
   } /* endfor */

   for (rid_x = rid_x_int * sizeof(int) ; rid_x < rid_size; rid_x += 1)
   {
      cand_byte = cand_rid [ rid_x ] ;
      new_byte = new_rid [ rid_x ] ;
      if (cand_byte != new_byte) break ;
   } /* endfor */

   if (rid_x >= rid_size)
   {
     /*
      * Exact key match
      */
     table_response -> field_length = PTR(table_shmat_data_t *, shmat_address, cand_node) -> data_length ;
     table_response -> actual_rid = cand_rid ;
     table_response -> field_addr = PTR(void *,shmat_address, cand_node+data_offset) ;
     table_response -> exact_rid = 1 ;
     table_response -> next_record_index = PTR(table_shmat_data_t *, shmat_address, cand_node) -> succ_key ;
     return PAC_TABLE_NOERROR ;
   }
   else
   {
      /*
       * Inexact key match. Back up and find the next larger key.
       */
       rid_xor = cand_byte ^ new_byte ;
       rid_y = hibit[rid_xor] ;

       /*
        * Scan from the root again to find the split point
        */

       split_ofs = root_node ;

       if (root_node < index_start)
       {
          /*
           * Degenerate case; only one record in the file
           */
          if (cand_byte > new_byte)
          {
            table_response -> field_length = PTR(table_shmat_data_t *, shmat_address, cand_node) -> data_length ;
            table_response -> actual_rid = cand_rid ;
            table_response -> field_addr = PTR(void *,shmat_address, cand_node+data_offset) ;
            table_response -> exact_rid = 0 ;
            table_response -> next_record_index = PTR(table_shmat_data_t *, shmat_address, cand_node) -> succ_key ;
            return PAC_TABLE_NOERROR ;
          }
          else
          {
            table_response -> field_length = 0 ;
            table_response -> actual_rid = NULL ;
            table_response -> field_addr = NULL ;
            table_response -> exact_rid = 0 ;
            table_response -> next_record_index = 0xffffffff ;
            return PAC_TABLE_NOTFND ;
          } /* endif */

       }

       if (cand_byte > new_byte)
       {
         split_ptr = PTR(table_shmat_index_t *,shmat_address, split_ofs) ;
         split_key_byte = split_ptr -> key_byte ;
         split_key_shift = split_ptr -> key_shift ;
         do
         {

           split_x = ( new_rid [ split_key_byte ] >> split_key_shift ) & 1 ;
           /*
            * If this split in the tree is 'past' the difference bit, the
            * rest of the scan does not apply; skip out.
            */
           if (split_key_byte > rid_x
               ||
              (split_key_byte == rid_x && split_key_shift < rid_y)
           ) {
             break ;
           }

           split_ofs = split_ptr -> next_key [ split_x ] ;
           split_ptr = PTR(table_shmat_index_t *,shmat_address, split_ofs) ;
           split_key_byte = split_ptr -> key_byte ;
           split_key_shift = split_ptr -> key_shift ;
         } while ( split_ofs >= index_start ) ;
        /*
         * 'this' branch of the tree contains keys
         * which exceed the one searched for. Check this.
         */
         succ_split = split_ofs ;
       }
       else
       {
         has_split = 0 ;
         split_ptr = PTR(table_shmat_index_t *,shmat_address, split_ofs) ;
         split_key_byte = split_ptr -> key_byte ;
         split_key_shift = split_ptr -> key_shift ;
         do
         {
           DIAG(diag_scan,(" Split at %02x bit %i\n",split_key_byte, split_key_shift)) ;

           split_x = ( new_rid [ split_key_byte ] >> split_key_shift ) & 1 ;
           /*
            * If this split in the tree is 'past' the difference bit, the
            * rest of the scan does not apply; skip out.
            */
           if (split_key_byte > rid_x
               ||
              (split_key_byte == rid_x && split_key_shift < rid_y)
           ) {
             break ;
           }

           split_ofs = split_ptr -> next_key [ split_x ] ;
           if (0 == split_x )
           {
              /*
               * There is a branch of the tree containing keys which are
               * greater than the one searched for. Remember it, we will need
               * to find the smallest key in it ... unless we find another
               * branch containing smaller keys also satisfying this criterion.
               */
              succ_split = split_ptr -> next_key [ 1 ] ;
              has_split = 1 ;
              DIAG(diag_scan,(" Will split from byte %i bit %i\n", split_key_byte, split_key_shift)) ;
           } /* endif */
           split_ptr = PTR(table_shmat_index_t *,shmat_address, split_ofs) ;
           split_key_byte = split_ptr -> key_byte ;
           split_key_shift = split_ptr -> key_shift ;

         } while ( split_ofs >= index_start ) ;
         if (0 == has_split)
         {
          /*
           * We are off the end of the table
           * Bomb out with 'record not found'
           */
          table_response -> field_length = 0 ;
          table_response -> actual_rid = NULL ;
          table_response -> field_addr = NULL ;
          table_response -> exact_rid = 0 ;
          table_response -> next_record_index = 0xffffffff ;
          return PAC_TABLE_NOTFND ;
         }
       } /* endif */



       /*
        * We have found the split. Follow the chain downwards
        */
       while (succ_split >= index_start)
       {
           succ_split = PTR(table_shmat_index_t *,shmat_address, succ_split)
                         -> next_key [ 0 ] ;
       } /* endwhile */
       table_response -> field_length = PTR(table_shmat_data_t *, shmat_address, succ_split) -> data_length ;
       table_response -> actual_rid = PTR(table_shmat_rid_t *,shmat_address,succ_split) -> rid_field ;
       table_response -> field_addr = PTR(void *,shmat_address, succ_split+data_offset) ;
       table_response -> exact_rid = 0 ;
       table_response -> next_record_index = PTR(table_shmat_data_t *, shmat_address, succ_split) -> succ_key ;
       return PAC_TABLE_NOERROR ;

   } /* endif */

}


static unsigned int make_file_table_index(unsigned int file_descriptor_and_file_index_hi,
                               unsigned int open_table_index)
{
   return
        (
            ( file_descriptor_and_file_index_hi << open_file_index_bits )
            |
            ( open_table_index & (open_file_index_depth-1) )
        )
      &
        ( all_file_index_depth-1 ) ;
}

static unsigned int file_table_open_table_bits ( unsigned int file_table_index )
{
   return file_table_index >> open_file_index_bits ;
}

static unsigned int open_table_file_table_bits ( unsigned int file_descriptor_and_file_index_hi )
{
   return file_descriptor_and_file_index_hi & ( ( 1 << open_table_file_index_bits ) - 1 ) ;
}

static unsigned int make_file_descriptor_and_file_index_hi(
  unsigned int file_descriptor,
  unsigned int file_index
) {
   return ( file_descriptor << open_table_file_index_bits )
        | ( ( file_index >> open_file_index_bits )
          & ( ( 1 << open_table_file_index_bits ) - 1 )
          ) ;
 }
static unsigned int open_table_file_descriptor ( unsigned int file_descriptor_and_file_index_hi )
{
   return file_descriptor_and_file_index_hi >> open_table_file_index_bits ;
}

static unsigned int seg_and_file(void * address, unsigned int file_index)
{
   return ( ((unsigned int) address) & 0xf0000000) | ( file_index & 0x0fffffff) ;
}

static int table_attach_and_read (
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              table_track_t * table_track ,
              unsigned int file_descriptor_and_file_index_hi,
              void * suitable_shmat_address,
              unsigned int file_table_index,
              unsigned int cluster_number
) {
   unsigned int file_descriptor = open_table_file_descriptor(file_descriptor_and_file_index_hi) ;
   void * actual_address ;
   DIAG(diag_vmm,(" Attaching file 0x%08x to address 0x%08x\n",file_descriptor, suitable_shmat_address)) ;
   actual_address = shmat(file_descriptor, suitable_shmat_address, SHM_MAP | SHM_RDONLY) ;
   if (actual_address != suitable_shmat_address)
   {
      return PAC_TABLE_SHMAT_FAILED ;
   } /* endif */
   table_track -> attached.page[0].line[0].segment_number_and_file_number = seg_and_file(actual_address,file_table_index) ;
   table_track -> attached.page[0].line[0].cluster_number                 = cluster_number ;
   return table_read_direct(table_response, rid, actual_address) ;
}

/*
 * Test whether a segment is currently unattached.
 * Note .. element 0 is not checked because by this stage we have just done a 'shmdt' of this one.
 */
static int is_unattached(table_track_t * table_track, unsigned int file_table_index, unsigned int cluster_number)
{

   unsigned int afsfn1 = table_track -> attached.page[0].line[1].segment_number_and_file_number ;
   unsigned int clust1 = table_track -> attached.page[0].line[1].cluster_number ;
   unsigned int afsfn2 = table_track -> attached.page[0].line[2].segment_number_and_file_number ;
   unsigned int clust2 = table_track -> attached.page[0].line[2].cluster_number ;
   unsigned int afsfn3 = table_track -> attached.page[0].line[3].segment_number_and_file_number ;
   unsigned int clust3 = table_track -> attached.page[0].line[3].cluster_number ;

   return ( cluster_number != clust1 || (afsfn1 & 0x0fffffff) != file_table_index)
       && ( cluster_number != clust2 || (afsfn2 & 0x0fffffff) != file_table_index)
       && ( cluster_number != clust3 || (afsfn3 & 0x0fffffff) != file_table_index) ;
}

static int table_open ( const char * file_name, unsigned int cluster_number )
{
   char full_path_name[1000] ;
   int open_rc ;

   /*
    * Construct the proper file name
    */
   sprintf(full_path_name, ( -1 == (int)cluster_number )
                             ? TABLE_DIRECTORY "%s"
                             : TABLE_DIRECTORY "%s" TABLE_CLUSTER_DIRECTORY "%s.%08x"
                             , file_name, file_name, cluster_number ) ;
   DIAG(diag_file,(" Opening file \"%s\" cluster 0x%08x full name \"%s\"\n",file_name, cluster_number,full_path_name)) ;
   /*
    * See if it can be opened
    */
   open_rc=open(full_path_name, O_RDONLY) ;

   DIAG(diag_file,(" Open result is %i, errno=%i\n",open_rc, (open_rc < 0) ? errno : 0)) ;

   return open_rc ;
}

/*
 * Read from a file, where another file has been pushed off the end to make space
 */
static int close_open_and_read(
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              table_track_t * table_track ,
              unsigned int file_table_index,
              unsigned int cluster_number ,
              void * suitable_shmat_address,
              unsigned int open_file_line_index
              ) {
  unsigned int fdfnh0 = table_track -> opened.page[open_file_line_index].line[0].file_descriptor_and_file_index_hi ;
  unsigned int clust0 = table_track -> opened.page[open_file_line_index].line[0].cluster_number ;
  unsigned int fdfnh2 = table_track -> opened.page[open_file_line_index].line[2].file_descriptor_and_file_index_hi ;
  unsigned int clust2 = table_track -> opened.page[open_file_line_index].line[2].cluster_number ;
  unsigned int fdfnh3 = table_track -> opened.page[open_file_line_index].line[3].file_descriptor_and_file_index_hi ;
  unsigned int clust3 = table_track -> opened.page[open_file_line_index].line[3].cluster_number ;

  unsigned int victim_file=
   is_unattached(table_track, make_file_table_index(fdfnh0,open_file_line_index-clust0),clust0) ? 0
   : is_unattached(table_track, make_file_table_index(fdfnh3,open_file_line_index-clust3),clust3) ? 3
   : is_unattached(table_track, make_file_table_index(fdfnh2,open_file_line_index-clust2),clust2) ? 2
   : 1 ;

  unsigned int fdfnh = table_track -> opened.page[open_file_line_index].line[victim_file].file_descriptor_and_file_index_hi ;
  int open_rc ;
  unsigned int new_fdfnh ;

  DIAG(diag_file,(" Slot %i is not attached\n",victim_file)) ;
  if (0xffffffff != fdfnh)
  {
    int close_rc = close(open_table_file_descriptor(fdfnh)) ;
    DIAG(diag_file,(" Closing entry 0x%08x\n",fdfnh)) ;
    if (0 != close_rc)
    {
     return PAC_TABLE_CLOSE_FAILED ;
    } /* endif */
  } /* endif */
  open_rc = table_open(table_track -> all.page[file_table_index].name, cluster_number) ;
  if (open_rc < 0 )
  {
     return PAC_TABLE_NOTFND ;
  } /* endif */
  DIAG(diag_file,(" Opened as file descriptor %i\n",open_rc)) ;
  new_fdfnh = make_file_descriptor_and_file_index_hi(open_rc,file_table_index) ;
  table_track -> opened.page[open_file_line_index].line[victim_file].file_descriptor_and_file_index_hi= new_fdfnh ;
  table_track -> opened.page[open_file_line_index].line[victim_file].cluster_number                    = cluster_number ;

   DIAG(diag_file,(" Revised open table at index=%i is\n"
                   "   (0x%08x 0x%08x 0x%08x 0x%08x)\n"
                   "   (0x%08x 0x%08x 0x%08x 0x%08x)\n",
                   open_file_line_index,
                   table_track -> opened.page[open_file_line_index].line[0].file_descriptor_and_file_index_hi,
                   table_track -> opened.page[open_file_line_index].line[1].file_descriptor_and_file_index_hi,
                   table_track -> opened.page[open_file_line_index].line[2].file_descriptor_and_file_index_hi,
                   table_track -> opened.page[open_file_line_index].line[3].file_descriptor_and_file_index_hi,
                   table_track -> opened.page[open_file_line_index].line[0].cluster_number,
                   table_track -> opened.page[open_file_line_index].line[1].cluster_number,
                   table_track -> opened.page[open_file_line_index].line[2].cluster_number,
                   table_track -> opened.page[open_file_line_index].line[3].cluster_number
                   )) ;

  return table_attach_and_read(table_response,rid,table_track,new_fdfnh,suitable_shmat_address,file_table_index, cluster_number) ;

}
/*
 * Read from a file which is already in the file table
 */
static int table_read_existing_file (
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              table_track_t * table_track ,
              unsigned int file_table_index,
              unsigned int cluster_number,
              void * suitable_shmat_address
) {
   /*
    * See if the file is currently open, as a 'primary' file
    */
   unsigned int open_file_line_index = ( file_table_index + cluster_number ) & ( open_file_index_depth - 1) ;

   unsigned int fdfnh0 = table_track -> opened.page[open_file_line_index].line[0].file_descriptor_and_file_index_hi ;
   unsigned int clust0 = table_track -> opened.page[open_file_line_index].line[0].cluster_number ;
   unsigned int fdfnh1 = table_track -> opened.page[open_file_line_index].line[1].file_descriptor_and_file_index_hi ;
   unsigned int clust1 = table_track -> opened.page[open_file_line_index].line[1].cluster_number ;

   unsigned int ftob = file_table_open_table_bits(file_table_index) ;

   DIAG(diag_file,(" Looking in open table index=%i\n"
                   "   (0x%08x 0x%08x 0x%08x 0x%08x)\n"
                   "   (0x%08x 0x%08x 0x%08x 0x%08x)\n",
                   open_file_line_index,
                   table_track -> opened.page[open_file_line_index].line[0].file_descriptor_and_file_index_hi,
                   table_track -> opened.page[open_file_line_index].line[1].file_descriptor_and_file_index_hi,
                   table_track -> opened.page[open_file_line_index].line[2].file_descriptor_and_file_index_hi,
                   table_track -> opened.page[open_file_line_index].line[3].file_descriptor_and_file_index_hi,
                   table_track -> opened.page[open_file_line_index].line[0].cluster_number,
                   table_track -> opened.page[open_file_line_index].line[1].cluster_number,
                   table_track -> opened.page[open_file_line_index].line[2].cluster_number,
                   table_track -> opened.page[open_file_line_index].line[3].cluster_number
                   )) ;

   if (cluster_number == clust0 && open_table_file_table_bits(fdfnh0) == ftob )
   {
     /*
      * It was open in pole position
      */
     DIAG(diag_file,(" Found in slot 0\n")) ;
     return table_attach_and_read(table_response,rid,table_track,fdfnh0,suitable_shmat_address,file_table_index, cluster_number) ;
   }
   else
   {
     /*
      * Not pole. Start shuffling things down.
      */
     unsigned int fdfnh2 ;
     unsigned int clust2 ;
     table_track -> opened.page[open_file_line_index].line[1].file_descriptor_and_file_index_hi =fdfnh0;
     table_track -> opened.page[open_file_line_index].line[1].cluster_number                     =clust0;
     fdfnh2 = table_track -> opened.page[open_file_line_index].line[2].file_descriptor_and_file_index_hi ;
     clust2 = table_track -> opened.page[open_file_line_index].line[2].cluster_number ;
     if (cluster_number == clust1 && open_table_file_table_bits(fdfnh1) == ftob )
     {
       /*
        * Number 2 on the grid. Swap it to pole position
        */
       DIAG(diag_file,(" Found in slot 1\n")) ;
       table_track -> opened.page[open_file_line_index].line[0].file_descriptor_and_file_index_hi =fdfnh1;
       table_track -> opened.page[open_file_line_index].line[0].cluster_number                     =clust1;
       return table_attach_and_read(table_response,rid,table_track,fdfnh1,suitable_shmat_address,file_table_index, cluster_number) ;
     }
     else
     {
       /*
        * Not number 2. More shuffling.
        */
       unsigned int fdfnh3 ;
       unsigned int clust3 ;
       table_track -> opened.page[open_file_line_index].line[2].file_descriptor_and_file_index_hi =fdfnh1;
       table_track -> opened.page[open_file_line_index].line[2].cluster_number                     =clust1;
       fdfnh3 = table_track -> opened.page[open_file_line_index].line[3].file_descriptor_and_file_index_hi ;
       clust3 = table_track -> opened.page[open_file_line_index].line[3].cluster_number ;
       if (cluster_number == clust2 && open_table_file_table_bits(fdfnh2) == ftob )
       {
         /*
          * Number 3 on the grid. Rotate it up to pole.
          */
         DIAG(diag_file,(" Found in slot 2\n")) ;
         table_track -> opened.page[open_file_line_index].line[0].file_descriptor_and_file_index_hi =fdfnh2;
         table_track -> opened.page[open_file_line_index].line[0].cluster_number                     =clust2;
         return table_attach_and_read(table_response,rid,table_track,fdfnh2,suitable_shmat_address,file_table_index, cluster_number) ;
       }
       else
       {
         /*
          * Not number 3. More shuffling.
          */
         table_track -> opened.page[open_file_line_index].line[3].file_descriptor_and_file_index_hi =fdfnh2;
         table_track -> opened.page[open_file_line_index].line[3].cluster_number                     =clust2;
         /*
          * Promote the bottom one. This may be temporary; if it's not the one we want and it is
          * not memory mapped, it will be swapped out.
          */
         table_track -> opened.page[open_file_line_index].line[0].file_descriptor_and_file_index_hi =fdfnh3;
         table_track -> opened.page[open_file_line_index].line[0].cluster_number                     =clust3;
         if (cluster_number == clust3 && open_table_file_table_bits(fdfnh3) == ftob )
         {
           /*
            * Number 4 on the grid. Rotate it up to pole.
            */
           DIAG(diag_file,(" Found in slot 3\n")) ;
           return table_attach_and_read(table_response,rid,table_track,fdfnh3,suitable_shmat_address,file_table_index, cluster_number) ;
         }
         else
         {
           /*
            * Not on the grid. If the bottom one is attached to memory, replace it in the grid
            * and find one which is not attached (there must be one). Close the victim file,
            * open the new file, and go.
            */
           DIAG(diag_file,(" Not found\n")) ;
           return close_open_and_read(table_response,rid,table_track,file_table_index,cluster_number,suitable_shmat_address,open_file_line_index) ;
         } /* endif */
       } /* endif */
     } /* endif */
   } /* endif */
   /*
    * All bases have been covered by here
    */
}


static unsigned int attached_table_file_table_bits ( unsigned int segment_number_and_file_number )
{
   return segment_number_and_file_number & ( all_file_index_depth - 1 ) ;
}

static void * attached_table_segment_address ( unsigned int segment_number_and_file_number )
{
   return (void *) ( segment_number_and_file_number & 0xf0000000 ) ;
}

static int table_detach_then_read_existing_file (
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              table_track_t * table_track ,
              unsigned int file_table_index,
              unsigned int cluster_number,
              unsigned int detach_seg_and_file
) {
   void * suitable_shmat_address=attached_table_segment_address(detach_seg_and_file) ;
   int shmdt_result ;
   DIAG(diag_vmm,(" Detaching from address 0x%08x\n",suitable_shmat_address)) ;
   shmdt_result=shmdt(suitable_shmat_address) ;
   if (0 != shmdt_result )
   {
      return PAC_TABLE_SHMDT_FAILED ;
   } /* endif */
   /*
    * And go find the table which is wanted
    */
   return table_read_existing_file(table_response,rid,table_track,file_table_index,cluster_number,suitable_shmat_address) ;
}

/*
 * Read a record from a file which is in the file table, but may not be currently attached and
 * may not even be open.
 */
static int table_read_fixup_shmat (
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              unsigned int file_table_index,
              unsigned int cluster_number,
              table_track_t * table_track
              )
{
   /*
    * Look in the attached-file associative array
    */
   unsigned int afsfn0 = table_track -> attached.page[0].line[0].segment_number_and_file_number ;
   unsigned int clust0 = table_track -> attached.page[0].line[0].cluster_number ;
   unsigned int afsfn1 = table_track -> attached.page[0].line[1].segment_number_and_file_number ;
   unsigned int clust1 = table_track -> attached.page[0].line[1].cluster_number ;


   DIAG(diag_file,(" Looking in attach table for file index=%i\n"
                   "   (0x%08x 0x%08x 0x%08x 0x%08x)\n"
                   "   (0x%08x 0x%08x 0x%08x 0x%08x)\n",
                   file_table_index,
                   table_track -> attached.page[0].line[0].segment_number_and_file_number,
                   table_track -> attached.page[0].line[1].segment_number_and_file_number,
                   table_track -> attached.page[0].line[2].segment_number_and_file_number,
                   table_track -> attached.page[0].line[3].segment_number_and_file_number,
                   table_track -> attached.page[0].line[0].cluster_number,
                   table_track -> attached.page[0].line[1].cluster_number,
                   table_track -> attached.page[0].line[2].cluster_number,
                   table_track -> attached.page[0].line[3].cluster_number)) ;

   if (cluster_number == clust0 && attached_table_file_table_bits(afsfn0) == file_table_index)
   {
      /*
       * Attached in pole position
       */
     DIAG(diag_file,(" Found in slot 0\n")) ;
     return table_read_direct(table_response, rid, attached_table_segment_address(afsfn0)) ;
   }
   else
   {
      /*
       * Not in pole position, start moving things down the grid
       */
      table_track -> attached.page[0].line[1].segment_number_and_file_number = afsfn0 ;
      table_track -> attached.page[0].line[1].cluster_number = clust0 ;
      if (cluster_number == clust1 && attached_table_file_table_bits(afsfn1) == file_table_index)
      {
        /*
         * Attached in number 2 position.
         */
        table_track -> attached.page[0].line[0].segment_number_and_file_number = afsfn1 ;
        table_track -> attached.page[0].line[0].cluster_number = clust1 ;
        DIAG(diag_file,(" Found in slot 1\n")) ;
        return table_read_direct(table_response, rid, attached_table_segment_address(afsfn1)) ;
      }
      else
      {
        /*
         * Not in number 2 position, keep moving down
         */
        unsigned int afsfn2 ;
        unsigned int clust2 ;
        unsigned int afsfn3 ;
        unsigned int clust3 ;
        afsfn2 = table_track -> attached.page[0].line[2].segment_number_and_file_number ;
        clust2 = table_track -> attached.page[0].line[2].cluster_number ;
        afsfn3 = table_track -> attached.page[0].line[3].segment_number_and_file_number ;
        clust3 = table_track -> attached.page[0].line[3].cluster_number ;
        table_track -> attached.page[0].line[2].segment_number_and_file_number = afsfn1 ;
        table_track -> attached.page[0].line[2].cluster_number = clust1 ;
        if (cluster_number == clust2 && attached_table_file_table_bits(afsfn2) == file_table_index)
        {
          /*
           * Attached in number 3 position.
           */
          table_track -> attached.page[0].line[0].segment_number_and_file_number = afsfn2 ;
          table_track -> attached.page[0].line[0].cluster_number = clust2 ;
          DIAG(diag_file,(" Found in slot 2\n")) ;
          return table_read_direct(table_response, rid, attached_table_segment_address(afsfn2)) ;
        }
        else
        {
           /*
            * Not in number 3 position. Last chance.
            */
          table_track -> attached.page[0].line[3].segment_number_and_file_number = afsfn2 ;
          table_track -> attached.page[0].line[3].cluster_number = clust2 ;
          if (cluster_number == clust3 && attached_table_file_table_bits(afsfn3) == file_table_index)
          {
            /*
             * Attached in number 4 position
             */
            table_track -> attached.page[0].line[0].segment_number_and_file_number = afsfn3 ;
            table_track -> attached.page[0].line[0].cluster_number = clust3 ;
            DIAG(diag_file,(" Found in slot 3\n")) ;
            return table_read_direct(table_response, rid, attached_table_segment_address(afsfn3)) ;
          }
          else
          {
             /*
              * Not attached at all.
              */
             void * suitable_shmat_address = attached_table_segment_address(afsfn3) ;
             DIAG(diag_file,(" Not found in attach table\n")) ;
             /*
              * Mark up the associative array to indicate that the memory is no longer attached
              */
             table_track -> attached.page[0].line[0].segment_number_and_file_number = seg_and_file(suitable_shmat_address,0xffffffff) ;
             if (not_attached_file_mask == (afsfn3 & not_attached_file_mask))
             {
                return table_read_existing_file(table_response,rid,table_track,file_table_index,cluster_number,suitable_shmat_address) ;
             }
             else
             {
                return table_detach_then_read_existing_file(table_response,rid,table_track,file_table_index,cluster_number,afsfn3) ;
             } /* endif */
          } /* endif */
        } /* endif */
      } /* endif */
   } /* endif */
}

/*
 * Read a file by byte number, when it is known to be attached, and sure to be in the
 * number 2 position
 */
static int table_read_rrn (
              _TABLE_RESPONSE_T * table_response,
              unsigned int rrn,
              unsigned int file_table_index,
              unsigned int cluster_number,
              table_track_t * table_track
              )
{
   /*
    * Look in the attached-file associative array
    */
   unsigned int afsfn1 = table_track -> attached.page[0].line[1].segment_number_and_file_number ;
   unsigned int clust1 = table_track -> attached.page[0].line[1].cluster_number ;
   unsigned int afsfn0 = table_track -> attached.page[0].line[0].segment_number_and_file_number ;
   unsigned int clust0 = table_track -> attached.page[0].line[0].cluster_number ;


   DIAG(diag_file,(" Looking in slot 1 of attach table for file index=%i\n"
                   "   (0x%08x 0x%08x 0x%08x 0x%08x)\n"
                   "   (0x%08x 0x%08x 0x%08x 0x%08x)\n",
                   file_table_index,
                   table_track -> attached.page[0].line[0].segment_number_and_file_number,
                   table_track -> attached.page[0].line[1].segment_number_and_file_number,
                   table_track -> attached.page[0].line[2].segment_number_and_file_number,
                   table_track -> attached.page[0].line[3].segment_number_and_file_number,
                   table_track -> attached.page[0].line[0].cluster_number,
                   table_track -> attached.page[0].line[1].cluster_number,
                   table_track -> attached.page[0].line[2].cluster_number,
                   table_track -> attached.page[0].line[3].cluster_number)) ;

   table_track -> attached.page[0].line[1].segment_number_and_file_number = afsfn0 ;
   table_track -> attached.page[0].line[1].cluster_number = clust0 ;
   table_track -> attached.page[0].line[0].segment_number_and_file_number = afsfn1 ;
   table_track -> attached.page[0].line[0].cluster_number = clust1 ;
   if (cluster_number == clust1 && attached_table_file_table_bits(afsfn1) == file_table_index)
   {
     /*
      * Attached in number 2 position.
      */
     DIAG(diag_file,(" Found in slot 1\n")) ;
     return table_read_rrn_direct(table_response, rrn, attached_table_segment_address(afsfn1)) ;
   }
   else
   {
      /*
       * We just scanned a secondary file (logically pushing the primary as above) but found that
       * the primary was no longer there. Hmmm.
       */
      return PAC_TABLE_ILLOGIC ;
   }
}

static void clear_open_table(open_file_page * opened)
{
   unsigned int line ;
   for (line=0; line < open_file_index_depth ; line += 1)
   {
      opened -> page[line].line[0].file_descriptor_and_file_index_hi = 0xffffffff ;
      opened -> page[line].line[1].file_descriptor_and_file_index_hi = 0xffffffff ;
      opened -> page[line].line[2].file_descriptor_and_file_index_hi = 0xffffffff ;
      opened -> page[line].line[3].file_descriptor_and_file_index_hi = 0xffffffff ;
   } /* endfor */
}


/*
 * Read from a table which is known about, according whether it has
 * secondary clusters
 */
static int table_read_may_have_secondaries (
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              unsigned int file_table_index,
              unsigned int cluster_number,
              table_track_t * table_track
) {
     int result = table_read_fixup_shmat(table_response, rid, file_table_index,cluster_number, table_track) ;
     unsigned int version = table_response -> version ;
     if (PAC_TABLE_NOERROR == result)
     {
        if (table_correct_version_primary == version)
        {
          /*
           * OK so far, but we had a table with secondaries ...
           */
          for (; ; )
          {
             unsigned int cluster_number=*(unsigned int *) (table_response -> field_addr) ;
             unsigned int next_in_primary ;
             unsigned int version_secondary ;
             DIAG(diag_cluster,(" Record is in secondary cluster %i\n",cluster_number)) ;
             next_in_primary = table_response -> next_record_index ;
             result = table_read_fixup_shmat(table_response, rid, file_table_index,cluster_number, table_track) ;
             version_secondary = table_response -> version ;
             if (PAC_TABLE_NOERROR == result && table_correct_version != version_secondary )
             {
                result = PAC_TABLE_TERTIARY ;
                break ;
             } /* endif */
             if (PAC_TABLE_NOTFND != result) break ;
             /*
              * We ran off the end of a secondary cluster. Look in the primary for the next secondary cluster
              * and go round again.
              */
             if (0xffffffff == next_in_primary) break ; /* Really was off the end of the file */
             DIAG(diag_cluster,(" Not in that secondary cluster, trying the next one\n")) ;
             result = table_read_rrn(table_response, next_in_primary, file_table_index, 0xffffffff, table_track) ;
             if (PAC_TABLE_NOERROR != result) break ;
          } /* endfor */
        }
        else
        {
           table_track -> all.page[file_table_index].file_has_no_secondaries = 1 ;
        } /* endif */
     } /* endif */
     return result ;
}

static int table_read_resolve_secondary (
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              unsigned int file_table_index,
              unsigned int cluster_number,
              table_track_t * table_track
) {
   int has_no_secondaries = table_track -> all.page[file_table_index].file_has_no_secondaries ;

   return (has_no_secondaries)
          ? table_read_fixup_shmat(table_response,rid,file_table_index,cluster_number,table_track)
          : table_read_may_have_secondaries(table_response,rid,file_table_index,cluster_number,table_track) ;
}

/*
 * Read a record from a file which has not been seen before
 */
static int table_read_new_file (
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              const char * file_name,
              _TABLE_STATIC_T * table_static,
              table_track_t * table_track ,
              unsigned int name_hash_value,
              unsigned int file_name_length_including_nul
) {
   unsigned int old_next_free_name_space ;
   unsigned int new_next_free_space ;
   char * copied_name ;
   int load_result ;
   unsigned int file_name_segment = ((unsigned int) file_name) >> 28 ;

   unsigned int new_hi_used_file ;


   old_next_free_name_space = table_track -> free_name_index ;
   new_next_free_space = old_next_free_name_space + file_name_length_including_nul ;
   new_hi_used_file = table_track -> free_file_index + 1 ;
   if (new_next_free_space > name_table_depth)
   {
      return PAC_TABLE_NOSTRINGSPACE ;
   } /* endif */

   if (new_hi_used_file > all_file_index_depth)
   {
      return PAC_TABLE_FILENOTFOUND ;
   } /* endif */

   /*
    * Initialise if needed
    */
   if (1 == new_hi_used_file)
   {
      /*
       * This 'load' is done so the statics will persist across TP loads and
       * unload, within a CICS region.
       */
      load_result = (int) load("/usr/lib/PAC_Data_Tables_Ext", 1, NULL) ;
      if (0 == load_result)
      {
         return PAC_TABLE_LOAD_FAILURE ;
      }
      /*
       * Set the attached file table to indicate no files attached
       */
      table_track->attached.page[0].line[0].segment_number_and_file_number=0x4fffffff ;
      table_track->attached.page[0].line[1].segment_number_and_file_number=0x5fffffff ;
      table_track->attached.page[0].line[2].segment_number_and_file_number=0x6fffffff ;
      table_track->attached.page[0].line[3].segment_number_and_file_number=0x7fffffff ;
      /*
       * Set the open file table to indicate no files open
       */
      clear_open_table(&(table_track->opened)) ;
   } /* endif */
   /*
    * Tack it into the name table and read from it
    */

   copied_name = table_track -> name.page + old_next_free_name_space ;
   table_track -> all.page [ new_hi_used_file ] . name = table_track -> name.page + old_next_free_name_space ;
   table_track -> all.page [ new_hi_used_file ] . next_hash = table_track -> name_hash.page[name_hash_value] ;
   table_track -> name_hash.page[name_hash_value] = new_hi_used_file ;

   table_static -> table_file_tag = new_hi_used_file ;

   /*
    * AIXism : If the passed-in address is in read-only storage, keep it
    * around for optimisation purposes (it will avoid a 'strcmp' next time
    * around
    */
   if (0x1 == file_name_segment || 0xd == file_name_segment )
   {
      table_static -> table_file_name = file_name ;
   }
   else
   {
      table_static -> table_file_name = NULL ; /* 19970219 tjcw */
   } /* endif */
   table_track -> free_name_index = new_next_free_space ;
   table_track -> free_file_index = new_hi_used_file ;
   memcpy(copied_name, file_name, file_name_length_including_nul ) ;
   return table_read_resolve_secondary(
            table_response,
            rid ,
            new_hi_used_file,
            0xffffffff,
            table_track
          ) ;
}
/*
 * Read a record, but the optimisation information supplied by the app was
 * not right. This may be because the file is not open yet
 *
 * Scan the name hash table to locate the file; then call table_read_new_file if the
 * file was not found, or table_read_fixup_shmat if the file was found.
 * 19980210 tjcw fixed up for new organisation, but check table_read_new_file/table_read_existing_file
 */

static int table_read_fixup_file (
              _TABLE_RESPONSE_T * table_response,
              const void * rid,
              const char * file_name ,
              _TABLE_STATIC_T * table_static,
              table_track_t * table_track
) {
   unsigned int name_hash_value ;
   unsigned int candidate_file_index ;
   unsigned int x ;
   unsigned int correct_tag ;
   void * expected_address ;
   unsigned int name_length = 0 ;
   /*
    * Hash from the file name to see if we can find it
    */


   unsigned int h = 0 ;

   unsigned int c = file_name[0] ;
   while (0 != c)
   {
      name_length += 1 ;
      h = ( h << 1 ) + c ;
      c = file_name[name_length] ;
   } /* endwhile */
   name_hash_value = h % name_hash_depth ;

   candidate_file_index = table_track -> name_hash.page [ name_hash_value ] ;

   for (x = 0 ; x < all_file_index_depth; x += 1 )
   {
      if (0 == candidate_file_index)
      {
         /*
          * We have followed the hash chain and not found the file at all.
          * So, the file has never been seen before
          */
         DIAG(diag_file,(" This is a new file\n")) ;
         return table_read_new_file(
              table_response,
              rid,
              file_name ,
              table_static,
              table_track ,
              name_hash_value,
              name_length+1
                ) ;
      }
      else if ( 0 == my_memcmp(table_track -> all.page[candidate_file_index].name,
                               file_name,
                               name_length+1
                               )
              )
      {
         /*
          * We were following the hash chain and now we find the file.
          * Remember where we found it, to speed the search next time.
          *
          * File has been seen before, the optimisation info was wrong
          * Fix it up for next time, and use the underlying call
          */
         unsigned int file_name_segment = ((unsigned int) file_name) >> 28 ;
         /* 19970219 tjcw */
         /*
          * AIXism : If the passed-in address is in read-only storage, keep it
          * around for optimisation purposes (it will avoid a 'strcmp' next time
          * around
          */
         if (0x1 == file_name_segment || 0xd == file_name_segment )
         {
            table_static -> table_file_tag = candidate_file_index ;
            table_static -> table_file_name = file_name ;
         }
         else
         {
            table_static -> table_file_tag = 0 ;
            table_static -> table_file_name = NULL ;
         } /* endif */
         DIAG(diag_file,(" This file is at index %i\n",candidate_file_index)) ;
         return table_read_resolve_secondary(
                  table_response,
                  rid ,
                  candidate_file_index,
                  0xffffffff,
                  table_track
                ) ;
      }
      /*
       * No match, scan down the hash chain
       */
      candidate_file_index = table_track -> all.page[candidate_file_index].next_hash ;
   } /* endfor */
   /*
    * If we get this far, the hash table is in a loop. Apologise to the application
    * and return.
    */
   return PAC_TABLE_ILLOGIC ;

}


/*
 * New-style call from application ...
 */
int table_read (
         _TABLE_RESPONSE_T * table_response,
   const void *  rid ,
   const char *  file_name,
         _TABLE_STATIC_T * table_static ,
         int     match_fails
) {
   int result ;
   unsigned int version ;
   unsigned int version_secondary ;
   unsigned int next_in_primary ;
   DIAG(diag_api,("\nTable name=\"%s\", match_fails=%i, table_static=(%i,0x%08x,\"%s\")\n",
                                 file_name,
                                                 match_fails,
                                                                   table_static -> table_file_tag,
                                                                      table_static -> table_file_name,
                                                                           table_static -> table_file_name)) ;
   result = ( ! match_fails )
           ? table_read_resolve_secondary(table_response, rid, table_static -> table_file_tag,0xffffffff, &table_track_ext)
           : table_read_fixup_file(table_response, rid, file_name, table_static, &table_track_ext ) ;
#if 0
   version = table_response -> version ;
   if (PAC_TABLE_NOERROR == result && table_correct_version_primary == version )
   {
      /*
       * OK so far, but we had a table with secondaries ...
       */
      for (; ; )
      {
         unsigned int cluster_number=*(unsigned int *) (table_response -> field_addr) ;
         DIAG(diag_cluster,(" Record is in secondary cluster %i\n",cluster_number)) ;
         next_in_primary = table_response -> next_record_index ;
         result = table_read_fixup_shmat(table_response, rid, table_static -> table_file_tag,cluster_number, &table_track_ext) ;
         version_secondary = table_response -> version ;
         if (PAC_TABLE_NOERROR == result && table_correct_version != version_secondary )
         {
            result = PAC_TABLE_TERTIARY ;
            break ;
         } /* endif */
         if (PAC_TABLE_NOTFND != result) break ;
         /*
          * We ran off the end of a secondary cluster. Look in the primary for the next secondary cluster
          * and go round again.
          */
         if (0xffffffff == next_in_primary) break ; /* Really was off the end of the file */
         DIAG(diag_cluster,(" Not in that secondary cluster, trying the next one\n")) ;
         result = table_read_rrn(table_response, next_in_primary, table_static -> table_file_tag, 0xffffffff, &table_track_ext) ;
         if (PAC_TABLE_NOERROR != result) break ;
      } /* endfor */
   } /* endif */
#endif
   DIAG(diag_api,("  Result is %i"
                  " field_addr=0x%08x"
                  " field_length=%i"
                  " actual_rid=0x%08x"
                  " exact_rid=%i\n",
                  result, table_response->field_addr,table_response->field_length,table_response->actual_rid, table_response->exact_rid)) ;

   return result ;
}
