#include "ModelGeometryParameter.hpp"
#include "AE_EffectCB.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <stdexcept>

bool fail_cpp_allocation=false;
void* operator new(std::size_t size){if(fail_cpp_allocation){fail_cpp_allocation=false;throw std::bad_alloc();}if(auto* p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
using namespace starfield;
namespace {
unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#x);}}while(0)
struct Memory{core::OpaqueBytes bytes;unsigned locks{};A_u_longlong reported_size{};};
std::map<PF_Handle,std::unique_ptr<Memory>> handles;
bool fail_host_allocation{},fail_lock{};unsigned lock_calls{};
PF_Handle allocate(A_u_longlong size){if(fail_host_allocation){fail_host_allocation=false;return nullptr;}
    try{auto m=std::make_unique<Memory>();m->bytes.resize(static_cast<std::size_t>(size));auto h=new void*(m->bytes.data());handles.emplace(h,std::move(m));return h;}catch(const std::bad_alloc&){return nullptr;}}
void* lock(PF_Handle h){++lock_calls;if(fail_lock){fail_lock=false;return nullptr;}auto i=handles.find(h);if(i==handles.end())return nullptr;++i->second->locks;return i->second->bytes.data();}
void unlock(PF_Handle h){CHECK(handles.contains(h)&&handles.at(h)->locks);if(handles.contains(h)&&handles.at(h)->locks)--handles.at(h)->locks;}
void dispose(PF_Handle h){if(!h)return;CHECK(handles.contains(h)&&!handles.at(h)->locks);if(handles.contains(h)){handles.erase(h);delete h;}}
A_u_longlong size_of(PF_Handle h){auto i=handles.find(h);return i==handles.end()?0:i->second->reported_size?i->second->reported_size:i->second->bytes.size();}
PF_ArbParamsExtra call_for(PF_FunctionSelector function){PF_ArbParamsExtra call{};call.id=1500;call.which_function=function;return call;}
PF_Err invoke(PF_InData& host,PF_ArbParamsExtra& call){return adapter::model_geometry_arbitrary_callback(&host,&call,1500);}
core::NeverCancelled never;
template<class T>T take(core::Result<T> result){if(!result.has_value())throw std::runtime_error(result.error().detail);return result.take_value();}
PF_ArbitraryH new_mesh(PF_InData& host){PF_ArbitraryH h=nullptr;auto call=call_for(PF_Arbitrary_NEW_FUNC);call.u.new_func_params.arbPH=&h;CHECK(invoke(host,call)==PF_Err_NONE&&h);return h;}
void clear(){while(!handles.empty())dispose(handles.begin()->first);}
struct UnlockedCancellation:core::Cancellation{bool is_cancelled()const noexcept override{for(const auto& [h,m]:handles)if(m->locks)return true;return false;}} unlocked;
struct Cancelled:core::Cancellation{bool is_cancelled()const noexcept override{return true;}} cancelled;
void selectors(PF_InData& host){
    auto original=new_mesh(host);auto mesh=take(adapter::read_model_geometry_parameter(&host,original,unlocked));CHECK(mesh.positions.size()==8&&mesh.triangles.size()==12);
    const auto canonical=take(core::encode_model_geometry(mesh,never));CHECK(handles.at(original)->bytes==canonical);
    PF_ArbitraryH copy=nullptr;auto call=call_for(PF_Arbitrary_COPY_FUNC);call.u.copy_func_params.src_arbH=original;call.u.copy_func_params.dst_arbPH=&copy;
    CHECK(invoke(host,call)==PF_Err_NONE&&copy&&copy!=original&&handles.at(copy)->bytes==canonical);
    A_u_long flat_size=0;call=call_for(PF_Arbitrary_FLAT_SIZE_FUNC);call.u.flat_size_func_params.arbH=original;call.u.flat_size_func_params.flat_data_sizePLu=&flat_size;
    CHECK(invoke(host,call)==PF_Err_NONE&&flat_size==canonical.size());
    core::OpaqueBytes flat(flat_size,std::byte{0x7f});call=call_for(PF_Arbitrary_FLATTEN_FUNC);call.u.flatten_func_params.arbH=original;call.u.flatten_func_params.flat_dataPV=flat.data();call.u.flatten_func_params.buf_sizeLu=flat_size-1;
    CHECK(invoke(host,call)==PF_Err_BAD_CALLBACK_PARAM&&flat.front()==std::byte{0x7f});call.u.flatten_func_params.buf_sizeLu=flat_size;CHECK(invoke(host,call)==PF_Err_NONE&&flat==canonical);
    PF_ArbitraryH restored=nullptr;call=call_for(PF_Arbitrary_UNFLATTEN_FUNC);call.u.unflatten_func_params.flat_dataPV=flat.data();call.u.unflatten_func_params.buf_sizeLu=flat_size;call.u.unflatten_func_params.arbPH=&restored;
    CHECK(invoke(host,call)==PF_Err_NONE&&restored&&handles.at(restored)->bytes==canonical);
    A_u_long text_size=0;call=call_for(PF_Arbitrary_PRINT_SIZE_FUNC);call.u.print_size_func_params.arbH=restored;call.u.print_size_func_params.print_sizePLu=&text_size;
    CHECK(invoke(host,call)==PF_Err_NONE&&text_size==9+canonical.size()*2+1);
    std::vector<char> text(text_size,'?');call=call_for(PF_Arbitrary_PRINT_FUNC);call.u.print_func_params.arbH=restored;call.u.print_func_params.print_bufferPC=text.data();call.u.print_func_params.print_sizeLu=text_size-1;
    CHECK(invoke(host,call)==PF_Err_BAD_CALLBACK_PARAM&&text.front()=='?');call.u.print_func_params.print_sizeLu=text_size;CHECK(invoke(host,call)==PF_Err_NONE&&text.back()=='\0');
    PF_ArbitraryH scanned=nullptr;call=call_for(PF_Arbitrary_SCAN_FUNC);call.u.scan_func_params.bufPC=text.data();call.u.scan_func_params.bytes_to_scanLu=text_size;call.u.scan_func_params.arbPH=&scanned;
    CHECK(invoke(host,call)==PF_Err_NONE&&handles.at(scanned)->bytes==canonical);dispose(scanned);scanned=nullptr;--call.u.scan_func_params.bytes_to_scanLu;
    CHECK(invoke(host,call)==PF_Err_NONE&&handles.at(scanned)->bytes==canonical);dispose(scanned);
    for(auto& c:text)if(c>='a'&&c<='f')c=static_cast<char>(c-'a'+'A');scanned=nullptr;
    CHECK(invoke(host,call)==PF_Err_NONE&&handles.at(scanned)->bytes==canonical);dispose(scanned);
    PF_ArbCompareResult comparison=PF_ArbCompare_NOT_EQUAL;call=call_for(PF_Arbitrary_COMPARE_FUNC);call.u.compare_func_params.a_arbH=original;call.u.compare_func_params.b_arbH=restored;call.u.compare_func_params.compareP=&comparison;
    CHECK(invoke(host,call)==PF_Err_NONE&&comparison==PF_ArbCompare_EQUAL);
    mesh.positions[0].value.x-=.25;PF_ArbitraryH changed=nullptr;CHECK(adapter::create_model_geometry_parameter(&host,mesh,&changed,never)==PF_Err_NONE);call.u.compare_func_params.b_arbH=changed;
    CHECK(invoke(host,call)==PF_Err_NONE&&comparison==PF_ArbCompare_NOT_EQUAL);
    for(double fraction:{0.,.5,.999,1.}){PF_ArbitraryH interpolated=nullptr;call=call_for(PF_Arbitrary_INTERP_FUNC);call.u.interp_func_params.left_arbH=original;call.u.interp_func_params.right_arbH=changed;call.u.interp_func_params.tF=fraction;call.u.interp_func_params.interpPH=&interpolated;
        CHECK(invoke(host,call)==PF_Err_NONE&&interpolated&&handles.at(interpolated)->bytes==handles.at(fraction<1?original:changed)->bytes);dispose(interpolated);}
    handles.at(copy)->bytes.back()^=std::byte{1};CHECK(handles.at(original)->bytes==canonical);
    call=call_for(PF_Arbitrary_DISPOSE_FUNC);call.u.dispose_func_params.arbH=copy;CHECK(invoke(host,call)==PF_Err_NONE);clear();
}
void bad_data(PF_InData& host){
    auto h=new_mesh(host);const auto encoded=handles.at(h)->bytes;
    for(std::size_t n=0;n<encoded.size();++n){PF_ArbitraryH out=reinterpret_cast<PF_ArbitraryH>(1);auto call=call_for(PF_Arbitrary_UNFLATTEN_FUNC);call.u.unflatten_func_params.flat_dataPV=const_cast<std::byte*>(encoded.data());call.u.unflatten_func_params.buf_sizeLu=static_cast<A_u_long>(n);call.u.unflatten_func_params.arbPH=&out;
        CHECK(invoke(host,call)==PF_Err_BAD_CALLBACK_PARAM&&!out);}
    for(std::size_t n=0;n<encoded.size();++n){handles.at(h)->bytes=encoded;handles.at(h)->bytes[n]^=std::byte{1};
        CHECK(!adapter::read_model_geometry_parameter(&host,h,never).has_value());
        PF_ArbitraryH out=nullptr;auto call=call_for(PF_Arbitrary_COPY_FUNC);call.u.copy_func_params.src_arbH=h;call.u.copy_func_params.dst_arbPH=&out;CHECK(invoke(host,call)==PF_Err_BAD_CALLBACK_PARAM&&!out);}
    handles.at(h)->bytes=encoded;
    handles.at(h)->bytes.back()^=std::byte{1};
    A_u_long invalid_size=123;auto invalid_call=call_for(PF_Arbitrary_FLAT_SIZE_FUNC);invalid_call.u.flat_size_func_params.arbH=h;invalid_call.u.flat_size_func_params.flat_data_sizePLu=&invalid_size;
    CHECK(invoke(host,invalid_call)==PF_Err_BAD_CALLBACK_PARAM&&invalid_size==0);
    std::vector<std::byte> untouched(encoded.size(),std::byte{0x6a});invalid_call=call_for(PF_Arbitrary_FLATTEN_FUNC);invalid_call.u.flatten_func_params.arbH=h;invalid_call.u.flatten_func_params.flat_dataPV=untouched.data();invalid_call.u.flatten_func_params.buf_sizeLu=static_cast<A_u_long>(untouched.size());
    CHECK(invoke(host,invalid_call)==PF_Err_BAD_CALLBACK_PARAM&&untouched.front()==std::byte{0x6a});
    invalid_call=call_for(PF_Arbitrary_PRINT_SIZE_FUNC);invalid_call.u.print_size_func_params.arbH=h;invalid_call.u.print_size_func_params.print_sizePLu=&invalid_size;
    CHECK(invoke(host,invalid_call)==PF_Err_BAD_CALLBACK_PARAM&&invalid_size==0);
    handles.at(h)->bytes=encoded;
    PF_ArbitraryH out=nullptr;for(double fraction:{-1.,2.,std::numeric_limits<double>::quiet_NaN()}){auto call=call_for(PF_Arbitrary_INTERP_FUNC);call.u.interp_func_params.left_arbH=h;call.u.interp_func_params.right_arbH=h;call.u.interp_func_params.tF=fraction;call.u.interp_func_params.interpPH=&out;CHECK(invoke(host,call)==PF_Err_BAD_CALLBACK_PARAM&&!out);}
    for(const char* text:{"","SFMODEL1:","SFMODEL1:xyz","SFOTHER1:0123456789","SFMODEL1:0"}){auto call=call_for(PF_Arbitrary_SCAN_FUNC);call.u.scan_func_params.bufPC=text;call.u.scan_func_params.bytes_to_scanLu=static_cast<A_u_long>(std::strlen(text));call.u.scan_func_params.arbPH=&out;CHECK(invoke(host,call)==PF_Err_CANNOT_PARSE_KEYFRAME_TEXT&&!out);}
    auto call=call_for(PF_Arbitrary_SCAN_FUNC);call.u.scan_func_params.bufPC="x";call.u.scan_func_params.bytes_to_scanLu=static_cast<A_u_long>(core::kMaxModelEncodedBytes*2+11);call.u.scan_func_params.arbPH=&out;
    CHECK(invoke(host,call)==PF_Err_CANNOT_PARSE_KEYFRAME_TEXT&&!out);
    auto before=lock_calls;handles.at(h)->reported_size=core::kMaxModelEncodedBytes+1;
    CHECK(!adapter::read_model_geometry_parameter(&host,h,never).has_value()&&lock_calls==before);handles.at(h)->reported_size=0;
    CHECK(!adapter::read_model_geometry_parameter(nullptr,h,never).has_value());
    call=call_for(PF_Arbitrary_NEW_FUNC);call.u.new_func_params.arbPH=&out;call.id=1499;CHECK(invoke(host,call)==PF_Err_BAD_CALLBACK_PARAM&&!out);
    CHECK(adapter::model_geometry_arbitrary_callback(&host,nullptr,1500)==PF_Err_BAD_CALLBACK_PARAM);
    for(auto selector:{PF_Arbitrary_NEW_FUNC,PF_Arbitrary_COPY_FUNC,PF_Arbitrary_FLAT_SIZE_FUNC,PF_Arbitrary_FLATTEN_FUNC,PF_Arbitrary_UNFLATTEN_FUNC,PF_Arbitrary_INTERP_FUNC,PF_Arbitrary_COMPARE_FUNC,PF_Arbitrary_PRINT_SIZE_FUNC,PF_Arbitrary_PRINT_FUNC,PF_Arbitrary_SCAN_FUNC}){
        call=call_for(selector);CHECK(invoke(host,call)==PF_Err_BAD_CALLBACK_PARAM);}
    clear();
}
void errors(PF_InData& host){
    const auto mesh=take(core::make_unit_cube());PF_ArbitraryH out=nullptr;auto before=handles.size();
    fail_host_allocation=true;CHECK(adapter::create_model_geometry_parameter(&host,mesh,&out,never)==PF_Err_OUT_OF_MEMORY&&!out&&handles.size()==before);
    fail_lock=true;CHECK(adapter::create_model_geometry_parameter(&host,mesh,&out,never)==PF_Err_OUT_OF_MEMORY&&!out&&handles.size()==before);
    fail_cpp_allocation=true;CHECK(adapter::create_model_geometry_parameter(&host,mesh,&out,never)==PF_Err_OUT_OF_MEMORY&&!out&&handles.size()==before);
    CHECK(adapter::create_model_geometry_parameter(&host,mesh,&out,cancelled)==PF_Interrupt_CANCEL&&!out);
    auto h=new_mesh(host);fail_lock=true;auto result=adapter::read_model_geometry_parameter(&host,h,never);CHECK(!result.has_value()&&result.error().code==core::ErrorCode::allocation_failed);
    fail_cpp_allocation=true;result=adapter::read_model_geometry_parameter(&host,h,never);CHECK(!result.has_value()&&result.error().code==core::ErrorCode::allocation_failed&&!handles.at(h)->locks);
    result=adapter::read_model_geometry_parameter(&host,h,cancelled);CHECK(!result.has_value()&&result.error().code==core::ErrorCode::cancelled&&!handles.at(h)->locks);
    fail_cpp_allocation=true;auto call=call_for(PF_Arbitrary_COPY_FUNC);call.u.copy_func_params.src_arbH=h;call.u.copy_func_params.dst_arbPH=&out;CHECK(invoke(host,call)==PF_Err_OUT_OF_MEMORY&&!out&&!handles.at(h)->locks);
    auto invalid=mesh;invalid.positions[0].value.x=std::numeric_limits<double>::quiet_NaN();CHECK(adapter::create_model_geometry_parameter(&host,invalid,&out,never)==PF_Err_BAD_CALLBACK_PARAM&&!out);
    const auto imported=take(core::parse_model_obj("v -1 -1 0\nv 1 -1 0\nv 1 1 0\nv -1 1 0\nf 1 2 3 4\n",never));
    CHECK(adapter::create_model_geometry_parameter(&host,imported,&out,never)==PF_Err_NONE&&out);
    const auto restored=take(adapter::read_model_geometry_parameter(&host,out,unlocked));
    CHECK(restored.positions.size()==4&&restored.triangles.size()==2&&take(core::encode_model_geometry(restored,never))==take(core::encode_model_geometry(imported,never)));
    clear();CHECK(handles.empty());
}
}
int main(){PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;PF_InData host{};host.utils=&utils;
    try{selectors(host);bad_data(host);errors(host);CHECK(handles.empty());std::printf("Model mesh parameter: %u checks, %u failures (fake host only)\n",checks,failures);return failures?1:0;}
    catch(const std::exception& e){std::printf("FAILED: %s\n",e.what());clear();return 1;}}
