/*
 * IBM Internal Use Only           T J C Ward       28 September 1993
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */
/*
 * Return codes from 'tables' functions
 */


typedef enum {
  /* Codes which the application may understand */
  PAC_TABLE_NOERROR = 0 ,
  PAC_TABLE_NOTFND = 1 /* Record not found in table */ ,
  PAC_TABLE_DISABLED = 2 /* Table file was not found */ ,
  PAC_TABLE_FILENOTFOUND = 3 /* No room to track any more files */ ,
  /* Codes which reflect internal errors which the application cannot do
   * anything useful about
   */
  PAC_TABLE_SHMAT_FAILED = 4 /* POSIX 'shmat' call failed */ ,
  PAC_TABLE_BAD_HASH = 5 /* Hash function returned out-of-range value */ ,
  PAC_TABLE_ILLOGIC = 6 /* Internal code error, should not happen */ ,
  PAC_TABLE_NYI = 7 /* Not yet implemented */ ,
  PAC_TABLE_NOSTRINGSPACE = 8 /* No space for more file name strings */ ,
  PAC_TABLE_LOAD_FAILURE = 9 /* 'load' failed (TP usage will fail) */ ,
  PAC_TABLE_SHMDT_FAILED = 10 /* Shared memory detach failed */ ,
  PAC_TABLE_NOT_TABLE = 11 /* File was found but was not a data table */ ,
  PAC_TABLE_WRONG_VERSION = 12 /* File was the wrong version of data tables */ ,
  PAC_TABLE_NAME_TOO_LONG = 13 /* Name was too long for supplied buffering */ ,
  PAC_TABLE_NOT_CLUSTERED = 14 /* Table was not clustered when it should have been */ ,
  PAC_TABLE_CLOSE_FAILED = 15 /* Attempt to close a file (before attaching another one) failed */ ,
  PAC_TABLE_TERTIARY = 16 /* Table seems to have a secondary which itself has a secondary */ ,
  /*
   * Error which can occur during table creation
   */
  PAC_TABLE_CLUSTER_OVERFLOW /* Inserting this record would overflow the 256MB cluster limit */
} table_error_t ;

