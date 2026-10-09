#include "ModelAssetWrite.hpp"
#include "ModelGeometryParameter.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "starfield/core/ModelGeometry.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <new>

namespace starfield::adapter {
namespace {
namespace layout=native_nodes::model_layout;
template<class T> struct Suite {
    SPBasicSuite* basic;const char* name;int version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,int v):basic(b),name(n),version(v){if(basic)basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
struct Cancel final:core::Cancellation {
    const ModelAssetWriteRequest& request;
    explicit Cancel(const ModelAssetWriteRequest& r):request(r){}
    bool is_cancelled()const noexcept override{return request.is_cancelled && request.is_cancelled(request.cancellation_context)!=0;}
};
ModelAssetError geometry_error(core::ErrorCode code) noexcept {
    return code==core::ErrorCode::allocation_failed?ModelAssetError::allocation_failed:
        code==core::ErrorCode::cancelled?ModelAssetError::cancelled:ModelAssetError::invalid_geometry;
}
PF_Err compare(PF_InData* data,AEGP_ArbBlockVal expected,AEGP_ArbBlockVal observed) noexcept {
    PF_ArbParamsExtra extra{};extra.id=static_cast<A_short>(layout::disk_id(layout::mesh));extra.which_function=PF_Arbitrary_COMPARE_FUNC;
    PF_ArbCompareResult result{};extra.u.compare_func_params.a_arbH=reinterpret_cast<PF_ArbitraryH>(expected);
    extra.u.compare_func_params.b_arbH=reinterpret_cast<PF_ArbitraryH>(observed);extra.u.compare_func_params.compareP=&result;
    const auto error=model_geometry_arbitrary_callback(data,&extra,layout::disk_id(layout::mesh));
    return error?error:result==PF_ArbCompare_EQUAL?PF_Err_NONE:PF_Err_INTERNAL_STRUCT_DAMAGED;
}
}
PF_Err write_model_asset(PF_InData* data,ModelAssetWriteRequest& request) noexcept try {
    if(request.magic!=0x53464d57 || request.bytes!=sizeof(request) || request.version!=1)return PF_Err_NONE;
    request.acknowledged=1;request.error=ModelAssetError::invalid_request;request.host_error=request.rollback_error=0;
    if(request.operation!=1 || request.expected_source<1 || request.expected_source>2 || request.desired_source<1 || request.desired_source>2 ||
        request.expected_revision>2147483647u || request.desired_revision>2147483647u || request.mesh_length>core::kMaxModelEncodedBytes ||
        (request.desired_revision==0?(request.mesh_length!=0 || request.mesh_bytes!=nullptr):(!request.mesh_bytes || request.mesh_length<32)) ||
        std::none_of(std::begin(request.expected_uuid),std::end(request.expected_uuid),[](auto byte){return byte!=0;}))return PF_Err_NONE;
    for(unsigned axis=0;axis<6;++axis)if(!std::isfinite(request.desired_bounds[axis]) || std::abs(request.desired_bounds[axis])>1e9 ||
        (axis>=3 && request.desired_bounds[axis]<request.desired_bounds[axis-3]))return PF_Err_NONE;
    request.error=ModelAssetError::unavailable;
    const auto plugin=node_graph_sync_plugin_id();
    if(!data || !data->pica_basicP || !data->effect_ref || !data->utils || !data->time_scale || !plugin ||
        (data->in_flags&PF_InFlag_PROJECT_IS_RENDER_ONLY))return PF_Err_NONE;
    Cancel cancel(request);
    auto decoded=request.desired_revision?core::decode_model_geometry(
        {reinterpret_cast<const std::byte*>(request.mesh_bytes),request.mesh_length},cancel):core::make_unit_cube();
    if(!decoded.has_value()){request.error=geometry_error(decoded.error().code);return PF_Err_NONE;}
    const auto& box=decoded.value().bounds;
    const std::array<double,6> bounds{box.minimum.x,box.minimum.y,box.minimum.z,box.maximum.x,box.maximum.y,box.maximum.z};
    if(!std::equal(bounds.begin(),bounds.end(),std::begin(request.desired_bounds))){request.error=ModelAssetError::invalid_geometry;return PF_Err_NONE;}
    if(cancel.is_cancelled()){request.error=ModelAssetError::cancelled;return PF_Err_NONE;}
    PF_ArbitraryH candidate{};
    const auto prepared=create_model_geometry_parameter(data,decoded.value(),&candidate,cancel);
    if(prepared){request.error=prepared==PF_Err_OUT_OF_MEMORY?ModelAssetError::allocation_failed:
        prepared==PF_Interrupt_CANCEL?ModelAssetError::cancelled:ModelAssetError::invalid_geometry;return PF_Err_NONE;}
    struct Mesh {PF_InData* data;PF_ArbitraryH handle;~Mesh(){data->utils->host_dispose_handle(handle);}} mesh{data,candidate};
    Suite<AEGP_PFInterfaceSuite1> pf(data->pica_basicP,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    Suite<AEGP_EffectSuite4> effects(data->pica_basicP,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(data->pica_basicP,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    if(!pf.value || !effects.value || !streams.value)return PF_Err_NONE;
    struct Field {AEGP_StreamRefH ref{};AEGP_StreamValue2 old{};bool read{},changed{};};
    struct Owned {
        const AEGP_StreamSuite6* streams;const AEGP_EffectSuite4* effects;AEGP_EffectRefH effect{};std::array<Field,10> fields{};
        ~Owned(){for(auto& f:fields){if(f.read)streams->AEGP_DisposeStreamValue(&f.old);if(f.ref)streams->AEGP_DisposeStream(f.ref);}
            if(effect)effects->AEGP_DisposeEffect(effect);}
    } owned{streams.value,effects.value};
    constexpr auto guard=native_nodes::sync_guard_index(native_nodes::Kind::model);
    constexpr std::array<A_long,10> indices{guard,layout::mesh,layout::revision,layout::source,
        layout::author_bounds_first,layout::author_bounds_first+1,layout::author_bounds_first+2,
        layout::author_bounds_first+3,layout::author_bounds_first+4,layout::author_bounds_first+5};
    request.error=ModelAssetError::host_error;
    auto error=pf.value->AEGP_GetNewEffectForEffect(plugin,data->effect_ref,&owned.effect);
    if(error || !owned.effect){request.host_error=error?error:PF_Err_BAD_CALLBACK_PARAM;return PF_Err_NONE;}
    const A_Time time{data->current_time,data->time_scale};
    for(unsigned i=0;i<indices.size();++i){auto& field=owned.fields[i];
        error=streams.value->AEGP_GetNewEffectStreamByIndex(plugin,owned.effect,indices[i],&field.ref);
        if(error || !field.ref){request.host_error=error?error:PF_Err_BAD_CALLBACK_PARAM;return PF_Err_NONE;}
        AEGP_StreamType type{};error=streams.value->AEGP_GetStreamType(field.ref,&type);
        if(error || type!=(i==1?AEGP_StreamType_ARB:AEGP_StreamType_OneD)){request.host_error=error?error:PF_Err_BAD_CALLBACK_PARAM;return PF_Err_NONE;}
        error=streams.value->AEGP_GetNewStreamValue(plugin,field.ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&field.old);
        if(error){request.host_error=error;return PF_Err_NONE;}field.read=true;
    }
    request.error=ModelAssetError::stale_author;
    if(owned.fields[0].old.val.one_d!=0 || owned.fields[2].old.val.one_d!=request.expected_revision ||
        owned.fields[3].old.val.one_d!=request.expected_source)return PF_Err_NONE;
    for(A_long part=0;part<8;++part){AEGP_StreamRefH ref{};
        request.error=ModelAssetError::host_error;
        error=streams.value->AEGP_GetNewEffectStreamByIndex(plugin,owned.effect,native_nodes::uuid_first_index(native_nodes::Kind::model)+part,&ref);
        if(error || !ref){request.host_error=error?error:PF_Err_BAD_CALLBACK_PARAM;return PF_Err_NONE;}
        struct Ref {const AEGP_StreamSuite6* suite;AEGP_StreamRefH ref;~Ref(){suite->AEGP_DisposeStream(ref);}} uuid{streams.value,ref};
        AEGP_StreamType type{};error=streams.value->AEGP_GetStreamType(ref,&type);AEGP_StreamValue2 value{};
        if(!error && type!=AEGP_StreamType_OneD)error=PF_Err_BAD_CALLBACK_PARAM;
        if(!error)error=streams.value->AEGP_GetNewStreamValue(plugin,ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&value);
        if(error){request.host_error=error;return PF_Err_NONE;}
        const auto number=value.val.one_d;error=streams.value->AEGP_DisposeStreamValue(&value);
        if(error){request.host_error=error;return PF_Err_NONE;}
        request.error=ModelAssetError::stale_author;
        if(number!=((std::uint32_t(request.expected_uuid[part*2])<<8)|request.expected_uuid[part*2+1]))return PF_Err_NONE;
    }
    auto old_mesh=read_model_geometry_parameter(data,reinterpret_cast<PF_ArbitraryH>(owned.fields[1].old.val.arbH),cancel);
    if(!old_mesh.has_value()){request.error=geometry_error(old_mesh.error().code);return PF_Err_NONE;}
    const auto& old_box=old_mesh.value().bounds;
    const double old_bounds[]{old_box.minimum.x,old_box.minimum.y,old_box.minimum.z,old_box.maximum.x,old_box.maximum.y,old_box.maximum.z};
    for(unsigned axis=0;axis<6;++axis)if(owned.fields[4+axis].old.val.one_d!=old_bounds[axis]){request.error=ModelAssetError::stale_author;return PF_Err_NONE;}
    if(cancel.is_cancelled()){request.error=ModelAssetError::cancelled;return PF_Err_NONE;}
    const auto verify=[&](unsigned i,const AEGP_StreamValue2& expected)->A_Err {
        AEGP_StreamValue2 observed{};auto code=streams.value->AEGP_GetNewStreamValue(plugin,owned.fields[i].ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&observed);
        if(code)return code;
        const auto checked=i==1?compare(data,expected.val.arbH,observed.val.arbH):
            expected.val.one_d==observed.val.one_d?PF_Err_NONE:PF_Err_INTERNAL_STRUCT_DAMAGED;
        const auto disposed=streams.value->AEGP_DisposeStreamValue(&observed);return checked?checked:disposed;
    };
    const auto write=[&](unsigned i,AEGP_StreamValue2 desired)->A_Err {
        owned.fields[i].changed=true;const auto code=streams.value->AEGP_SetStreamValue(plugin,owned.fields[i].ref,&desired);
        return code?code:verify(i,desired);
    };
    const auto scalar=[&](unsigned i,double number)->A_Err {AEGP_StreamValue2 value{};value.streamH=owned.fields[i].ref;value.val.one_d=number;return write(i,value);};
    request.error=ModelAssetError::host_error;error=scalar(0,1);
    if(!error){AEGP_StreamValue2 value{};value.streamH=owned.fields[1].ref;value.val.arbH=reinterpret_cast<AEGP_ArbBlockVal>(candidate);error=write(1,value);}
    for(unsigned axis=0;axis<6 && !error;++axis)error=scalar(4+axis,bounds[axis]);
    if(!error)error=scalar(2,request.desired_revision);
    if(!error)error=scalar(3,request.desired_source);
    if(!error && cancel.is_cancelled()){request.error=ModelAssetError::cancelled;error=PF_Interrupt_CANCEL;}
    if(!error)error=scalar(0,0);
    if(!error && cancel.is_cancelled()){request.error=ModelAssetError::cancelled;error=PF_Interrupt_CANCEL;}
    if(!error){request.error=ModelAssetError::none;return PF_Err_NONE;}
    request.host_error=error;
    request.rollback_error=scalar(0,1);
    for(unsigned i=static_cast<unsigned>(owned.fields.size());i-->1;)if(owned.fields[i].changed){
        const auto restored=streams.value->AEGP_SetStreamValue(plugin,owned.fields[i].ref,&owned.fields[i].old);
        const auto checked=restored?restored:verify(i,owned.fields[i].old);if(checked)request.rollback_error=checked;}
    const auto reset=streams.value->AEGP_SetStreamValue(plugin,owned.fields[0].ref,&owned.fields[0].old);
    const auto checked=reset?reset:verify(0,owned.fields[0].old);if(checked)request.rollback_error=checked;
    return PF_Err_NONE;
} catch(const std::bad_alloc&){request.error=ModelAssetError::allocation_failed;return PF_Err_NONE;}
  catch(...){request.error=ModelAssetError::host_error;return PF_Err_NONE;}
}
