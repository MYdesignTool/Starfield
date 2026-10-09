#include "ModelGeometryParameter.hpp"
#include "AE_EffectCB.h"
#include <cmath>
#include <cstring>
#include <new>
#include <string_view>

namespace starfield::adapter {
namespace {
constexpr std::size_t kMeshHeaderBytes=32;
constexpr std::string_view kTextPrefix="SFMODEL1:";
constexpr char kHex[]="0123456789abcdef";
core::NeverCancelled never;
bool handles_available(const PF_InData* data) noexcept {
    return data&&data->utils&&data->utils->host_new_handle&&data->utils->host_lock_handle&&
        data->utils->host_unlock_handle&&data->utils->host_dispose_handle&&data->utils->host_get_handle_size;
}
PF_Err host_error(const core::CoreError& error) noexcept {
    return error.code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:
        error.code==core::ErrorCode::cancelled?PF_Interrupt_CANCEL:PF_Err_BAD_CALLBACK_PARAM;
}
struct BorrowedMeshBytes {
    PF_InData* data;PF_ArbitraryH handle;void* pointer;
    ~BorrowedMeshBytes(){if(pointer)data->utils->host_unlock_handle(handle);}
};
core::Result<core::OpaqueBytes> copy_handle(PF_InData* data,PF_ArbitraryH handle) {
    using R=core::Result<core::OpaqueBytes>;
    if(!handles_available(data)||!handle)return R::failure(core::ErrorCode::invalid_request,"invalid Model mesh handle callbacks");
    const auto size=data->utils->host_get_handle_size(handle);
    if(size<kMeshHeaderBytes||size>core::kMaxModelEncodedBytes)
        return R::failure(core::ErrorCode::invalid_request,"invalid Model mesh handle size");
    BorrowedMeshBytes borrowed{data,handle,data->utils->host_lock_handle(handle)};
    if(!borrowed.pointer)return R::failure(core::ErrorCode::allocation_failed,"Model mesh handle lock failed");
    const auto* begin=static_cast<const std::byte*>(borrowed.pointer);
    return R::success(core::OpaqueBytes(begin,begin+size));
}
PF_Err write_handle(PF_InData* data,std::span<const std::byte> bytes,PF_ArbitraryH* output) {
    if(!output)return PF_Err_BAD_CALLBACK_PARAM;
    *output=nullptr;
    if(!handles_available(data)||bytes.size()<kMeshHeaderBytes||bytes.size()>core::kMaxModelEncodedBytes)
        return PF_Err_BAD_CALLBACK_PARAM;
    const auto handle=data->utils->host_new_handle(bytes.size());
    if(!handle)return PF_Err_OUT_OF_MEMORY;
    auto* pointer=data->utils->host_lock_handle(handle);
    if(!pointer){data->utils->host_dispose_handle(handle);return PF_Err_OUT_OF_MEMORY;}
    std::memcpy(pointer,bytes.data(),bytes.size());data->utils->host_unlock_handle(handle);
    *output=handle;return PF_Err_NONE;
}
core::Result<core::OpaqueBytes> validated_bytes(PF_InData* data,PF_ArbitraryH handle) {
    auto bytes=copy_handle(data,handle);if(!bytes.has_value())return bytes;
    auto mesh=core::decode_model_geometry(bytes.value(),never);
    if(!mesh.has_value())return core::Result<core::OpaqueBytes>::failure(mesh.error());
    return bytes;
}
PF_Err accept_bytes(PF_InData* data,std::span<const std::byte> bytes,PF_ArbitraryH* output) {
    if(!output)return PF_Err_BAD_CALLBACK_PARAM;
    *output=nullptr;
    auto mesh=core::decode_model_geometry(bytes,never);if(!mesh.has_value())return host_error(mesh.error());
    return write_handle(data,bytes,output);
}
PF_Err clone_mesh(PF_InData* data,PF_ArbitraryH input,PF_ArbitraryH* output) {
    if(!output)return PF_Err_BAD_CALLBACK_PARAM;
    *output=nullptr;
    auto bytes=validated_bytes(data,input);if(!bytes.has_value())return host_error(bytes.error());
    return write_handle(data,bytes.value(),output);
}
int hex_digit(char c) noexcept {
    return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
}
PF_Err dispatch(PF_InData* data,PF_ArbParamsExtra& extra) {
    switch(extra.which_function) {
    case PF_Arbitrary_NEW_FUNC: {
        auto* output=extra.u.new_func_params.arbPH;if(!output)return PF_Err_BAD_CALLBACK_PARAM;*output=nullptr;
        auto mesh=core::make_unit_cube();if(!mesh.has_value())return host_error(mesh.error());
        return create_model_geometry_parameter(data,mesh.value(),output,never);
    }
    case PF_Arbitrary_DISPOSE_FUNC:
        if(extra.u.dispose_func_params.arbH)data->utils->host_dispose_handle(extra.u.dispose_func_params.arbH);
        return PF_Err_NONE;
    case PF_Arbitrary_COPY_FUNC:
        return clone_mesh(data,extra.u.copy_func_params.src_arbH,extra.u.copy_func_params.dst_arbPH);
    case PF_Arbitrary_FLAT_SIZE_FUNC: {
        auto& p=extra.u.flat_size_func_params;if(!p.flat_data_sizePLu)return PF_Err_BAD_CALLBACK_PARAM;*p.flat_data_sizePLu=0;
        auto bytes=validated_bytes(data,p.arbH);if(!bytes.has_value())return host_error(bytes.error());
        *p.flat_data_sizePLu=static_cast<A_u_long>(bytes.value().size());return PF_Err_NONE;
    }
    case PF_Arbitrary_FLATTEN_FUNC: {
        auto& p=extra.u.flatten_func_params;if(!p.flat_dataPV)return PF_Err_BAD_CALLBACK_PARAM;
        auto bytes=validated_bytes(data,p.arbH);if(!bytes.has_value())return host_error(bytes.error());
        if(p.buf_sizeLu<bytes.value().size())return PF_Err_BAD_CALLBACK_PARAM;
        std::memcpy(p.flat_dataPV,bytes.value().data(),bytes.value().size());return PF_Err_NONE;
    }
    case PF_Arbitrary_UNFLATTEN_FUNC: {
        auto& p=extra.u.unflatten_func_params;if(!p.arbPH)return PF_Err_BAD_CALLBACK_PARAM;*p.arbPH=nullptr;
        if(!p.flat_dataPV||p.buf_sizeLu<kMeshHeaderBytes||p.buf_sizeLu>core::kMaxModelEncodedBytes)return PF_Err_BAD_CALLBACK_PARAM;
        return accept_bytes(data,{static_cast<const std::byte*>(p.flat_dataPV),p.buf_sizeLu},p.arbPH);
    }
    case PF_Arbitrary_INTERP_FUNC: {
        auto& p=extra.u.interp_func_params;if(!p.interpPH)return PF_Err_BAD_CALLBACK_PARAM;*p.interpPH=nullptr;
        if(!std::isfinite(p.tF)||p.tF<0||p.tF>1)return PF_Err_BAD_CALLBACK_PARAM;
        return clone_mesh(data,p.tF<1?p.left_arbH:p.right_arbH,p.interpPH);
    }
    case PF_Arbitrary_COMPARE_FUNC: {
        auto& p=extra.u.compare_func_params;if(!p.compareP)return PF_Err_BAD_CALLBACK_PARAM;*p.compareP=PF_ArbCompare_NOT_EQUAL;
        auto a=validated_bytes(data,p.a_arbH);if(!a.has_value())return host_error(a.error());
        auto b=validated_bytes(data,p.b_arbH);if(!b.has_value())return host_error(b.error());
        if(a.value()==b.value())*p.compareP=PF_ArbCompare_EQUAL;return PF_Err_NONE;
    }
    case PF_Arbitrary_PRINT_SIZE_FUNC: {
        auto& p=extra.u.print_size_func_params;if(!p.print_sizePLu)return PF_Err_BAD_CALLBACK_PARAM;*p.print_sizePLu=0;
        auto bytes=validated_bytes(data,p.arbH);if(!bytes.has_value())return host_error(bytes.error());
        *p.print_sizePLu=static_cast<A_u_long>(kTextPrefix.size()+bytes.value().size()*2+1);return PF_Err_NONE;
    }
    case PF_Arbitrary_PRINT_FUNC: {
        auto& p=extra.u.print_func_params;if(!p.print_bufferPC)return PF_Err_BAD_CALLBACK_PARAM;
        auto bytes=validated_bytes(data,p.arbH);if(!bytes.has_value())return host_error(bytes.error());
        if(p.print_sizeLu<kTextPrefix.size()+bytes.value().size()*2+1)return PF_Err_BAD_CALLBACK_PARAM;
        std::memcpy(p.print_bufferPC,kTextPrefix.data(),kTextPrefix.size());auto* out=p.print_bufferPC+kTextPrefix.size();
        for(const auto byte:bytes.value()){const auto n=std::to_integer<unsigned>(byte);*out++=kHex[n>>4];*out++=kHex[n&15];}
        *out='\0';return PF_Err_NONE;
    }
    case PF_Arbitrary_SCAN_FUNC: {
        auto& p=extra.u.scan_func_params;if(!p.arbPH)return PF_Err_BAD_CALLBACK_PARAM;*p.arbPH=nullptr;
        if(!p.bufPC||p.bytes_to_scanLu>kTextPrefix.size()+core::kMaxModelEncodedBytes*2+1)return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;
        std::string_view text(p.bufPC,p.bytes_to_scanLu);if(!text.empty()&&text.back()=='\0')text.remove_suffix(1);
        if(!text.starts_with(kTextPrefix))return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;text.remove_prefix(kTextPrefix.size());
        if(text.size()<kMeshHeaderBytes*2||text.size()%2)return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;
        core::OpaqueBytes bytes(text.size()/2);
        for(std::size_t i=0;i<bytes.size();++i){const auto hi=hex_digit(text[i*2]),lo=hex_digit(text[i*2+1]);
            if(hi<0||lo<0)return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;bytes[i]=static_cast<std::byte>((hi<<4)|lo);}
        const auto error=accept_bytes(data,bytes,p.arbPH);return error==PF_Err_BAD_CALLBACK_PARAM?PF_Err_CANNOT_PARSE_KEYFRAME_TEXT:error;
    }
    default:return PF_Err_BAD_CALLBACK_PARAM;
    }
}
}
core::Result<core::ModelGeometry> read_model_geometry_parameter(PF_InData* data,PF_ArbitraryH handle,
    const core::Cancellation& cancel) noexcept try {
    if(cancel.is_cancelled())return core::Result<core::ModelGeometry>::failure(core::ErrorCode::cancelled,"Model mesh read cancelled");
    auto bytes=copy_handle(data,handle);if(!bytes.has_value())return core::Result<core::ModelGeometry>::failure(bytes.error());
    return core::decode_model_geometry(bytes.value(),cancel);
} catch(const std::bad_alloc&){return core::Result<core::ModelGeometry>::failure(core::ErrorCode::allocation_failed,"Model mesh read allocation failed");}
catch(...){return core::Result<core::ModelGeometry>::failure(core::ErrorCode::invalid_request,"Model mesh handle read failed");}
PF_Err create_model_geometry_parameter(PF_InData* data,const core::ModelGeometry& mesh,PF_ArbitraryH* output,
    const core::Cancellation& cancel) noexcept {
    if(!output)return PF_Err_BAD_CALLBACK_PARAM;*output=nullptr;
    if(!handles_available(data))return PF_Err_BAD_CALLBACK_PARAM;
    auto bytes=core::encode_model_geometry(mesh,cancel);if(!bytes.has_value())return host_error(bytes.error());
    return write_handle(data,bytes.value(),output);
}
PF_Err model_geometry_arbitrary_callback(PF_InData* data,PF_ArbParamsExtra* extra,A_long disk_id) noexcept {
    if(!handles_available(data)||!extra||disk_id<1||disk_id>9999||extra->id!=disk_id)return PF_Err_BAD_CALLBACK_PARAM;
    try{return dispatch(data,*extra);}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}
    catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
}
