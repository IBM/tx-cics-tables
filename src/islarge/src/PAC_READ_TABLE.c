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

