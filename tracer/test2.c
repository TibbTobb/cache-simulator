#include "dr_api.h"
#include "drutil.h"

static void event_exit(void);
static void clean_call(uint memref_count);
static uint64 memref_total;

static void event_exit(void) {
    dr_printf("%i", memref_total);
}

static void clean_call(uint memref_count) {
    memref_total++;
}

static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb,
                                         bool for_trace, bool translating) {

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
    /* register events */
    dr_printf("hello");
    dr_register_exit_event(event_exit);
    drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL);
    drmgr_register_thread_init_event(event_thread_init);
    drmgr_register_thread_exit_event(event_trhead_exit);
    drmgr_register_app2app_event(event_bb_app2app, NULL);   
    memref_total = 0;
}

