#include "dr_api.h"
#include "drutil.h"
#include "drmgr.h"

static void event_exit(void);
static void clean_call(uint memref_count);
static uint64 memref_total;

static void event_exit(void) {
    dr_printf("%i", memref_total);
}

static void clean_call(uint memref_count) {
    memref_total++;
}

    static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace,
                                                 bool translating, void *user_data) {
    /* insert clean call */
    //dr_insert_clean_call(drcontext, bb, instrlist_first(bb), (void*)clean_call, false, 0);

    return DR_EMIT_DEFAULT;
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
    /* register events */
    dr_printf("hello");
    dr_register_exit_event(event_exit);
    drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL);
    memref_total = 0;
}


