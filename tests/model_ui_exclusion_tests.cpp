#include "UiExclusionClient.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
unsigned checks{},messages{};bool blocked{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" at "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
using Begin=std::uint64_t(*)() noexcept;
using End=void(*)(std::uint64_t) noexcept;
using Notify=void(*)() noexcept;
Begin begin{};End end{};
LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    if(message==WM_APP){
        CHECK(GetCurrentThreadId()==static_cast<DWORD>(wp));
        {starfield::adapter::EffectUiExclusion scope;CHECK(bool(scope)==!blocked);}
        const auto token=begin();CHECK((token==0)==blocked);if(token)end(token);
        ++messages;return 0;
    }
    return DefWindowProcW(window,message,wp,lp);
}
void pump(HWND window) {
    CHECK(PostMessageW(window,WM_APP,GetCurrentThreadId(),0));
    MSG message{};CHECK(GetMessageW(&message,window,WM_APP,WM_APP)==1);DispatchMessageW(&message);
}
}
int wmain(int argc,wchar_t** argv) {
    CHECK(argc==3);CHECK(!GetModuleHandleW(L"StarfieldHost.aex"));
    {starfield::adapter::EffectUiExclusion absent;CHECK(!absent);}
    auto module=LoadLibraryW(argv[1]);CHECK(module);
    {starfield::adapter::EffectUiExclusion mismatch;CHECK(!mismatch);}
    CHECK(FreeLibrary(module));CHECK(!GetModuleHandleW(L"StarfieldHost.aex"));
    module=LoadLibraryW(argv[2]);CHECK(module);
    const auto initialize=std::bit_cast<Notify>(GetProcAddress(module,"SFLD_TestInitialize"));
    const auto stop=std::bit_cast<Notify>(GetProcAddress(module,"SFLD_TestStop"));
    begin=std::bit_cast<Begin>(GetProcAddress(module,"SFLD_BeginUiExclusionV1"));
    end=std::bit_cast<End>(GetProcAddress(module,"SFLD_EndUiExclusionV1"));CHECK(initialize && stop && begin && end);
    {starfield::adapter::EffectUiExclusion uninitialized;CHECK(!uninitialized);}
    initialize();
    WNDCLASSW type{};type.lpfnWndProc=window_proc;type.hInstance=GetModuleHandleW(nullptr);type.lpszClassName=L"StarfieldUiExclusionTest";
    CHECK(RegisterClassW(&type));
    const auto window=CreateWindowExW(0,type.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,type.hInstance,nullptr);CHECK(window);
    pump(window);
    for(const auto* location:{"Presets executeCommand","Presets alert","OBJ chooser","Editor preset chooser","Texture source chooser"}) {
        (void)location;starfield::adapter::EffectUiExclusion modal;CHECK(modal);blocked=true;
        for(unsigned i=0;i<16;++i)pump(window);
        {starfield::adapter::EffectUiExclusion nested;CHECK(!nested);}
        const auto token=begin();CHECK(token==0);
        std::uint64_t worker_token{1};std::thread worker([&]{worker_token=begin();end(1);});worker.join();CHECK(worker_token==0);
        CHECK(begin()==0);blocked=false;
    }
    pump(window);
    auto token=begin();CHECK(token);end(token+1);CHECK(begin()==0);end(token);
    const auto next=begin();CHECK(next && next!=token);end(token);CHECK(begin()==0);end(next);
    try {starfield::adapter::EffectUiExclusion scope;CHECK(scope);throw std::runtime_error("callback failed");}catch(const std::runtime_error&){}
    pump(window);
    {starfield::adapter::EffectUiExclusion scope;CHECK(scope);stop();CHECK(begin()==0);}
    CHECK(begin()==0);initialize();pump(window);
    CHECK(messages==84);CHECK(DestroyWindow(window));CHECK(UnregisterClassW(type.lpszClassName,type.hInstance));
    CHECK(FreeLibrary(module));
    {starfield::adapter::EffectUiExclusion unloaded;CHECK(!unloaded);}
    std::cout<<"Model UI exclusion: "<<checks<<" checks, 0 failures (actual cross-module Windows message pump; no AE qualification)\n";
}
