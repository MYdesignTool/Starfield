// Exercise the actual qualification journal scope, not a synthetic log writer.
// This is a standalone executable. No effect/AEX is loaded and no AE is started.
#include "AEConfig.h"
#include "AE_Effect.h"
#define STARFIELD_TEST_NO_JOURNAL
#define STARFIELD_MFR_QUALIFICATION
#define STARFIELD_GRAPH_RENDER_JOURNAL_FILE L"artifacts/mfr-offline-handoff-20261116/journal-tests/records.jsonl"
#include "MfrQualification.hpp"
#include <barrier>
#include <thread>
#include <vector>

int main(){
    using starfield::adapter::MfrQualificationScope;
    std::barrier rendezvous(4);
    std::vector<std::thread> workers;
    for(unsigned worker=0;worker<4;++worker)workers.emplace_back([&,worker]{
        for(unsigned frame=0;frame<12;++frame){
            PF_InData data{};data.current_time=A_long(worker*100+frame);data.time_scale=24;
            MfrQualificationScope outer(&data,PF_Cmd_SMART_RENDER,"main");
            rendezvous.arrive_and_wait();
            try {
                auto nested=data;++nested.current_time;
                MfrQualificationScope inner(&nested,PF_Cmd_SMART_RENDER_GPU,"main");
                throw 1; // The destructor must pair the begin record on unwind.
            } catch(int) {}
            rendezvous.arrive_and_wait();
        }
    });
    for(auto& worker:workers)worker.join();
    PF_InData data{};data.current_time=500;data.time_scale=24;
    {MfrQualificationScope unobserved(&data,PF_Cmd_GLOBAL_SETUP,"main");}
    {MfrQualificationScope pre(&data,PF_Cmd_SMART_PRE_RENDER,"main");}
    std::printf("Offline production journal: pid=%lu, 48 overlapping render scopes, 48 exception-unwound nested GPU scopes, one pre-render, no GLOBAL_SETUP record expected.\n",static_cast<unsigned long>(GetCurrentProcessId()));
}
