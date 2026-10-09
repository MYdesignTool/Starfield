#include "ModelAssetExport.hpp"
#include "ModelGeometryParameter.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <new>

namespace starfield::adapter {
namespace {
namespace layout=native_nodes::model_layout;
template<class T>struct Suite {
    SPBasicSuite* basic;const char* name;int version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,int v):basic(b),name(n),version(v){
        if(basic)basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
struct Cancel final:core::Cancellation {
    const ModelAssetExportRequest& request;
    explicit Cancel(const ModelAssetExportRequest& r):request(r){}
    bool is_cancelled()const noexcept override{return request.is_cancelled && request.is_cancelled(request.cancellation_context)!=0;}
};
ModelAssetError map_error(core::ErrorCode code) noexcept {
    return code==core::ErrorCode::allocation_failed?ModelAssetError::allocation_failed:
        code==core::ErrorCode::cancelled?ModelAssetError::cancelled:ModelAssetError::invalid_geometry;
}
}
PF_Err export_model_asset(PF_InData* data,ModelAssetExportRequest& request) noexcept try {
    if(request.magic!=0x53464d58 || request.bytes!=sizeof(request) || request.version!=1)return PF_Err_NONE;
    request.acknowledged=1;request.error=ModelAssetError::invalid_request;request.host_error=0;request.payload_bytes=0;
    std::fill(std::begin(request.bounds),std::end(request.bounds),0);
    if(request.operation!=1 || request.expected_source<1 || request.expected_source>2 || request.expected_revision>2147483647u ||
       (request.expected_revision && !request.write_bytes) ||
       std::none_of(std::begin(request.expected_uuid),std::end(request.expected_uuid),[](auto b){return b!=0;}))return PF_Err_NONE;
    request.error=ModelAssetError::unavailable;
    const auto plugin=node_graph_sync_plugin_id();
    if(!data || !data->pica_basicP || !data->effect_ref || !data->utils || !data->time_scale || !plugin ||
       (data->in_flags&PF_InFlag_PROJECT_IS_RENDER_ONLY))return PF_Err_NONE;
    Cancel cancel(request);if(cancel.is_cancelled()){request.error=ModelAssetError::cancelled;return PF_Err_NONE;}
    Suite<AEGP_PFInterfaceSuite1> pf(data->pica_basicP,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    Suite<AEGP_EffectSuite4> effects(data->pica_basicP,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(data->pica_basicP,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    if(!pf.value || !effects.value || !streams.value)return PF_Err_NONE;
    AEGP_EffectRefH effect{};
    auto error=pf.value->AEGP_GetNewEffectForEffect(plugin,data->effect_ref,&effect);
    if(error || !effect){request.error=ModelAssetError::host_error;request.host_error=error?error:PF_Err_BAD_CALLBACK_PARAM;return PF_Err_NONE;}
    struct Effect{const AEGP_EffectSuite4* suite;AEGP_EffectRefH value;~Effect(){suite->AEGP_DisposeEffect(value);}} owned{effects.value,effect};
    const A_Time time{data->current_time,data->time_scale};
    const auto read=[&](A_long index,AEGP_StreamType type,AEGP_StreamValue2& value)->bool {
        AEGP_StreamRefH ref{};auto e=streams.value->AEGP_GetNewEffectStreamByIndex(plugin,effect,index,&ref);
        if(e || !ref){request.host_error=e?e:PF_Err_BAD_CALLBACK_PARAM;return false;}
        // A returned value owns its stream reference until both are disposed.
        AEGP_StreamType actual{};e=streams.value->AEGP_GetStreamType(ref,&actual);
        if(!e && actual!=type)e=PF_Err_BAD_CALLBACK_PARAM;
        if(!e)e=streams.value->AEGP_GetNewStreamValue(plugin,ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&value);
        if(e){streams.value->AEGP_DisposeStream(ref);request.host_error=e;return false;}
        value.streamH=ref;return true;
    };
    const auto release=[&](AEGP_StreamValue2& value)->bool {
        const auto ref=value.streamH;const auto disposed=streams.value->AEGP_DisposeStreamValue(&value);
        const auto freed=streams.value->AEGP_DisposeStream(ref);value={};
        if(disposed || freed){request.host_error=disposed?disposed:freed;return false;}return true;
    };
    const auto number=[&](A_long index,double& output)->bool {
        AEGP_StreamValue2 value{};if(!read(index,AEGP_StreamType_OneD,value))return false;
        output=value.val.one_d;if(!release(value))return false;
        if(!std::isfinite(output)){request.host_error=PF_Err_BAD_CALLBACK_PARAM;return false;}return true;
    };
    request.error=ModelAssetError::host_error;
    double guard{},source{},revision{};
    if(!number(native_nodes::sync_guard_index(native_nodes::Kind::model),guard) ||
       !number(layout::source,source) || !number(layout::revision,revision))return PF_Err_NONE;
    request.error=ModelAssetError::stale_author;
    if(guard!=0 || source!=request.expected_source || revision!=request.expected_revision)return PF_Err_NONE;
    for(A_long chunk=0;chunk<8;++chunk){double part{};request.error=ModelAssetError::host_error;
        if(!number(native_nodes::uuid_first_index(native_nodes::Kind::model)+chunk,part))return PF_Err_NONE;
        request.error=ModelAssetError::stale_author;
        if(part<0 || part>65535 || std::floor(part)!=part ||
           part!=((std::uint32_t(request.expected_uuid[chunk*2])<<8)|request.expected_uuid[chunk*2+1]))return PF_Err_NONE;}
    if(!request.expected_revision){request.error=ModelAssetError::none;return PF_Err_NONE;}
    std::array<double,6> bounds{};
    for(A_long axis=0;axis<6;++axis){request.error=ModelAssetError::host_error;
        if(!number(layout::author_bounds_first+axis,bounds[axis]))return PF_Err_NONE;
        request.error=ModelAssetError::invalid_geometry;
        if(std::abs(bounds[axis])>1e9 || (axis>=3 && bounds[axis]<bounds[axis-3]))return PF_Err_NONE;}
    AEGP_StreamValue2 value{};request.error=ModelAssetError::host_error;
    if(!read(layout::mesh,AEGP_StreamType_ARB,value))return PF_Err_NONE;
    auto mesh=read_model_geometry_parameter(data,reinterpret_cast<PF_ArbitraryH>(value.val.arbH),cancel);
    if(!release(value))return PF_Err_NONE;
    if(!mesh.has_value()){request.error=map_error(mesh.error().code);return PF_Err_NONE;}
    const auto& box=mesh.value().bounds;
    const std::array<double,6> observed{box.minimum.x,box.minimum.y,box.minimum.z,box.maximum.x,box.maximum.y,box.maximum.z};
    if(observed!=bounds){request.error=ModelAssetError::stale_author;return PF_Err_NONE;}
    auto bytes=core::encode_model_geometry(mesh.value(),cancel);
    if(!bytes.has_value()){request.error=map_error(bytes.error().code);return PF_Err_NONE;}
    if(cancel.is_cancelled()){request.error=ModelAssetError::cancelled;return PF_Err_NONE;}
    if(request.write_bytes(request.sink_context,reinterpret_cast<const std::uint8_t*>(bytes.value().data()),
        static_cast<std::uint32_t>(bytes.value().size()))!=0){request.error=ModelAssetError::sink_rejected;return PF_Err_NONE;}
    request.payload_bytes=static_cast<std::uint32_t>(bytes.value().size());std::copy(bounds.begin(),bounds.end(),std::begin(request.bounds));
    request.error=ModelAssetError::none;return PF_Err_NONE;
} catch(const std::bad_alloc&){request.error=ModelAssetError::allocation_failed;return PF_Err_NONE;}
  catch(...){request.error=ModelAssetError::host_error;return PF_Err_NONE;}
}
