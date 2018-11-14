#include "dr_api.h"
#include <vector>

static void event_exit(void);
static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb,
				bool for_trace, bool translating);
static void clean_call(uint memref_count);
static uint64 memref_total;
static std::vector<uint64> memAddrs;

enum REF_TYPE {
        READ = 0,
        WRITE = 1
};

/*typedef struct mem_ref_t {
  	REF_TYPE ref_type;
  	ushort size;
  	ushort addr;
} mem_ref_t;
 */

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
	/* register events */
	dr_register_exit_event(event_exit);
	dr_register_bb_event(event_basic_block);
	//memref_total = 0;
	 memAddrs = new std::vector<uint64>;
}

static void event_exit(void) {
	//dr_printf("%i", memref_total);
	for(uint64 addr : memAddrs) {
	    dr(printf("i", memAddrs));
	}
}

static void createMemRef(uint64 memAddr) {
    memAddrs.insert(memAddr);
}


static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb,
                                         bool for_trace, bool translating) {
    //uint num_memory = 0;
    instr_t *instr;
    //std::vector<mem_ref_t> memRefs = new std::vector<mem_ref_t>;

    /* find all memory references in block */
    for(instr = instrlist_first(bb); instr != NULL; instr = instr_get_next(instr)) {
        if(instr_reads_memory(instr) || instr_writes_memory(instr)) {
            for(i=0; i < instr_num_srcs(instr), i++) {
                if(opnd_is_memory_reference((instr_get_src(instr, i)))) {
                    /* insert clean call */
                    dr_insert_clean_call(drcontext, bb, instrlist_first(bb), createMemRef, false, 1,
                                         OPND_CREATE_INT64(instr_get_src(instr, i)));
                }
            }for(i=0; i < instr_num_dsts(instr), i++) {
                if (opnd_is_memory_reference((instr_get_dest(instr, i)))) {
                    /* insert clean call */
                    dr_insert_clean_call(drcontext, bb, instrlist_first(bb), createMemRef, false, 1,
                                         OPND_CREATE_INT64(instr_get_dest(instr, i)));
                }
            }
        }
    }



    return DR_EMIT_DEFAULT;
}
