/* 
 * This file is part of MICA, a Pin tool to collect
 * microarchitecture-independent program characteristics using the Pin
 * instrumentation framework. 
 *
 * Please see the README.txt file distributed with the MICA release for more
 * information.
 */

#include "mica.h"

void init_linecount();
VOID instrument_linecount(INS ins, VOID* v);
VOID fini_linecount(INT32 code, VOID* v);

VOID linecount_mem_op(ADDRINT effMemAddr, ADDRINT size);

VOID linecount_instr_interval_output();
VOID linecount_instr_interval_reset();
