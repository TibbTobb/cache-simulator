#include "dr_api.h"
#include "drutil.h"
#include "drreg.h"
#include "drmgr.h"
#include <vector>
#include <string>
#include <dr_ir_macros_aarch64.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../common/memref.h"

typedef struct {
    thread_id_t thread_id;
} per_thread_t;

static int tls_idx;
static bool online;
static std::string pipe_name;
static int fd;
static FILE *file;

static void create_mem_ref(uint write, unsigned short size) {


    void *drcontext = dr_get_current_drcontext();

    per_thread_t *data;
    data = static_cast<per_thread_t *>(drmgr_get_tls_field(drcontext, tls_idx));
    DR_ASSERT(data != nullptr);


    reg_t addr = dr_read_saved_reg(drcontext, SPILL_SLOT_1);
    thread_id_t thread_id = data->thread_id;

    if(online) {
        mem_ref_t mem_ref = {write ? WRITE: READ, size, addr, thread_id};

        ::write(fd, &mem_ref, sizeof(mem_ref));

    } else {
        //write mem_ref to per thread file

        fprintf(file, "%lu %hu %i %i\n", addr, size, write, thread_id);

    }
}

static void instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write) {
    //reserve two scratch registers
    reg_id_t reg_for_addr, reg_scratch;
    if(drreg_reserve_register(drcontext, ilist, where, nullptr, &reg_for_addr) != DRREG_SUCCESS
       || drreg_reserve_register(drcontext, ilist, where, nullptr, &reg_scratch) != DRREG_SUCCESS ) {
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
    dr_insert_clean_call(drcontext, ilist, where, (void *) create_mem_ref, false, 2, OPND_CREATE_INT(type),
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

static void event_exit() {

    dr_printf("finished tracing\n");

    if(!drmgr_unregister_bb_app2app_event(event_bb_app2app) ||
       !drmgr_unregister_bb_insertion_event(event_app_instruction)) {
        DR_ASSERT(false);
    }
    drreg_exit();
    drutil_exit();
    drmgr_exit();

    if(online) {
        close(fd);
    } else {
        fclose(file);
    }

}

static void event_thread_init(void *drcontext) {
    thread_id_t thread_id = dr_get_thread_id(drcontext);

    if(online) {
        mem_ref_t mem_ref = {THREAD_INIT, 0, 0, thread_id};

        ::write(fd, &mem_ref, sizeof(mem_ref));

    } else {
        //write mem_ref to per thread file

        fprintf(file, "%lu %hu %i %i\n", 0L, 0, THREAD_INIT, thread_id);

    }


    //open file for thread to write to
    per_thread_t *data = static_cast<per_thread_t *>(dr_thread_alloc(drcontext, sizeof(per_thread_t)));
    DR_ASSERT(data != nullptr);
    drmgr_set_tls_field(drcontext, tls_idx, data);
    thread_id_t threadid = dr_get_thread_id(drcontext);
    data->thread_id = thread_id;
}

static void event_thread_exit(void *drcontext) {

    per_thread_t *data;
    data = static_cast<per_thread_t *>(drmgr_get_tls_field(drcontext, tls_idx));
    thread_id_t thread_id = data->thread_id;

    if(online) {
        mem_ref_t mem_ref = {THREAD_EXIT, 0, 0, thread_id};

        ::write(fd, &mem_ref, sizeof(mem_ref));

    } else {
        //write mem_ref to per thread file

        fprintf(file, "%lu %hu %i %i\n", 0, 0, THREAD_EXIT, thread_id);

    }
    //free per thread data
    dr_thread_free(drcontext, data, sizeof(per_thread_t));
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
    if(argc > 1) {
        online = std::string(argv[1]) == "online";
    } else {
        online = false;
    }
    if(online) {
        pipe_name = "cachesimpipe";

        if (mkfifo(pipe_name.c_str(), 0666) != 0)
            DR_ASSERT(false);
        dr_printf("pipe opened\n");
        fd = ::open(pipe_name.c_str(), O_WRONLY);
    } else {
        file = fdopen(dr_open_file("memref-output.dat", DR_FILE_WRITE_OVERWRITE), "w");
    }

    dr_printf("Starting tracer\n");
    //get 3 reg slots
    drreg_options_t ops = {sizeof(ops), 3, false};
	if(!drutil_init()) {
        DR_ASSERT(false);
    }
	if(drreg_init(&ops) != DRREG_SUCCESS) {
	    DR_ASSERT(false);
	}

    /* register events */
	dr_register_exit_event(event_exit);
	if(!drmgr_register_thread_init_event(event_thread_init) ||
	!drmgr_register_thread_exit_event(event_thread_exit) ||
	!drmgr_register_bb_app2app_event(event_bb_app2app, nullptr) ||
	!drmgr_register_bb_instrumentation_event(nullptr, event_app_instruction, nullptr)) {
	    DR_ASSERT(false);
	}

	//register tls slot to hold private thread data
	tls_idx = drmgr_register_tls_field();
	DR_ASSERT(tls_idx != -1);
}