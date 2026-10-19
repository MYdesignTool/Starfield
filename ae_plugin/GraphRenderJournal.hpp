#pragma once
#include "PluginVersion.h"
#if !defined(STARFIELD_TEST_NO_JOURNAL) && __has_include("../artifacts/graph-render-journal-config.hpp")
#include "../artifacts/graph-render-journal-config.hpp"
#endif
#if defined(STARFIELD_GRAPH_RENDER_JOURNAL_FILE) && defined(_WIN32)
#include <windows.h>
#include <cstdio>
#include <mutex>
#include <cmath>
#endif
namespace starfield::adapter {
// Only explicit, confined development builds enable this journal. It observes
// the result, never drives host state, retains SDK handles or changes the result.
inline void journal_graph_render(const PF_InData* data,double guard,PF_Err error,const char* message) noexcept {
#if defined(STARFIELD_GRAPH_RENDER_JOURNAL_FILE) && defined(_WIN32)
    try {
        static std::mutex mutex;std::lock_guard lock(mutex);
        auto* file=_wfopen(STARFIELD_GRAPH_RENDER_JOURNAL_FILE,L"ab");if(!file)return;
        struct Close {FILE* value;~Close(){std::fclose(value);}} close{file};
        std::fprintf(file,"{\"nativeBuild\":%d,\"pid\":%lu,\"effect\":\"%p\",\"time\":%ld,\"scale\":%lu,\"guard\":",
            STARFIELD_VERSION_BUILD,static_cast<unsigned long>(GetCurrentProcessId()),data?data->effect_ref:nullptr,
            static_cast<long>(data?data->current_time:0),static_cast<unsigned long>(data?data->time_scale:0));
        if(std::isfinite(guard))std::fprintf(file,"%.17g",guard);else std::fputs("null",file);
        std::fprintf(file,",\"result\":%ld,\"canceled\":%s,\"message\":\"",static_cast<long>(error),error==PF_Interrupt_CANCEL?"true":"false");
        if(message)for(unsigned i=0;message[i] && i<255;++i) {
            const auto c=static_cast<unsigned char>(message[i]);
            if(c=='"' || c=='\\'){std::fputc('\\',file);std::fputc(c,file);}
            else if(c<32)std::fprintf(file,"\\u%04x",static_cast<unsigned>(c));else std::fputc(c,file);
        }
        std::fputs("\"}\n",file);
    } catch(...) {} // Diagnostic I/O must not fail a render or hide its result.
#else
    (void)data;(void)guard;(void)error;(void)message;
#endif
}
}
