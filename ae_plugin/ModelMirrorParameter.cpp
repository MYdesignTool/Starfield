#include "ModelMirrorParameter.hpp"
#include "AE_EffectCB.h"
#include "Param_Utils.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <string_view>

namespace starfield::adapter {
namespace {
constexpr std::size_t kMeshHeaderBytes=64;
constexpr std::string_view kTextPrefix="SFMIRROR1:";
constexpr char kHex[]="0123456789abcdef";
core::NeverCancelled never;
template<class T>core::Result<T> invalid(const char* detail) noexcept {
    return core::Result<T>::failure(core::ErrorCode::invalid_request,detail);
}
core::Result<std::uint32_t> checksum(std::span<const std::byte> bytes,const core::Cancellation& cancel) noexcept {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=0;i<bytes.size();++i){
        if(i%4096==0 && cancel.is_cancelled())return core::Result<std::uint32_t>::failure(core::ErrorCode::cancelled,"Model mirror checksum cancelled");
        crc^=std::to_integer<std::uint8_t>(bytes[i]);
        for(unsigned b=0;b<8;++b)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);
    }
    return core::Result<std::uint32_t>::success(crc^0xffffffffu);
}
std::uint32_t read_u32(std::span<const std::byte> bytes,std::size_t at) noexcept {
    std::uint32_t value{};for(unsigned b=0;b<4;++b)value|=std::to_integer<std::uint32_t>(bytes[at+b])<<(8*b);return value;
}
void write_u32(std::span<std::byte> bytes,std::size_t at,std::uint32_t value) noexcept {
    for(unsigned b=0;b<4;++b)bytes[at+b]=static_cast<std::byte>((value>>(8*b))&255);
}
core::Result<ModelMirrorValue> decode_mirror(std::span<const std::byte> bytes,const core::Cancellation& cancel) {
    using R=core::Result<ModelMirrorValue>;
    if(bytes.size()<kMeshHeaderBytes || bytes.size()>core::kMaxModelEncodedBytes+kMeshHeaderBytes ||
        std::memcmp(bytes.data(),"SFMR",4) || bytes[4]!=std::byte{1} || bytes[5]!=std::byte{0} ||
        bytes[6]!=std::byte{64} || bytes[7]!=std::byte{0} || read_u32(bytes,8)!=bytes.size())
        return invalid<ModelMirrorValue>("invalid Model mirror header");
    if(std::any_of(bytes.begin()+36,bytes.begin()+64,[](auto byte){return byte!=std::byte{0};}))
        return invalid<ModelMirrorValue>("invalid Model mirror reserved bytes");
    const auto crc=checksum(bytes.subspan(16),cancel);if(!crc.has_value())return R::failure(crc.error());
    if(crc.value()!=read_u32(bytes,12))return invalid<ModelMirrorValue>("Model mirror checksum mismatch");
    ModelMirrorValue value;
    for(unsigned i=0;i<16;++i)value.id[i]=std::to_integer<std::uint8_t>(bytes[16+i]);
    value.revision=read_u32(bytes,32);
    if(value.id==core::ModelResourceId{}) {
        if(value.revision || bytes.size()!=kMeshHeaderBytes)return invalid<ModelMirrorValue>("invalid empty Model mirror");
    } else {
        if(!value.revision || value.revision>2147483647u)return invalid<ModelMirrorValue>("invalid Model mirror revision");
        auto mesh=core::decode_model_geometry(bytes.subspan(kMeshHeaderBytes),cancel);
        if(!mesh.has_value())return R::failure(mesh.error());value.geometry=mesh.take_value();
    }
    return R::success(std::move(value));
}
core::Result<core::OpaqueBytes> encode_mirror(const ModelMirrorValue& value,const core::Cancellation& cancel) {
    using R=core::Result<core::OpaqueBytes>;
    if(cancel.is_cancelled())return R::failure(core::ErrorCode::cancelled,"Model mirror encode cancelled");
    core::OpaqueBytes mesh;
    if(value.id==core::ModelResourceId{}) {
        if(value.revision || !value.geometry.positions.empty() || !value.geometry.texture_coordinates.empty() ||
            !value.geometry.normals.empty() || !value.geometry.triangles.empty())return invalid<core::OpaqueBytes>("invalid empty Model mirror value");
    } else {
        if(!value.revision || value.revision>2147483647u)return invalid<core::OpaqueBytes>("invalid Model mirror value revision");
        auto encoded=core::encode_model_geometry(value.geometry,cancel);if(!encoded.has_value())return R::failure(encoded.error());mesh=encoded.take_value();
    }
    core::OpaqueBytes bytes(kMeshHeaderBytes+mesh.size());std::memcpy(bytes.data(),"SFMR",4);
    bytes[4]=std::byte{1};bytes[6]=std::byte{64};write_u32(bytes,8,static_cast<std::uint32_t>(bytes.size()));
    for(unsigned i=0;i<16;++i)bytes[16+i]=static_cast<std::byte>(value.id[i]);write_u32(bytes,32,value.revision);
    std::copy(mesh.begin(),mesh.end(),bytes.begin()+kMeshHeaderBytes);
    const auto crc=checksum(std::span<const std::byte>(bytes).subspan(16),cancel);if(!crc.has_value())return R::failure(crc.error());
    write_u32(bytes,12,crc.value());return R::success(std::move(bytes));
}
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
    if(size<kMeshHeaderBytes||size>(core::kMaxModelEncodedBytes+kMeshHeaderBytes))
        return R::failure(core::ErrorCode::invalid_request,"invalid Model mesh handle size");
    BorrowedMeshBytes borrowed{data,handle,data->utils->host_lock_handle(handle)};
    if(!borrowed.pointer)return R::failure(core::ErrorCode::allocation_failed,"Model mesh handle lock failed");
    const auto* begin=static_cast<const std::byte*>(borrowed.pointer);
    return R::success(core::OpaqueBytes(begin,begin+size));
}
PF_Err write_handle(PF_InData* data,std::span<const std::byte> bytes,PF_ArbitraryH* output) {
    if(!output)return PF_Err_BAD_CALLBACK_PARAM;
    *output=nullptr;
    if(!handles_available(data)||bytes.size()<kMeshHeaderBytes||bytes.size()>(core::kMaxModelEncodedBytes+kMeshHeaderBytes))
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
    auto mesh=decode_mirror(bytes.value(),never);
    if(!mesh.has_value())return core::Result<core::OpaqueBytes>::failure(mesh.error());
    return bytes;
}
PF_Err accept_bytes(PF_InData* data,std::span<const std::byte> bytes,PF_ArbitraryH* output) {
    if(!output)return PF_Err_BAD_CALLBACK_PARAM;
    *output=nullptr;
    auto mesh=decode_mirror(bytes,never);if(!mesh.has_value())return host_error(mesh.error());
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
        return create_model_mirror_parameter(data,ModelMirrorValue{},output,never);
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
        if(!p.flat_dataPV||p.buf_sizeLu<kMeshHeaderBytes||p.buf_sizeLu>(core::kMaxModelEncodedBytes+kMeshHeaderBytes))return PF_Err_BAD_CALLBACK_PARAM;
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
        if(!p.bufPC||p.bytes_to_scanLu>kTextPrefix.size()+(core::kMaxModelEncodedBytes+kMeshHeaderBytes)*2+1)return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;
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
core::Result<ModelMirrorValue> read_model_mirror_parameter(PF_InData* data,PF_ArbitraryH handle,
    const core::Cancellation& cancel) noexcept try {
    if(cancel.is_cancelled())return core::Result<ModelMirrorValue>::failure(core::ErrorCode::cancelled,"Model mirror read cancelled");
    auto bytes=copy_handle(data,handle);if(!bytes.has_value())return core::Result<ModelMirrorValue>::failure(bytes.error());
    return decode_mirror(bytes.value(),cancel);
} catch(const std::bad_alloc&){return core::Result<ModelMirrorValue>::failure(core::ErrorCode::allocation_failed,"Model mirror read allocation failed");}
catch(...){return core::Result<ModelMirrorValue>::failure(core::ErrorCode::invalid_request,"Model mirror handle read failed");}
PF_Err create_model_mirror_parameter(PF_InData* data,const ModelMirrorValue& value,PF_ArbitraryH* output,
    const core::Cancellation& cancel) noexcept {
    if(!output)return PF_Err_BAD_CALLBACK_PARAM;*output=nullptr;
    if(!handles_available(data))return PF_Err_BAD_CALLBACK_PARAM;
    try {auto bytes=encode_mirror(value,cancel);if(!bytes.has_value())return host_error(bytes.error());return write_handle(data,bytes.value(),output);}
    catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
PF_Err model_mirror_arbitrary_callback(PF_InData* data,PF_ArbParamsExtra* extra) noexcept {
    if(!handles_available(data)||!extra||extra->id<kModelMirrorFirstDiskId||extra->id>=kModelMirrorFirstDiskId+kModelMirrorCapacity)return PF_Err_BAD_CALLBACK_PARAM;
    try{return dispatch(data,*extra);}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}
    catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
PF_Err register_model_mirror_parameters(PF_InData* data) noexcept {
    if(!data || !data->inter.add_param)return PF_Err_BAD_CALLBACK_PARAM;
    for(A_long slot=0;slot<kModelMirrorCapacity;++slot) {
        PF_ArbitraryH initial{};auto error=create_model_mirror_parameter(data,{},&initial,never);if(error)return error;
        PF_ParamDef def{};def.param_type=PF_Param_ARBITRARY_DATA;def.flags=PF_ParamFlag_NONE;
        def.ui_flags=PF_PUI_NO_ECW_UI|PF_PUI_INVISIBLE;def.uu.id=def.u.arb_d.id=static_cast<A_short>(kModelMirrorFirstDiskId+slot);
        std::snprintf(def.name,sizeof(def.name),"Model Resource %ld",static_cast<long>(slot+1));def.u.arb_d.dephault=initial;
        error=PF_ADD_PARAM(data,-1,&def);if(error){data->utils->host_dispose_handle(initial);return error;}
    }
    PF_ParamDef def{};def.param_type=PF_Param_SLIDER;def.flags=PF_ParamFlag_CANNOT_TIME_VARY;
    def.ui_flags=PF_PUI_NO_ECW_UI|PF_PUI_INVISIBLE;def.uu.id=kModelMirrorCountDiskId;
    std::snprintf(def.name,sizeof(def.name),"Model Resource Count");def.u.sd.valid_max=def.u.sd.slider_max=kModelMirrorCapacity;
    return PF_ADD_PARAM(data,-1,&def);
}
core::Result<std::vector<ModelMirrorRecipe>> model_mirror_recipes(const core::Graph& graph) noexcept try {
    using R=core::Result<std::vector<ModelMirrorRecipe>>;using namespace core::graph_keys;
    std::vector<ModelMirrorRecipe> recipes;
    for(const auto& node:graph.nodes)if(node.type_key==kModelNode) {
        if(node.schema_version!=1)return invalid<std::vector<ModelMirrorRecipe>>("invalid Model mirror node schema");
        core::ModelResourceId resource{};std::uint32_t source{},revision{};bool seen[4]{};
        const core::OpaqueBytes* encoded_bounds{};
        for(const auto& p:node.parameters) {
            const int field=p.key==kModelResource?0:p.key==kModelRevision?1:p.key==kModelSource?2:p.key==kModelBounds?3:-1;
            if(field<0)continue;
            if(seen[field])return invalid<std::vector<ModelMirrorRecipe>>("duplicate Model mirror metadata");seen[field]=true;
            if(field==0){const auto* bytes=std::get_if<core::OpaqueBytes>(&p.value);
                if(!bytes||bytes->size()!=16)return invalid<std::vector<ModelMirrorRecipe>>("invalid Model mirror identity");
                for(unsigned i=0;i<16;++i)resource[i]=std::to_integer<std::uint8_t>((*bytes)[i]);}
            else if(field==3){encoded_bounds=std::get_if<core::OpaqueBytes>(&p.value);
                if(!encoded_bounds||encoded_bounds->size()!=48)return invalid<std::vector<ModelMirrorRecipe>>("invalid Model mirror bounds");}
            else {const auto* value=std::get_if<std::uint32_t>(&p.value);
                if(!value)return invalid<std::vector<ModelMirrorRecipe>>("invalid Model mirror source/revision kind");
                if(field==1)revision=*value;else source=*value;}
        }
        if(source>1||revision>2147483647u)return invalid<std::vector<ModelMirrorRecipe>>("invalid Model mirror source/revision");
        if(resource==core::ModelResourceId{}) {
            if(revision)return invalid<std::vector<ModelMirrorRecipe>>("Model mirror revision has no resource");continue;
        }
        if(source!=1||!revision||resource!=node.id.value.bytes||!encoded_bounds)
            return invalid<std::vector<ModelMirrorRecipe>>("Model mirror metadata does not match its author");
        ModelMirrorRecipe recipe{node.id,revision,{}};
        double* bounds[]{&recipe.bounds.minimum.x,&recipe.bounds.minimum.y,&recipe.bounds.minimum.z,
            &recipe.bounds.maximum.x,&recipe.bounds.maximum.y,&recipe.bounds.maximum.z};
        for(unsigned i=0;i<6;++i){std::uint64_t bits{};for(unsigned b=0;b<8;++b)bits|=std::to_integer<std::uint64_t>((*encoded_bounds)[8*i+b])<<(8*b);
            *bounds[i]=std::bit_cast<double>(bits);
            if(!std::isfinite(*bounds[i])||std::abs(*bounds[i])>core::kMaxModelCoordinate)
                return invalid<std::vector<ModelMirrorRecipe>>("invalid Model mirror bounds value");}
        if(recipe.bounds.minimum.x>recipe.bounds.maximum.x||recipe.bounds.minimum.y>recipe.bounds.maximum.y||recipe.bounds.minimum.z>recipe.bounds.maximum.z)
            return invalid<std::vector<ModelMirrorRecipe>>("inverted Model mirror bounds");
        recipes.push_back(recipe);
        if(recipes.size()>core::kMaxModelSources)return R::failure(core::ErrorCode::work_limit_exceeded,"Model mirror source budget exceeded");
    }
    std::sort(recipes.begin(),recipes.end(),[](const auto& a,const auto& b){return a.node<b.node;});
    for(std::size_t i=1;i<recipes.size();++i)if(recipes[i-1].node==recipes[i].node)
        return invalid<std::vector<ModelMirrorRecipe>>("duplicate Model mirror author identity");
    return R::success(std::move(recipes));
} catch(const std::bad_alloc&){return core::Result<std::vector<ModelMirrorRecipe>>::failure(core::ErrorCode::allocation_failed,"Model mirror recipe allocation failed");}
catch(...){return core::Result<std::vector<ModelMirrorRecipe>>::failure(core::ErrorCode::invalid_request,"Model mirror recipe failed");}
}
