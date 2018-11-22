#include "dr_api.h"
#include "drutil.h"
#include "drreg.h"
#include "drmgr.h"
#include <vector>
#include <dr_ir_macros_aarch64.h>

enum REF_TYPE {
    READ = 0,
    WRITE = 1
};

typedef struct mem_ref_t {
    REF_TYPE ref_type;
    ushort size;
    uint64 addr;
} mem_ref_t;

static std::vector<mem_ref_t> *memRefs;


static void createMemRef(uint read, uint64 size) {
    void *drcontext = dr_get_current_drcontext();

    mem_ref_t *m = new mem_ref_t;
    //get address from spill slot
    m->addr = dr_read_saved_reg(drcontext, SPILL_SLOT_1);
    m->size = size;
    m->ref_type = read ? READ : WRITE;
    memRefs->push_back(*m);
}

static void instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write) {
    //reserve two scratch registers
    reg_id_t reg_for_addr, reg_scratch;
    if(drreg_reserve_register(drcontext, ilist, where, NULL, &reg_for_addr) != DRREG_SUCCESS
       || drreg_reserve_register(drcontext, ilist, where, NULL, &reg_scratch) != DRREG_SUCCESS ) {
        DR_ASSERT(false);
        return;
    }

    //get size
    uint size = drutil_opnd_mem_size_in_bytes(ref,where);
    //get type
    uint type = write ? 1 : 0;


    //insert instruction to get address
    drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg_for_addr, reg_scratch);

    //insert instruction to save register in spill slot
    dr_save_reg(drcontext, ilist, where, reg_for_addr, SPILL_SLOT_1);

    //insert clean call
    dr_insert_clean_call(drcontext, ilist, where, (void*) createMemRef, false, 2, OPND_CREATE_INT(type),
                         OPND_CREATE_INT64(size));

    //restore scrach registers
    if(drreg_unreserve_register(drcontext, ilist, where, reg_for_addr) != DRREG_SUCCESS
       || drreg_unreserve_register(drcontext, ilist, where, reg_scratch) != DRREG_SUCCESS ) {
        DR_ASSERT(false);
    }
}

static dr_emit_flags_t event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating) {
    //expand repeat string instructions
    if(!drutil_expand_rep_string(drcontext, bb)) {
        DR_ASSERT(false);
    }
    return DR_EMIT_DEFAULT;
}

static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace,
        bool translating, void *user_data) {
        if(instr_reads_memory(instr) || instr_writes_memory(instr)) {
            //for each source mem ref
            for(int i=0; i < instr_num_srcs(instr); i++) {
                if(opnd_is_memory_reference((instr_get_src(instr, i)))) {
                    instrument_mem(drcontext, bb, instr, instr_get_src(instr, i), false);
                }
            }
            //for each destination mem ref
            for(int i=0; i < instr_num_dsts(instr); i++) {
                if (opnd_is_memory_reference((instr_get_dst(instr, i)))) {
                    instrument_mem(drcontext, bb, instr, instr_get_dst(instr, i), true);
                }
            }
        }
    return DR_EMIT_DEFAULT;
}

static void event_exit(void) {
    //dr_printf("%i", memref_total);

    int i = 0;
    for(mem_ref_t m : *memRefs) {
        if(i>100)
            break;
        i++;
        dr_printf("Address: 0x%x, Size: %i, Type: %i ", m.addr, m.size, m.ref_type);
    }
    delete(memRefs);
    if(!drmgr_unregister_bb_app2app_event(event_bb_app2app) ||
        !drmgr_unregister_bb_insertion_event(event_app_instruction)) {
        DR_ASSERT(false);
    }
    drreg_exit();
    drutil_exit();
    drmgr_exit();
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
    dr_printf("Starting");
    drreg_options_t ops = {sizeof(ops), 3, false};
	if(!drutil_init()) {
        DR_ASSERT(false);
    }
	if(drreg_init(&ops) != DRREG_SUCCESS) {
	    DR_ASSERT(false);
	}

    /* register events */
	dr_register_exit_event(event_exit);
	if(!drmgr_register_bb_app2app_event(event_bb_app2app, NULL) ||
	!drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL)) {
	    DR_ASSERT(false);
	}

	//memref_total = 0;
	 memRefs = new std::vector<mem_ref_t>;
}