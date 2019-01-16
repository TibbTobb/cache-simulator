#include "dr_api.h"

static void event_exit(void);
static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb,
				bool for_trace, bool translating);
static void clean_call(uint memref_count);
static uint64 memref_total;

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
	/* register events */
	dr_register_exit_event(event_exit);
	dr_register_bb_event(event_basic_block);
	memref_total = 0;
}

static void event_exit(void) {
	dr_printf("%i", memref_total);
	}

static void clean_call(uint memref_count) {
	memref_total += memref_count;
}
	
	
static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb,
				bool for_trace, bool translating) {
	uint num_memory = 0;
	instr_t *instr;
	/* count number of memory instructions in block*/
	for(instr = instrlist_first(bb); instr != NULL; instr = instr_get_next(instr)) {
		if(instr_reads_memory(instr) || instr_writes_memory(instr)) {
		num_memory++;		
		}
	}
	
	/* insert clean call */
	dr_insert_clean_call(drcontext, bb, instrlist_first(bb), clean_call, false, 1, OPND_CREATE_INT32(num_memory++));
	
	return DR_EMIT_DEFAULT;
}
