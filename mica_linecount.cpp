/*
 * This file is part of MICA, a Pin tool to collect
 * microarchitecture-independent program characteristics using the Pin
 * instrumentation framework.
 *
 * Please see the README.txt file distributed with the MICA release for more
 * information.
 */

#include "pin.H"

/* MICA includes */
#include "mica_utils.h"
#include "mica_linecount.h"

/* Global variables */
extern INT64 interval_size;
extern INT64 interval_ins_count;
extern INT64 interval_ins_count_for_hpc_alignment;
extern INT64 total_ins_count;
extern INT64 total_ins_count_for_hpc_alignment;

extern UINT32 _block_size;

/* Local variables */
#define MAX_TABLE_ACCESSES 1000

static ofstream output_file_linecount;
static int accesses_for_line[MAX_TABLE_ACCESSES];
static int working_set_size = 0;
static nlist* DmemCacheWorkingSetTable[MAX_MEM_TABLE_ENTRIES];
static long long mem_ref = 0;

/* initializing */
void init_linecount(){
	mem_ref = 0;
	working_set_size = 0;
	for (int i = 0; i < MAX_MEM_TABLE_ENTRIES; i++) {
		DmemCacheWorkingSetTable[i] = (nlist*) NULL;
	}

	if (interval_size == -1) {
		output_file_linecount.open(mkfilename("linecount_full_int"), ios::out|ios::trunc);
	} else {
		output_file_linecount.open(mkfilename("linecount_phases_int"), ios::out|ios::trunc);
	}
}

static void linecount_output() {
	int begin = working_set_size - working_set_size % MAX_TABLE_ACCESSES;
	for (int i = 0; i < MAX_TABLE_ACCESSES && i + begin <= working_set_size; i++) {
		output_file_linecount << (begin + i) << " " << accesses_for_line[i] << endl;
	}
}

VOID linecount_mem_op(ADDRINT effMemAddr, ADDRINT size){
	mem_ref++;
	if(size > 0){
		ADDRINT a;
		ADDRINT addr, endAddr, upperAddr, indexInChunk;
		memNode* chunk;

		/* D-stream (64-byte) cache block memory footprint */

		addr = effMemAddr >> _block_size;
		endAddr = (effMemAddr + size - 1) >> _block_size;

		for(a = addr; a <= endAddr; a++){

			upperAddr = a >> LOG_MAX_MEM_BLOCK;
			indexInChunk = a ^ (upperAddr << LOG_MAX_MEM_BLOCK);

			chunk = lookup(DmemCacheWorkingSetTable, upperAddr);
			if(chunk == (memNode*)NULL) {
				chunk = install(DmemCacheWorkingSetTable, upperAddr);
			}

			if (!chunk->numReferenced[indexInChunk]) {
				working_set_size++;
				accesses_for_line[working_set_size % MAX_TABLE_ACCESSES] = mem_ref;
				if ((working_set_size + 1) % MAX_TABLE_ACCESSES == 0) {
					linecount_output();
				}
			}

			//assert(indexInChunk >= 0 && indexInChunk < MAX_MEM_BLOCK);
			chunk->numReferenced[indexInChunk] = true;
		}
	}
}

static ADDRINT linecount_instr_intervals() {
	return (ADDRINT)(interval_ins_count_for_hpc_alignment == interval_size);
}

VOID linecount_instr_interval_output(){
	linecount_output();
	output_file_linecount << endl;
}

VOID linecount_instr_interval_reset(){
	/* clean used memory, to avoid memory shortage for long (CPU2006) benchmarks */
	working_set_size = 0;
	mem_ref = 0;
	for(ADDRINT i=0; i < MAX_MEM_TABLE_ENTRIES; i++){
		free_nlist(DmemCacheWorkingSetTable[i]);
	}
	accesses_for_line[0] = 0;
}

static VOID linecount_instr_interval(){
	linecount_instr_interval_output();
	linecount_instr_interval_reset();
	interval_ins_count = 0;
	interval_ins_count_for_hpc_alignment = 0;
}

/* instrumenting (instruction level) */
VOID instrument_linecount(INS ins, VOID* v) {
	if(INS_IsMemoryRead(ins)){
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)linecount_mem_op, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_END);

		if(INS_HasMemoryRead2(ins)){
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)linecount_mem_op, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_END);
		}
	}

	if (interval_size != -1) {
		INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)linecount_instr_intervals, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)linecount_instr_interval, IARG_END);
	}
}

/* finishing... */
VOID fini_linecount(INT32 code, VOID* v){
	linecount_output();
	output_file_linecount.close();
}
