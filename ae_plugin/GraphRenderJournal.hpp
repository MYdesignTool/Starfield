#pragma once
#include "PluginVersion.h"
#if !defined(STARFIELD_TEST_NO_JOURNAL) && __has_include("../artifacts/graph-render-journal-config.hpp")
#include "../artifacts/graph-render-journal-config.hpp"
#endif
#if defined(STARFIELD_GRAPH_RENDER_JOURNAL_FILE) && defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cmath>
#include "UiExclusionClient.hpp"
#endif
namespace starfield::adapter {
#if defined(STARFIELD_GRAPH_RENDER_JOURNAL_FILE) && defined(_WIN32)
namespace graph_journal_detail {
inline void append(const char* line,DWORD bytes) noexcept {
    // Host and renderer use separate DLL/CRT instances. A CRT append stream's
    // seek-then-write can overwrite their concurrent records. Ask Windows for
    // append-only access and submit each complete JSON line in one write.
    const auto file=CreateFileW(STARFIELD_GRAPH_RENDER_JOURNAL_FILE,FILE_APPEND_DATA,
        FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return;
    DWORD written{};(void)WriteFile(file,line,bytes,&written,nullptr);CloseHandle(file);
}
}
#endif
// Only explicit, confined development builds enable this journal. It observes
// the result, never drives host state, retains SDK handles or changes the result.
inline void journal_graph_render(const PF_InData* data,double guard,PF_Err error,const char* message) noexcept {
#if defined(STARFIELD_GRAPH_RENDER_JOURNAL_FILE) && defined(_WIN32)
    try {
        char line[2048]{};
        const auto header=std::snprintf(line,sizeof(line),"{\"event\":\"render\",\"nativeBuild\":%d,\"pid\":%lu,\"thread\":%lu,\"tickMs\":%llu,\"uiTransaction\":%s,\"effect\":\"%p\",\"time\":%ld,\"scale\":%lu,\"guard\":",
            STARFIELD_VERSION_BUILD,static_cast<unsigned long>(GetCurrentProcessId()),static_cast<unsigned long>(GetCurrentThreadId()),
            static_cast<unsigned long long>(GetTickCount64()),ui_transaction_active()?"true":"false",data?data->effect_ref:nullptr,
            static_cast<long>(data?data->current_time:0),static_cast<unsigned long>(data?data->time_scale:0));
        if(header<0 || static_cast<std::size_t>(header)>=sizeof(line))return;
        auto used=static_cast<std::size_t>(header);
        const auto number=std::isfinite(guard)?std::snprintf(line+used,sizeof(line)-used,"%.17g",guard):
            std::snprintf(line+used,sizeof(line)-used,"null");
        if(number<0 || static_cast<std::size_t>(number)>=sizeof(line)-used)return;
        used+=static_cast<std::size_t>(number);
        const auto result=std::snprintf(line+used,sizeof(line)-used,",\"result\":%ld,\"canceled\":%s,\"message\":\"",
            static_cast<long>(error),error==PF_Interrupt_CANCEL?"true":"false");
        if(result<0 || static_cast<std::size_t>(result)>=sizeof(line)-used)return;
        used+=static_cast<std::size_t>(result);
        if(message)for(unsigned i=0;message[i] && i<255;++i) {
            if(used+9>=sizeof(line))break; // Six escaped bytes plus closing JSON.
            const auto c=static_cast<unsigned char>(message[i]);
            if(c=='"' || c=='\\'){line[used++]='\\';line[used++]=static_cast<char>(c);}
            else if(c<32){std::snprintf(line+used,sizeof(line)-used,"\\u%04x",static_cast<unsigned>(c));used+=6;}
            else line[used++]=static_cast<char>(c);
        }
        line[used++]='"';line[used++]='}';line[used++]='\n';
        graph_journal_detail::append(line,static_cast<DWORD>(used));
    } catch(...) {} // Diagnostic I/O must not fail a render or hide its result.
#else
    (void)data;(void)guard;(void)error;(void)message;
#endif
}
// phase is an internal literal. This shares the render journal's explicit build
// opt-in; ordinary builds do no I/O, and observation never controls the outcome.
inline void journal_graph_transaction(const char* phase,const unsigned char* id,unsigned stage,bool committed,
    long error,long rollback,long asset_rollback,long cleanup,long undo) noexcept {
#if defined(STARFIELD_GRAPH_RENDER_JOURNAL_FILE) && defined(_WIN32)
    try {
        char identity[33]{};constexpr char hex[]="0123456789abcdef";
        if(id)for(unsigned i=0;i<16;++i){identity[2*i]=hex[id[i]>>4];identity[2*i+1]=hex[id[i]&15];}
        char line[1024]{};
        const auto length=std::snprintf(line,sizeof(line),"{\"event\":\"transaction\",\"nativeBuild\":%d,\"pid\":%lu,\"thread\":%lu,\"tickMs\":%llu,\"id\":\"%s\",\"phase\":\"%s\",\"stage\":%u,\"committed\":%s,\"error\":%ld,\"rollback\":%ld,\"assetRollback\":%ld,\"cleanup\":%ld,\"undo\":%ld}\n",
            STARFIELD_VERSION_BUILD,static_cast<unsigned long>(GetCurrentProcessId()),static_cast<unsigned long>(GetCurrentThreadId()),
            static_cast<unsigned long long>(GetTickCount64()),identity,phase,stage,committed?"true":"false",error,rollback,asset_rollback,cleanup,undo);
        if(length>0 && static_cast<std::size_t>(length)<sizeof(line))graph_journal_detail::append(line,static_cast<DWORD>(length));
    }catch(...) {}
#else
    (void)phase;(void)id;(void)stage;(void)committed;(void)error;(void)rollback;(void)asset_rollback;(void)cleanup;(void)undo;
#endif
}
// The synchronous graph command is also used by initialization/reconciliation,
// before a Model transaction may exist. Keep its own native failure detail.
inline void journal_graph_command(long nonce,long error,const char* message) noexcept {
#if defined(STARFIELD_GRAPH_RENDER_JOURNAL_FILE) && defined(_WIN32)
    try {
        char line[4096]{};
        const auto header=std::snprintf(line,sizeof(line),"{\"event\":\"graph-command\",\"nativeBuild\":%d,\"pid\":%lu,\"thread\":%lu,\"tickMs\":%llu,\"nonce\":%ld,\"result\":%ld,\"message\":\"",
            STARFIELD_VERSION_BUILD,static_cast<unsigned long>(GetCurrentProcessId()),static_cast<unsigned long>(GetCurrentThreadId()),
            static_cast<unsigned long long>(GetTickCount64()),nonce,error);
        if(header<0 || static_cast<std::size_t>(header)>=sizeof(line))return;
        auto used=static_cast<std::size_t>(header);
        if(message)for(unsigned i=0;message[i] && i<512;++i){
            if(used+9>=sizeof(line))break;
            const auto c=static_cast<unsigned char>(message[i]);
            if(c=='"' || c=='\\'){line[used++]='\\';line[used++]=static_cast<char>(c);}
            else if(c<32){std::snprintf(line+used,sizeof(line)-used,"\\u%04x",static_cast<unsigned>(c));used+=6;}
            else line[used++]=static_cast<char>(c);
        }
        line[used++]='"';line[used++]='}';line[used++]='\n';
        graph_journal_detail::append(line,static_cast<DWORD>(used));
    }catch(...){}
#else
    (void)nonce;(void)error;(void)message;
#endif
}
}
