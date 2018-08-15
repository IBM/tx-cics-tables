/*
 * IBM Internal Use Only           T J C Ward                 10 May 1994
 *
 * This is prototype code. It is supplies 'as is', and no responsibility
 * is accepted for completeness or correctness.
 *
 */

/*
 * COBOL wrapper for CICS/6000 Support PAC data tables access
 *
 * COBOL should use this as ...
 *
 * CALL PAC_READ_TABLE
 * USING <file-name>,
 *       <rid-field>,
 *       <record-pointer>,
 *       <response-variable>,
 *       <field-length-variable>,
 *       <actual-rid-pointer>,
 *       <exact-rid-variable>
 *
 * This provides a thin wrapper around the CICS/6000 C language data tables
 * access method
 */
#include <table_ext.h>

void PAC_READ_TABLE (
  const char * file_name,
  const void * rid,
  void ** record,
  int * resp,
  int * flength,
  const void ** actual_rid,
  int * exact
)
{
  EXEC_PAC_READ_TABLE
    FILE(file_name)
    RIDFLD(rid)
    SET(*record)
    FLENGTH(*flength)
    ACTUAL_RID(*actual_rid)
    EXACT_RID(*exact)
  END_EXEC_PAC_READ_TABLE ;
}

