#pragma once
#include "GraphRenderJournal.hpp"
#if defined(STARFIELD_MFR_QUALIFICATION)
#include <atomic>
#if !defined(STARFIELD_GRAPH_RENDER_JOURNAL_FILE)
#error MFR qualification requires its confined development journal.
#endif
#endif

namespace starfield::adapter {
// Instrument only the isolated qualification build. No mutex or host callback;
// one atomic counter per AEX and complete append records prove selector overlap.
class MfrQualificationScope {
#if defined(STARFIELD_MFR_QUALIFICATION)
    inline static std::atomic<unsigned> active_{};
    inline static std::atomic<unsigned long long> serial_{};
    const PF_InData* data_{};const char* module_{};PF_Cmd command_{};
    unsigned long long id_{};bool observed_{};
    void record(const char* phase,unsigned active)const noexcept {
        char line[768]{};LARGE_INTEGER now{};QueryPerformanceCounter(&now);
        const auto length=std::snprintf(line,sizeof(line),"{\"event\":\"mfr-selector\",\"phase\":\"%s\",\"module\":\"%s\",\"nativeBuild\":%d,\"id\":%llu,\"pid\":%lu,\"thread\":%lu,\"qpc\":%lld,\"active\":%u,\"selector\":%d,\"time\":%ld,\"scale\":%lu}\n",
            phase,module_,STARFIELD_VERSION_BUILD,id_,static_cast<unsigned long>(GetCurrentProcessId()),static_cast<unsigned long>(GetCurrentThreadId()),
            now.QuadPart,active,int(command_),static_cast<long>(data_?data_->current_time:0),static_cast<unsigned long>(data_?data_->time_scale:0));
        if(length>0 && static_cast<std::size_t>(length)<sizeof(line))graph_journal_detail::append(line,static_cast<DWORD>(length));
    }
#endif
public:
    MfrQualificationScope(const PF_InData* data,PF_Cmd command,const char* module)noexcept {
#if defined(STARFIELD_MFR_QUALIFICATION)
        observed_=command==PF_Cmd_SMART_PRE_RENDER || command==PF_Cmd_SMART_RENDER || command==PF_Cmd_SMART_RENDER_GPU || command==PF_Cmd_RENDER;
        if(observed_){data_=data;module_=module;command_=command;id_=serial_.fetch_add(1)+1;const auto count=active_.fetch_add(1)+1;record("begin",count);}
#else
        (void)data;(void)command;(void)module;
#endif
    }
    ~MfrQualificationScope(){
#if defined(STARFIELD_MFR_QUALIFICATION)
        if(observed_){record("end",active_.load());active_.fetch_sub(1);}
#endif
    }
    MfrQualificationScope(const MfrQualificationScope&)=delete;
    MfrQualificationScope& operator=(const MfrQualificationScope&)=delete;
};
}
