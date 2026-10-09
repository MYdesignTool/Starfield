#include "AEConfig.h"
#include "NativeNodeGraph.hpp"
#include "NativeTemporalCache.hpp"
#include "GraphParameter.hpp"

#include "AE_EffectCB.h"
#include "AE_GeneralPlug.h"
#include "NodeRecord.hpp"
#include "NodeGraphSync.hpp"
#include "Parameters.hpp"
#include "MotionBlur.hpp"
#include "TransformBinding.hpp"
#include "ModelGeometryParameter.hpp"
#include "ModelMirrorTransaction.hpp"
#include "EffectGraphBackup.hpp"
#include "SPBasic.h"

#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/ColorGradient.hpp"
#include "starfield/core/Geometry.hpp"

#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace starfield::adapter {
namespace {

namespace core = starfield::core;
using native_nodes::Kind;
namespace particle_layout=native_nodes::particle_layout;
namespace transform_layout=native_nodes::transform_layout;
namespace model_layout=native_nodes::model_layout;

constexpr char kRendererMatchName[] = "org.starfieldfx.particle";
constexpr char kEmitterMatchName[] = "org.starfieldfx.node.emitter";
constexpr char kParticleMatchName[] = "org.starfieldfx.node.particle";
constexpr char kForceMatchName[] = "org.starfieldfx.node.force";
constexpr char kTransformMatchName[] = "org.starfieldfx.node.transform";
constexpr char kModelMatchName[] = "org.starfieldfx.node.model";

struct RawField {
    bool present{};
    node_sync::ValueKind type{};
    A_long slot{-1};
    std::array<double, 3> value{};
    std::array<bool, 3> constant{}; // frame-local, never serialized
};
struct RawNode {
    core::NodeId id{};
    Kind kind{};
    std::array<RawField, particle_layout::birth_enabled+1> fields{};
};
constexpr std::uint16_t kBindingRecordTag = 0x8002;
constexpr A_long component_count(node_sync::ValueKind type) noexcept {
    return type == node_sync::ValueKind::scalar ? 1 : type == node_sync::ValueKind::point2 ? 2 : 3;
}
constexpr bool animated_index(Kind kind,A_long index) noexcept {
    if(kind==Kind::particle) return native_nodes::particle_layout::animated(index);
    if(kind==Kind::transform) return transform_layout::animated(index);
    if(kind==Kind::model) return model_layout::animated(index);
    return index>=1 && index<=(kind==Kind::emitter?33:10);
}

struct SuiteSet {
    explicit SuiteSet(PF_InData* data) : data(data), basic(data ? data->pica_basicP : nullptr) {}
    ~SuiteSet() {
        if (!basic) return;
        if (layer) basic->ReleaseSuite(kAEGPLayerSuite, kAEGPLayerSuiteVersion9);
        if (stream) basic->ReleaseSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6);
        if (effect) basic->ReleaseSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4);
        if (pf_interface) basic->ReleaseSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    }
    PF_Err acquire() noexcept {
        if (!basic) return PF_Err_BAD_CALLBACK_PARAM;
        A_Err error = basic->AcquireSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1,
                                          reinterpret_cast<const void**>(&pf_interface));
        if (!error) error = basic->AcquireSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4,
                                                 reinterpret_cast<const void**>(&effect));
        if (!error) error = basic->AcquireSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6,
                                                 reinterpret_cast<const void**>(&stream));
        return static_cast<PF_Err>(error);
    }
    RawNode* recording{};
    const RawNode* previous_node{}; // saved attachment, borrowed for UI compilation
    const RawNode* playback{};
    PF_InData* data{};
    AEGP_LayerH owner_layer{};
    SPBasicSuite* basic{};
    const AEGP_PFInterfaceSuite1* pf_interface{};
    const AEGP_EffectSuite4* effect{};
    const AEGP_StreamSuite6* stream{};
    const AEGP_LayerSuite9* layer{}; // acquired only by UI compilation of a selected resource
    const node_sync::NativeEdit* edit{};
    AEGP_EffectRefH edited_effect{};
    bool edit_applied{};
    bool edit_matched{};
    bool fail(A_long index) const noexcept {
        if (edit) edit->stream_index = index;
        return false;
    }
};

struct EffectRef {
    const AEGP_EffectSuite4* suite{};
    AEGP_EffectRefH value{};
    ~EffectRef() { if (suite && value) suite->AEGP_DisposeEffect(value); }
    EffectRef(const EffectRef&) = delete;
    EffectRef& operator=(const EffectRef&) = delete;
    EffectRef(const AEGP_EffectSuite4* effect, AEGP_EffectRefH ref) : suite(effect), value(ref) {}
};

struct StreamRef {
    const AEGP_StreamSuite6* suite{};
    AEGP_StreamRefH value{};
    ~StreamRef() { if (suite && value) suite->AEGP_DisposeStream(value); }
    StreamRef(const AEGP_StreamSuite6* stream, AEGP_StreamRefH ref) : suite(stream), value(ref) {}
    StreamRef(const StreamRef&) = delete;
    StreamRef& operator=(const StreamRef&) = delete;
};

bool record_value(SuiteSet& suites, A_long index, node_sync::ValueKind type,
                  const std::array<double, 3>& value) noexcept {
    if (suites.recording) {
        if (index < 1 || index >= static_cast<A_long>(suites.recording->fields.size())) return false;
        auto& field = suites.recording->fields[index];
        field.present = true; field.type = type; field.value = value;
    }
    return true;
}
bool playback_value(SuiteSet& suites, A_long index, node_sync::ValueKind type,
                    std::array<double, 3>& value) noexcept {
    if (index < 1 || index >= static_cast<A_long>(suites.playback->fields.size())) return false;
    const auto& field = suites.playback->fields[index];
    if (!field.present || field.type != type) return false;
    value = field.value; return true;
}

bool read_one_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, double& output, bool record = true) noexcept {
    if (suites.playback) {
        std::array<double, 3> sampled{};
        if (!playback_value(suites, index, node_sync::ValueKind::scalar, sampled)) return false;
        output = sampled[0]; return record_value(suites, index, node_sync::ValueKind::scalar, {output, 0.0, 0.0});
    }

    if(suites.edit && effect==suites.edited_effect) for(unsigned i=0;i<suites.edit->additional_count;++i) {
        const auto& field=suites.edit->additional_fields[i];if(field.index!=index)continue;
        if(field.kind!=node_sync::ValueKind::scalar)return suites.fail(index);
        suites.edit_applied=true;output = field.value[0];
        return record_value(suites,index,node_sync::ValueKind::scalar,{output,0.0,0.0});
    }
    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::scalar) return suites.fail(index);
        suites.edit_applied = true; output = suites.edit->value[0]; return record_value(suites, index, node_sync::ValueKind::scalar, {output, 0.0, 0.0});
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, suites.recording ? FALSE : TRUE, &value);
    if (error) return suites.fail(index);
    const double result = value.val.one_d;
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result)) return suites.fail(index);
    output = result;
    return !record || record_value(suites, index, node_sync::ValueKind::scalar, {output, 0.0, 0.0});
}

bool read_layer_resource(SuiteSet& suites, AEGP_PluginID id, AEGP_EffectRefH effect,
                         const A_Time& time, std::uint32_t& output,A_long index=transform_layout::inherit) noexcept {
    if(suites.playback) {
        std::array<double,3> value{};
        if(!playback_value(suites,index,node_sync::ValueKind::scalar,value) ||
           !std::isfinite(value[0]) || value[0]<0 || value[0]>0x7fffffff || std::floor(value[0])!=value[0])return false;
        output=static_cast<std::uint32_t>(value[0]);return true;
    }
    AEGP_StreamRefH raw{};
    if(suites.stream->AEGP_GetNewEffectStreamByIndex(id,effect,index,&raw) || !raw)return suites.fail(index);
    StreamRef stream(suites.stream,raw);AEGP_StreamType type{};
    if(suites.stream->AEGP_GetStreamType(raw,&type) || type!=AEGP_StreamType_LAYER_ID)return suites.fail(index);
    AEGP_StreamValue2 value{};
    if(suites.stream->AEGP_GetNewStreamValue(id,raw,AEGP_LTimeMode_LayerTime,&time,FALSE,&value))return suites.fail(index);
    const auto resource=value.val.layer_id;suites.stream->AEGP_DisposeStreamValue(&value);
    if(resource<0)return suites.fail(index);
    output=static_cast<std::uint32_t>(resource);
    // PF_LayerDef contains the checked-out world, not this project-local ID.
    // The supervised layer edit uses the authoritative UI stream selection.
    if(suites.edit && effect==suites.edited_effect && suites.edit->parameter_index==index)suites.edit_applied=true;
    return record_value(suites,index,node_sync::ValueKind::scalar,{double(output),0,0});
}

bool read_layer_affine(SuiteSet& suites, AEGP_PluginID id, std::uint32_t resource,
                           const core::LayerUnits& units, transform_binding::PixelAffine& pixel) {
    (void)units;
    if(resource==0)pixel={1,0,0,0,0,1,0,0,0,0,1,0};
    else {
        if(!suites.data || !suites.owner_layer || !suites.data->time_scale)return false;
        if(!suites.layer && suites.basic->AcquireSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,
            reinterpret_cast<const void**>(&suites.layer)))return false;
        if(!suites.layer || !suites.layer->AEGP_GetLayerParentComp || !suites.layer->AEGP_GetLayerFromLayerID ||
           !suites.layer->AEGP_GetLayerToWorldXform || !suites.layer->AEGP_ConvertLayerToCompTime ||
           !suites.stream->AEGP_GetNewLayerStream)return false;
        AEGP_CompH comp{};AEGP_LayerH source{};A_Time comp_time{};
        const A_Time layer_time{suites.data->current_time,suites.data->time_scale};
        if(suites.layer->AEGP_GetLayerParentComp(suites.owner_layer,&comp) || !comp ||
           suites.layer->AEGP_GetLayerFromLayerID(comp,static_cast<AEGP_LayerIDVal>(resource),&source) || !source ||
           suites.layer->AEGP_ConvertLayerToCompTime(suites.owner_layer,&layer_time,&comp_time))return false;
        A_Matrix4 source_world{},owner_world{};
        if(suites.layer->AEGP_GetLayerToWorldXform(source,&comp_time,&source_world) ||
           suites.layer->AEGP_GetLayerToWorldXform(suites.owner_layer,&comp_time,&owner_world))return false;
        AEGP_StreamRefH raw{};
        if(suites.stream->AEGP_GetNewLayerStream(id,source,AEGP_LayerStream_ANCHORPOINT,&raw) || !raw)return false;
        StreamRef anchor_stream(suites.stream,raw);AEGP_StreamType type{};
        if(suites.stream->AEGP_GetStreamType(raw,&type) ||
           (type!=AEGP_StreamType_TwoD_SPATIAL && type!=AEGP_StreamType_ThreeD_SPATIAL))return false;
        AEGP_StreamValue2 value{};
        if(suites.stream->AEGP_GetNewStreamValue(id,raw,AEGP_LTimeMode_CompTime,&comp_time,FALSE,&value))return false;
        core::Vec3 anchor=type==AEGP_StreamType_TwoD_SPATIAL ? core::Vec3{value.val.two_d.x,value.val.two_d.y,0} :
            core::Vec3{value.val.three_d.x,value.val.three_d.y,value.val.three_d.z};
        suites.stream->AEGP_DisposeStreamValue(&value);
        transform_binding::Matrix source_matrix{},owner_matrix{};
        // AEGP matrices act on row vectors; the core helper acts on columns.
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c) {
            source_matrix[r*4+c]=source_world.mat[c][r];owner_matrix[r*4+c]=owner_world.mat[c][r];
        }
        auto relative=transform_binding::relative_anchor_affine(source_matrix,owner_matrix,anchor);
        if(!relative.has_value())return false;
        pixel=relative.value();
    }
    return true;
}

bool attachment_fields(const RawNode& node,transform_binding::PixelAffine& offset) noexcept {
    for(A_long i=0;i<12;++i) {
        const auto& field=node.fields[transform_layout::compensation_first+i];
        if(!field.present || field.type!=node_sync::ValueKind::scalar || field.slot>=0 ||
           !std::isfinite(field.value[0]) || std::abs(field.value[0])>1e12)return false;
        offset[i]=field.value[0];
    }
    return true;
}

bool read_inherited_affine(SuiteSet& suites, AEGP_PluginID id, std::uint32_t resource,
                           const core::LayerUnits& units, transform_binding::PixelAffine& pixel) {
    if(suites.playback) {
        for(A_long i=0;i<12;++i)
            if(!read_one_d(suites,id,nullptr,transform_layout::matrix_first+i,{},pixel[i]))return false;
        return true;
    }
    using transform_binding::PixelAffine;
    const PixelAffine identity{1,0,0,0,0,1,0,0,0,0,1,0};
    const PixelAffine centred{1,0,0,units.layer_width*.5,0,1,0,units.layer_height*.5,0,0,1,0};
    PixelAffine current{},offset{},preserved=centred;
    if(!read_layer_affine(suites,id,resource,units,current))return suites.fail(transform_layout::inherit);
    const auto* previous=suites.previous_node;
    if(!resource)offset=centred;
    else if(previous) {
        const auto& source=previous->fields[transform_layout::inherit];
        if(!source.present || source.type!=node_sync::ValueKind::scalar || source.value[0]<0 ||
           source.value[0]>0x7fffffff || std::floor(source.value[0])!=source.value[0])return false;
        const auto old_resource=static_cast<std::uint32_t>(source.value[0]);
        PixelAffine old_offset{};
        const bool calibrated=attachment_fields(*previous,old_offset);
        if(old_resource==resource) {
            // Legacy v1 selected resources retain their absolute behavior until
            // the owner changes the reference. A normal graph edit never binds again.
            offset=calibrated?old_offset:identity;
        } else {
            if(old_resource) {
                PixelAffine old_frame{};
                if(!read_layer_affine(suites,id,old_resource,units,old_frame))return suites.fail(transform_layout::inherit);
                preserved=calibrated?transform_binding::compose_affine(old_frame,old_offset):old_frame;
            } else preserved=centred;
            auto attached=transform_binding::attachment_offset(current,preserved);
            if(!attached.has_value())return suites.fail(transform_layout::inherit);
            offset=attached.value();
        }
    } else {
        auto attached=transform_binding::attachment_offset(current,preserved);
        if(!attached.has_value())return suites.fail(transform_layout::inherit);
        offset=attached.value();
    }
    pixel=resource?transform_binding::compose_affine(current,offset):offset;
    for(A_long i=0;i<12;++i) {
        if(!std::isfinite(offset[i]) || std::abs(offset[i])>1e12 ||
           !record_value(suites,transform_layout::compensation_first+i,node_sync::ValueKind::scalar,{offset[i],0,0}) ||
           !record_value(suites,transform_layout::matrix_first+i,node_sync::ValueKind::scalar,{pixel[i],0,0}))return false;
    }
    return true;
}

bool read_two_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.playback) {
        std::array<double, 3> sampled{};
        if (!playback_value(suites, index, node_sync::ValueKind::point2, sampled)) return false;
        output = {sampled[0], sampled[1], sampled[2]}; return record_value(suites, index, node_sync::ValueKind::point2, {output.x, output.y, output.z});
    }

    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::point2) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], 0.0}; return record_value(suites, index, node_sync::ValueKind::point2, {output.x, output.y, output.z});
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                &time, suites.recording ? FALSE : TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.two_d.x, value.val.two_d.y, 0.0};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y)) return suites.fail(index);
    output = result;
    return record_value(suites, index, node_sync::ValueKind::point2, {output.x, output.y, output.z});
}

bool read_three_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                  A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.playback) {
        std::array<double, 3> sampled{};
        if (!playback_value(suites, index, node_sync::ValueKind::point3, sampled)) return false;
        output = {sampled[0], sampled[1], sampled[2]}; return record_value(suites, index, node_sync::ValueKind::point3, {output.x, output.y, output.z});
    }

    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::point3) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], suites.edit->value[2]}; return record_value(suites, index, node_sync::ValueKind::point3, {output.x, output.y, output.z});
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, suites.recording ? FALSE : TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.three_d.x, value.val.three_d.y, value.val.three_d.z};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) return suites.fail(index);
    output = result;
    return record_value(suites, index, node_sync::ValueKind::point3, {output.x, output.y, output.z});
}

bool read_color(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.playback) {
        std::array<double, 3> sampled{};
        if (!playback_value(suites, index, node_sync::ValueKind::color, sampled)) return false;
        output = {sampled[0], sampled[1], sampled[2]}; return record_value(suites, index, node_sync::ValueKind::color, {output.x, output.y, output.z});
    }

    if(suites.edit && effect==suites.edited_effect) for(unsigned i=0;i<suites.edit->additional_count;++i) {
        const auto& field=suites.edit->additional_fields[i];if(field.index!=index)continue;
        if(field.kind!=node_sync::ValueKind::color)return suites.fail(index);
        suites.edit_applied=true;output = {field.value[0],field.value[1],field.value[2]};
        return record_value(suites,index,node_sync::ValueKind::color,{output.x,output.y,output.z});
    }
    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::color) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], suites.edit->value[2]}; return record_value(suites, index, node_sync::ValueKind::color, {output.x, output.y, output.z});
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, suites.recording ? FALSE : TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.color.redF, value.val.color.greenF, value.val.color.blueF};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) return suites.fail(index);
    output = result;
    return record_value(suites, index, node_sync::ValueKind::color, {output.x, output.y, output.z});
}

bool read_uint(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
               A_long index, const A_Time& time, std::uint32_t& output) noexcept {
    double value = 0.0;
    if (!read_one_d(suites, plugin_id, effect, index, time, value) || value < 0.0 ||
        value > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) return suites.fail(index);
    if ((suites.playback && animated_index(suites.playback->kind,index)) ||
        (suites.recording && animated_index(suites.recording->kind,index))) value = std::round(value);
    if (std::floor(value) != value) return suites.fail(index);
    output = static_cast<std::uint32_t>(value);
    return true;
}

bool read_uuid(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
               A_long first_index, const A_Time& time, core::Uuid128& output) noexcept {
    core::Uuid128 id{};
    bool any = false;
    for (A_long chunk = 0; chunk < 8; ++chunk) {
        std::uint32_t value = 0;
        if (!read_uint(suites, plugin_id, effect, first_index + chunk, time, value) || value > 65535u) return false;
        id.bytes[static_cast<std::size_t>(chunk) * 2] = static_cast<std::uint8_t>(value >> 8u);
        id.bytes[static_cast<std::size_t>(chunk) * 2 + 1] = static_cast<std::uint8_t>(value & 0xffu);
        any = any || value != 0;
    }
    if (!any) return false;
    output = id;
    return true;
}

bool read_curve(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long count_index, A_long first_point_index, const A_Time& time,
                core::OpaqueBytes& bytes, bool& has_curve,double minimum=0.0,double maximum=100.0) {
    std::uint32_t count = 0;
    if (!read_uint(suites, plugin_id, effect, count_index, time, count) || count > core::kMaxAgeCurvePoints ||
        (count != 0 && count < 2)) return false;
    has_curve = count != 0;
    bytes.clear();
    if (count == 0) return true;
    core::AgeCurve curve{};
    curve.count = static_cast<std::uint8_t>(count);
    std::uint32_t interpolation{};
    if(!read_uint(suites,plugin_id,effect,particle_layout::curve_interpolation(count_index),time,interpolation) || interpolation>3)return false;
    curve.interpolation=static_cast<core::CurveInterpolation>(interpolation);
    for (std::uint32_t point = 0; point < count; ++point) {
        if (!read_one_d(suites, plugin_id, effect, first_point_index + static_cast<A_long>(point * 2), time,
                        curve.points[point].age) ||
            !read_one_d(suites, plugin_id, effect, first_point_index + static_cast<A_long>(point * 2 + 1), time,
                        curve.points[point].value)) return false;
    }
    if (!core::valid_age_curve(curve, minimum, maximum)) return false;
    bytes = core::encode_age_curve(curve);
    return !bytes.empty();
}

void add_value(core::GraphNode& node, core::ParameterKey key, core::ParameterValue value) {
    node.parameters.push_back(core::NodeParameter{key, std::move(value)});
}

struct Connection {
    core::NodeId source{};
    core::EdgeId edge{};
    core::NodeId destination{};
    Kind source_kind{Kind::emitter};
};

struct LayoutEntry {
    core::NodeId node{};
    double x{};
    double y{};
};

void append_u16(core::OpaqueBytes& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xffu));
    bytes.push_back(static_cast<std::byte>((value >> 8u) & 0xffu));
}

void append_u32(core::OpaqueBytes& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}

void append_u64(core::OpaqueBytes& bytes, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8)
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}

core::OpaqueBytes make_layout_record(std::vector<LayoutEntry>& entries) {
    std::sort(entries.begin(), entries.end(), [](const LayoutEntry& left, const LayoutEntry& right) {
        return left.node < right.node;
    });
    core::OpaqueBytes bytes;
    bytes.reserve(12u + entries.size() * 32u);
    append_u16(bytes, 0x8001u);
    append_u16(bytes, 1u);
    append_u32(bytes, static_cast<std::uint32_t>(12u + entries.size() * 32u));
    append_u32(bytes, static_cast<std::uint32_t>(entries.size()));
    for (const auto& entry : entries) {
        for (const auto byte : entry.node.value.bytes) bytes.push_back(static_cast<std::byte>(byte));
        append_u64(bytes, std::bit_cast<std::uint64_t>(entry.x));
        append_u64(bytes, std::bit_cast<std::uint64_t>(entry.y));
    }
    return bytes;
}

core::OpaqueBytes make_binding_record(std::vector<RawNode>& nodes) {
    std::sort(nodes.begin(), nodes.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    core::OpaqueBytes bytes;
    const bool attachments=std::any_of(nodes.begin(),nodes.end(),[](const auto& node){return node.kind==Kind::transform;});
    const bool births=std::any_of(nodes.begin(),nodes.end(),[](const auto& node){return node.kind==Kind::particle;});
    const bool models=std::any_of(nodes.begin(),nodes.end(),[](const auto& node){return node.kind==Kind::model;});
    append_u16(bytes, kBindingRecordTag); append_u16(bytes, models?7:births?6:attachments?2:1); append_u32(bytes, 0);
    append_u32(bytes, static_cast<std::uint32_t>(nodes.size()));
    A_long slot = 0;
    for (auto& node : nodes) {
        const auto field_limit = native_nodes::binding_field_count(node.kind);
        if (field_limit < 1 || static_cast<std::size_t>(field_limit) >= node.fields.size() ||
            node.fields[0].present) return {};
        for (auto b : node.id.value.bytes) bytes.push_back(static_cast<std::byte>(b));
        append_u16(bytes, static_cast<std::uint16_t>(node.kind));
        std::uint16_t count = 0;
        for (A_long index = 1; index < static_cast<A_long>(node.fields.size()); ++index) {
            if (!node.fields[index].present) continue;
            if (index > field_limit) return {};
            ++count;
        }
        append_u16(bytes, count);
        for (A_long index = 1; index <= field_limit; ++index) {
            auto& field = node.fields[index]; if (!field.present) continue;
            if (animated_index(node.kind,index)) {
                field.slot = slot; slot += component_count(field.type);
                if (slot > kNativeBindingCapacity) return {};
            }
            append_u16(bytes, static_cast<std::uint16_t>(index));
            append_u16(bytes, static_cast<std::uint16_t>(field.type));
            append_u16(bytes, field.slot < 0 ? 0xffffu : static_cast<std::uint16_t>(field.slot));
            append_u16(bytes, 0);
            for (auto value : field.value) append_u64(bytes, std::bit_cast<std::uint64_t>(value));
        }
    }
    for (unsigned i = 0; i < 4; ++i) bytes[4 + i] = static_cast<std::byte>((bytes.size() >> (i * 8)) & 0xffu);
    return bytes;
}

bool read_binding_record(const core::Graph& graph, std::vector<RawNode>& nodes) {
    bool found = false;
    std::array<bool, kNativeBindingCapacity> used{};
    for (const auto& bytes : graph.optional_records) {
        if (bytes.size() < 2 || bytes[0] != std::byte{2} || bytes[1] != std::byte{0x80}) continue;
        if (found || bytes.size() < 12) return false;
        found = true; std::size_t at = 0;
        auto read = [&](unsigned count, std::uint64_t& out) {
            if (count > bytes.size() - at) return false;
            out = 0;
            for (unsigned i = 0; i < count; ++i) out |= std::uint64_t(std::to_integer<unsigned char>(bytes[at++])) << (8 * i);
            return true;
        };
        std::uint64_t tag{}, version{}, length{}, count{};
        if (!read(2, tag) || !read(2, version) || (version < 1 || version > 7) || !read(4, length) ||
            length != bytes.size() || !read(4, count) || count >= core::kMaxGraphNodes) return false;
        for (std::uint64_t n = 0; n < count; ++n) {
            RawNode node;
            std::uint64_t value{};
            for (auto& byte : node.id.value.bytes) { if (!read(1, value)) return false; byte = static_cast<std::uint8_t>(value); }
            std::uint64_t kind{}, fields{};
            if (!read(2, kind) || (kind > 5 || kind == 2 || (kind==5 && version<7)) || !read(2, fields)) return false;
            node.kind = static_cast<Kind>(kind);
            // Dense Draw banks have 64 age/value pairs. Their constant fields
            // belong in the record but do not consume animation alias slots.
            // Keep writer/reader bounds tied to the current per-kind layout,
            // rather than the retired Particle81 control count.
            const auto field_limit = node.kind==Kind::transform && version==1 ?
                transform_layout::matrix_last : node.kind==Kind::particle && version<3 ?
                particle_layout::last : node.kind==Kind::particle && version==3 ?
                particle_layout::transfer : node.kind==Kind::particle && version==4 ?
                particle_layout::texture_perspective : node.kind==Kind::particle && version==5 ?
                particle_layout::cloud_enabled : native_nodes::binding_field_count(node.kind);
            if (fields > static_cast<std::uint64_t>(field_limit)) return false;
            if (std::any_of(nodes.begin(), nodes.end(), [&](const auto& old) {return old.id == node.id;})) return false;
            for (std::uint64_t f = 0; f < fields; ++f) {
                std::uint64_t index{}, type{}, slot{}, reserved{};
                if (!read(2, index) || index < 1 || index > static_cast<std::uint64_t>(field_limit) ||
                    index >= node.fields.size() || node.fields[index].present ||
                    !read(2, type) || type > 3 || !read(2, slot) || !read(2, reserved) || reserved != 0) return false;
                auto& field = node.fields[index]; field.present = true;
                if(node.kind==Kind::particle && !native_nodes::authored_parameter(node.kind,static_cast<A_long>(index)))return false;
                if(node.kind==Kind::particle && index>=particle_layout::transfer && type!=0)return false;
                field.type = static_cast<node_sync::ValueKind>(type);
                if(node.kind==Kind::transform && field.type!=(index==transform_layout::anchor_xy ?
                    node_sync::ValueKind::point2 : node_sync::ValueKind::scalar))return false;
                if(node.kind==Kind::model && (!model_layout::binding_field(static_cast<A_long>(index)) ||
                    field.type!=node_sync::ValueKind::scalar))return false;
                field.slot = slot == 0xffffu ? -1 : static_cast<A_long>(slot);
                if (field.slot >= 0) {
                    if (!animated_index(node.kind,static_cast<A_long>(index)) ||
                        field.slot + component_count(field.type) > kNativeBindingCapacity) return false;
                    for (A_long c = 0; c < component_count(field.type); ++c) {
                        if (used[field.slot + c]) return false;
                        used[field.slot + c] = true;
                    }
                } else if (animated_index(node.kind,static_cast<A_long>(index))) return false;
                for (auto& scalar : field.value) {
                    if (!read(8, value)) return false;
                    scalar = std::bit_cast<double>(value);
                    if (!std::isfinite(scalar)) return false;
                }
            }
            if(node.kind==Kind::transform && version>=2) {
                transform_binding::PixelAffine offset{};
                if(!attachment_fields(node,offset))return false;
            }
            if(node.kind==Kind::model) {
                for(A_long index=1;index<=model_layout::bounds_last;++index)
                    if(model_layout::binding_field(index) && (!node.fields[index].present ||
                        node.fields[index].value[1]!=0 || node.fields[index].value[2]!=0))return false;
            }
            nodes.push_back(std::move(node));
        }
        if (at != bytes.size()) return false;
    }
    return true; // No binding record on bootstrap graphs.
}

std::u16string binding_expression(const RawNode& node, A_long index, A_long component) {
    // PropertyGroup and Effect are host objects; use documented methods.
    std::string text = "var count = thisLayer(\"ADBE Effect Parade\").numProperties; var result = -1099511627776;\n";
    text += "var owner = thisProperty.propertyGroup(1).propertyIndex;\n";
    text += "for (var n = 1; n <= count; n++) { if (n === owner) continue; var fx = thisLayer.effect(n); var match = false; try { match = ";
    const auto first = native_nodes::uuid_first_index(node.kind);
    // Other effects can have animated inputs at these same numeric indices.
    // In particular, the renderer's 98..609 aliases overlap node UUID indices.
    // Check the metadata property name before reading ANY numeric value, so a
    // search never evaluates our own bindings (or another renderer's bindings).
    // Display names of effects remain irrelevant; all eight UUID words follow.
    text += "fx.param(" + std::to_string(first) + ").name === \"Node UUID 0\" && ";
    for (A_long chunk = 0; chunk < 8; ++chunk) {
        if (chunk) text += " && ";
        const unsigned word = (node.id.value.bytes[chunk * 2] << 8u) | node.id.value.bytes[chunk * 2 + 1];
        text += "fx.param(" + std::to_string(first + chunk) + ").value === " + std::to_string(word);
    }
    text += "; } catch (unrelatedEffect) {} if (match) { ";
    if(node.kind==Kind::transform && transform_layout::matrix_field(index)) {
        transform_binding::PixelAffine offset{};
        const bool calibrated=attachment_fields(node,offset);
        text+=transform_binding::matrix_expression(static_cast<unsigned>(index-transform_layout::matrix_first),
            node.fields[transform_layout::inherit].value[0] != 0,calibrated?&offset:nullptr);
    }
    else {
        text += "result = fx.param(" + std::to_string(index) + ").value";
        if (node.fields[index].type != node_sync::ValueKind::scalar) text += "[" + std::to_string(component) + "]";
        text += "; ";
    }
    text += "break; } }\nresult;";
    return std::u16string(text.begin(), text.end());
}

bool decode_node_kind(const char* match_name, Kind& kind, const char*& type_key, std::uint16_t& schema) noexcept {
    if (std::strcmp(match_name, kEmitterMatchName) == 0) {
        kind = Kind::emitter; type_key = core::graph_keys::kEmitterNode; schema = 7; return true;
    }
    if (std::strcmp(match_name, kParticleMatchName) == 0) {
        kind = Kind::particle; type_key = core::graph_keys::kParticleNode; schema = 7; return true;
    }
    if (std::strcmp(match_name, kForceMatchName) == 0) {
        kind = Kind::force; type_key = core::graph_keys::kForceNode; schema = 3; return true;
    }
    if (std::strcmp(match_name, kTransformMatchName) == 0) {
        kind = Kind::transform; type_key = core::graph_keys::kTransformNode; schema = 1; return true;
    }
    if (std::strcmp(match_name,kModelMatchName)==0) {
        kind=Kind::model;type_key=core::graph_keys::kModelNode;schema=1;return true;
    }
    return false;
}

bool read_node_parameters(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                          Kind kind, const A_Time& time, const core::LayerUnits& units, core::GraphNode& node) {
    using namespace core::graph_keys;
    double scalar = 0.0;
    std::uint32_t integer = 0;
    core::Vec3 vector{};
    if(kind==Kind::model) {
        std::uint32_t source{},revision{};
        if(!read_uint(suites,plugin_id,effect,model_layout::source,time,source) || source<1 || source>2)
            return suites.fail(model_layout::source);
        if(!read_uint(suites,plugin_id,effect,model_layout::revision,time,revision) || revision>2147483647u)
            return suites.fail(model_layout::revision);
        core::ModelLocalSettings pose;
        const auto read_vector=[&](A_long first,core::Vec3& value) {
            return read_one_d(suites,plugin_id,effect,first,time,value.x) &&
                read_one_d(suites,plugin_id,effect,first+1,time,value.y) &&
                read_one_d(suites,plugin_id,effect,first+2,time,value.z);
        };
        if(!read_vector(model_layout::origin,pose.origin) || !read_vector(model_layout::rotation,pose.rotation_degrees) ||
            !read_vector(model_layout::scale,pose.scale_percent))return false;
        bool* flags[]{&pose.flip_x,&pose.flip_y,&pose.flip_z,&pose.center,&pose.normalize};
        for(A_long b=0;b<5;++b) {
            if(!read_uint(suites,plugin_id,effect,model_layout::flip_x+b,time,integer) || integer>1)
                return suites.fail(model_layout::flip_x+b);
            *flags[b]=integer!=0;
        }
        const bool imported=source==2 && revision!=0;
        core::ModelBounds bounds{{-.5,-.5,-.5},{.5,.5,.5}};
        double* fields[]{&bounds.minimum.x,&bounds.minimum.y,&bounds.minimum.z,
            &bounds.maximum.x,&bounds.maximum.y,&bounds.maximum.z};
        if(suites.playback) {
            for(A_long b=0;b<6;++b)if(!read_one_d(suites,plugin_id,effect,model_layout::bounds_first+b,time,*fields[b]))return false;
        } else {
            if(imported) {
                AEGP_StreamRefH raw{};
                if(suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id,effect,model_layout::mesh,&raw) || !raw)
                    return suites.fail(model_layout::mesh);
                StreamRef stream(suites.stream,raw);AEGP_StreamType type{};
                if(!suites.stream->AEGP_GetStreamType || suites.stream->AEGP_GetStreamType(raw,&type) || type!=AEGP_StreamType_ARB)
                    return suites.fail(model_layout::mesh);
                AEGP_StreamValue2 value{};
                if(suites.stream->AEGP_GetNewStreamValue(plugin_id,raw,AEGP_LTimeMode_LayerTime,&time,TRUE,&value))
                    return suites.fail(model_layout::mesh);
                core::NeverCancelled never;
                auto mesh=read_model_geometry_parameter(suites.data,reinterpret_cast<PF_ArbitraryH>(value.val.arbH),never);
                const auto released=suites.stream->AEGP_DisposeStreamValue(&value);
                if(!mesh.has_value() || released)return suites.fail(model_layout::mesh);
                bounds=mesh.value().bounds;
                for(A_long b=0;b<6;++b){double stored{};
                    // Author bounds are physical constant controls, not binding fields.
                    // Only the synthetic bounds19..24 below enter the playback record.
                    if(!read_one_d(suites,plugin_id,effect,model_layout::author_bounds_first+b,time,stored,false)||stored!=*fields[b])
                        return suites.fail(model_layout::author_bounds_first+b);}
            }
            for(A_long b=0;b<6;++b)
                if(!record_value(suites,model_layout::bounds_first+b,node_sync::ValueKind::scalar,{*fields[b],0,0}))return false;
        }
        auto matrix=core::model_local_matrix(pose,bounds);if(!matrix.has_value())return suites.fail(model_layout::origin);
        core::OpaqueBytes resource(16),encoded_bounds;
        if(imported)for(unsigned b=0;b<16;++b)resource[b]=static_cast<std::byte>(node.id.value.bytes[b]);
        for(const auto* field:fields)append_u64(encoded_bounds,std::bit_cast<std::uint64_t>(*field));
        add_value(node,kModelResource,std::move(resource));add_value(node,kModelRevision,imported?revision:std::uint32_t{0});
        add_value(node,kModelOrigin,pose.origin);add_value(node,kModelRotation,pose.rotation_degrees);add_value(node,kModelScale,pose.scale_percent);
        constexpr core::ParameterKey keys[]{kModelFlipX,kModelFlipY,kModelFlipZ,kModelCenter,kModelNormalize};
        for(unsigned b=0;b<5;++b)add_value(node,keys[b],std::uint32_t(*flags[b]));
        add_value(node,kModelBounds,std::move(encoded_bounds));add_value(node,kModelSource,source-1);
        return true;
    }
    if (kind == Kind::emitter) {
        if (!read_one_d(suites, plugin_id, effect, 3, time, scalar)) return false;
        add_value(node, kBirthRate, scalar);
        if (!read_uint(suites, plugin_id, effect, 27, time, integer)) return false;
        add_value(node, kSeed, integer);
        if (!read_uint(suites, plugin_id, effect, 1, time, integer) || integer < 1 || integer > 4) return false;
        add_value(node, kEmitterShape, integer - 1u);
        if (!read_two_d(suites, plugin_id, effect, 4, time, vector) ||
            !read_one_d(suites, plugin_id, effect, 5, time, vector.z)) return false;
        add_value(node, kEmitterOrigin, core::layer_point_to_world(vector.x, vector.y, vector.z + units.layer_height / 2.0, units));
        if (!read_one_d(suites, plugin_id, effect, 30, time, vector.x) ||
            !read_one_d(suites, plugin_id, effect, 31, time, vector.y) ||
            !read_one_d(suites, plugin_id, effect, 32, time, vector.z)) return false;
        add_value(node, kVelocity, vector);
        if (!read_one_d(suites, plugin_id, effect, 28, time, scalar)) return false;
        add_value(node, kParticleSize, scalar);
        if (!read_one_d(suites, plugin_id, effect, 29, time, scalar)) return false;
        add_value(node, kOpacity, scalar / 100.0);
        if (!read_one_d(suites, plugin_id, effect, 11, time, scalar)) return false;
        add_value(node, kEmitterSize, scalar);
        if (!read_one_d(suites, plugin_id, effect, 33, time, scalar)) return false;
        add_value(node, kVelocitySpread, scalar);
        if (!read_one_d(suites, plugin_id, effect, 6, time, scalar)) return false;
        const double emission_speed = scalar / units.layer_height;
        add_value(node, kEmissionSpeed, emission_speed);
        if (!read_one_d(suites, plugin_id, effect, 7, time, scalar)) return false;
        add_value(node, kEmissionSpeedRandomPercent, scalar);
        constexpr std::array<core::ParameterKey, 3> angle_keys{kEmissionAngleX, kEmissionAngleY, kEmissionAngleZ};
        for (A_long axis = 0; axis < 3; ++axis) {
            if (!read_one_d(suites, plugin_id, effect, 12 + axis, time, scalar)) return false;
            add_value(node, angle_keys[static_cast<std::size_t>(axis)], scalar);
        }
        if (!read_uint(suites, plugin_id, effect, 15, time, integer) || integer < 1 || integer > 2) return false;
        add_value(node, kDirectionMode, integer - 1u);
        if (!read_one_d(suites, plugin_id, effect, 16, time, vector.x) ||
            !read_one_d(suites, plugin_id, effect, 17, time, vector.y) ||
            !read_one_d(suites, plugin_id, effect, 18, time, vector.z)) return false;
        add_value(node,kEmitterOrient,vector);
        if (!read_one_d(suites, plugin_id, effect, 19, time, scalar)) return false;
        add_value(node, kDirectionSpan, scalar);
        constexpr std::array<core::ParameterKey, 3> size_keys{kEmitterSizeX, kEmitterSizeY, kEmitterSizeZ};
        for (A_long axis = 0; axis < 3; ++axis) {
            if (!read_one_d(suites, plugin_id, effect, 8 + axis, time, scalar)) return false;
            add_value(node, size_keys[static_cast<std::size_t>(axis)], scalar);
        }
        if (!read_uint(suites, plugin_id, effect, 2, time, integer) || integer < 1 || integer > 4) return false;
        add_value(node, kEmittingMode, integer - 1u);
        if(!read_uint(suites,plugin_id,effect,34,time,integer) || integer>1) return false;
        add_value(node,kAuxiliarySource,integer);
        constexpr std::array<core::ParameterKey, 7> auxiliary_keys{kEmitChance, kEmitLifeStart, kEmitLifeEnd,
            kInheritVelocity, kInheritSize, kInheritOpacity, kInheritColor};
        for (A_long field = 0; field < 7; ++field) {
            if (!read_one_d(suites, plugin_id, effect, 20 + field, time, scalar)) return false;
            add_value(node, auxiliary_keys[field], scalar);
        }
        return true;
    }
    if (kind == Kind::force) {
        if (!read_one_d(suites, plugin_id, effect, 1, time, scalar)) return false;
        add_value(node, kGravity, core::Vec3{0, -scalar / units.layer_height, 0});
        if (!read_one_d(suites, plugin_id, effect, 2, time, scalar)) return false;
        add_value(node, kGravityRandom, scalar);
        if (!read_one_d(suites, plugin_id, effect, 3, time, vector.x) ||
            !read_one_d(suites, plugin_id, effect, 4, time, vector.y) ||
            !read_one_d(suites, plugin_id, effect, 5, time, vector.z)) return false;
        add_value(node, kWind, core::Vec3{vector.x * units.pixel_aspect_ratio / units.layer_height, -vector.y / units.layer_height, vector.z / units.layer_height});
        constexpr std::array<core::ParameterKey, 4> spin_keys{kSpin, kSpinFrequency, kSpinResist, kSpinDelay};
        for (A_long field = 0; field < 4; ++field) {
            if (!read_one_d(suites, plugin_id, effect, 6 + field, time, scalar)) return false;
            add_value(node, spin_keys[field], field == 0 ? scalar / units.layer_height : scalar);
        }
        if (!read_one_d(suites, plugin_id, effect, 10, time, scalar)) return false;
        add_value(node, kLinearDrag, scalar);
        core::OpaqueBytes curve; bool present = false;
        if (!read_curve(suites, plugin_id, effect, 11, 12, time, curve, present)) return false;
        if (present) add_value(node, kWindSpinCurve, std::move(curve));
        return true;
    }

    if(kind==Kind::transform) {
        if(!read_layer_resource(suites,plugin_id,effect,time,integer))return false;
        add_value(node,kTransformInheritLayer,integer);
        transform_binding::PixelAffine pixel{};
        if(!read_inherited_affine(suites,plugin_id,integer,units,pixel))return false;
        auto matrix=transform_binding::canonical_matrix(pixel,units);
        if(!matrix.has_value())return false;
        core::OpaqueBytes bytes{std::byte{1},std::byte{0},std::byte{0},std::byte{0}};
        for(double v:matrix.value())append_u64(bytes,std::bit_cast<std::uint64_t>(v));
        add_value(node,kTransformInheritedMatrix,std::move(bytes));
        if(!read_two_d(suites,plugin_id,effect,transform_layout::anchor_xy,time,vector) ||
           !read_one_d(suites,plugin_id,effect,transform_layout::anchor_z,time,vector.z))return false;
        add_value(node,kTransformAnchor,core::layer_point_to_world(vector.x,vector.y,vector.z+units.layer_height*.5,units));
        const auto read_vector=[&](A_long first) {
            return read_one_d(suites,plugin_id,effect,first,time,vector.x) &&
                read_one_d(suites,plugin_id,effect,first+1,time,vector.y) &&
                read_one_d(suites,plugin_id,effect,first+2,time,vector.z);
        };
        if(!read_vector(transform_layout::position))return false;
        add_value(node,kTransformPosition,core::Vec3{vector.x*units.pixel_aspect_ratio/units.layer_height,
            -vector.y/units.layer_height,vector.z/units.layer_height});
        if(!read_vector(transform_layout::rotation))return false;
        add_value(node,kTransformRotation,core::Vec3{-vector.x,vector.y,-vector.z});
        if(!read_vector(transform_layout::scale))return false;
        add_value(node,kTransformSystemScale,vector);
        if(!read_one_d(suites,plugin_id,effect,transform_layout::particles_scale,time,scalar))return false;
        add_value(node,kTransformParticleScale,scalar);
        if(!read_one_d(suites,plugin_id,effect,transform_layout::particles_opacity,time,scalar))return false;
        add_value(node,kTransformParticleOpacity,scalar);
        return true;
    }
    if(kind!=Kind::particle)return false;
    const A_long color_start_index = particle_layout::color;
    const A_long size_index = particle_layout::size;
    const A_long size_end_index = particle_layout::size_over_life;
    const A_long opacity_index = particle_layout::opacity;
    const A_long opacity_end_index = particle_layout::opacity_over_life;
    const A_long size_random_index = particle_layout::size_random;
    const A_long opacity_random_index = particle_layout::opacity_random;
    if (!read_color(suites, plugin_id, effect, color_start_index, time, vector)) return false;
    add_value(node, kColorStart, vector);
    {
        std::uint32_t mode{},count{};
        if(!read_uint(suites,plugin_id,effect,particle_layout::color_mode,time,mode))return false;
        if(mode<1 || mode>4)return suites.fail(particle_layout::color_mode);
        if(!read_uint(suites,plugin_id,effect,particle_layout::gradient,time,count))return false;
        if(count<2 || count>8)return suites.fail(particle_layout::gradient);
        add_value(node,kParticleColorMode,mode-1);
        core::ColorGradient gradient;gradient.count=static_cast<std::uint8_t>(count);
        std::uint32_t interpolation{};
        if(!read_uint(suites,plugin_id,effect,particle_layout::gradient_interpolation,time,interpolation) || interpolation>1)return false;
        gradient.interpolation=static_cast<core::ColorInterpolation>(interpolation);
        for(std::uint32_t i=0;i<count;++i) {
            if(!read_one_d(suites,plugin_id,effect,particle_layout::gradient_first+2*i,time,gradient.stops[i].position) ||
                !read_color(suites,plugin_id,effect,particle_layout::gradient_first+2*i+1,time,gradient.stops[i].color)) return false;
            gradient.stops[i].position/=100;
            if(gradient.stops[i].position<0 || gradient.stops[i].position>1 ||
               (i && gradient.stops[i].position<=gradient.stops[i-1].position) ||
               gradient.stops[i].position<0 || gradient.stops[i].position>1)
                return suites.fail(particle_layout::gradient_first+2*i);
        }
        auto bytes=core::encode_color_gradient(gradient);if(bytes.empty())return suites.fail(particle_layout::gradient);
        add_value(node,kColorGradient,std::move(bytes));
    }
    if (!read_one_d(suites, plugin_id, effect, size_index, time, scalar)) return false;
    add_value(node, kSizeStart, scalar);
    if (!read_one_d(suites, plugin_id, effect, size_end_index, time, scalar)) return false;
    add_value(node, kSizeEnd, scalar);
    if (!read_one_d(suites, plugin_id, effect, opacity_index, time, scalar)) return false;
    add_value(node, kOpacityStart, scalar / 100.0);
    if (!read_one_d(suites, plugin_id, effect, opacity_end_index, time, scalar)) return false;
    add_value(node, kOpacityEnd, scalar);
    if (!read_one_d(suites, plugin_id, effect, size_random_index, time, scalar)) return false;
    add_value(node, kSizeRandom, scalar);
    if (!read_one_d(suites, plugin_id, effect, opacity_random_index, time, scalar)) return false;
    add_value(node, kOpacityRandom, scalar);
    {
        if (!read_one_d(suites, plugin_id, effect, 2, time, scalar)) return false;
        add_value(node, kParticleLifetimeSeconds, scalar);
        for(auto [index,key]:{std::pair{particle_layout::life_random,kLifeRandom},std::pair{particle_layout::size_y,kSizeY},std::pair{particle_layout::feather,kParticleFeather},
                            std::pair{particle_layout::angle_random,kAngleRandom},std::pair{particle_layout::speed_random,kRotationSpeedRandom},
                            std::pair{particle_layout::limit_angle,kLimitAngle},std::pair{particle_layout::anchor_x,kAnchorX},std::pair{particle_layout::anchor_y,kAnchorY}}) {
            if(!read_one_d(suites,plugin_id,effect,index,time,scalar)) return false;
            add_value(node,key,scalar);
        }
        for(auto [index,key]:{std::pair{particle_layout::shape,kParticleShape},std::pair{particle_layout::up_axis,kUpAxis},
                            std::pair{particle_layout::orient,kOrientTo},std::pair{particle_layout::random_limit,kRandomLimit}}) {
            if(!read_uint(suites,plugin_id,effect,index,time,integer) || integer<1) return false;
            add_value(node,key,integer-1);
        }
        integer=1;
        // Saved v1/v2 binding records predate Transfer Mode and retain Normal.
        if(!suites.playback || suites.playback->fields[particle_layout::transfer].present) {
            if(!read_uint(suites,plugin_id,effect,particle_layout::transfer,time,integer) || integer<1 || integer>4)
                return suites.fail(particle_layout::transfer);
        }
        add_value(node,kParticleTransferMode,integer-1);
        for(auto [index,key]:{std::pair{particle_layout::texture_front,kTextureFront},
                             std::pair{particle_layout::texture_back,kTextureBack}}) {
            integer=0;
            if(!suites.playback || suites.playback->fields[index].present)
                if(!read_layer_resource(suites,plugin_id,effect,time,integer,index))return false;
            add_value(node,key,integer);
        }
        for(auto [index,key]:{std::pair{particle_layout::texture_time,kTextureTimeMode},
                             std::pair{particle_layout::texture_color,kTextureColorUse},
                             std::pair{particle_layout::texture_ratio,kTextureUseRatio},
                             std::pair{particle_layout::texture_perspective,kTextureIgnorePerspective}}) {
            const bool popup=index==particle_layout::texture_time || index==particle_layout::texture_color;
            integer=popup || index==particle_layout::texture_ratio?1:0;
            if(!suites.playback || suites.playback->fields[index].present)
                if(!read_uint(suites,plugin_id,effect,index,time,integer))return false;
            if((popup && (integer<1 || integer>(index==particle_layout::texture_time?8u:3u))) || (!popup && integer>1))
                return suites.fail(index);
            add_value(node,key,popup?integer-1:integer);
        }
        integer=0;
        if(!suites.playback || suites.playback->fields[particle_layout::cloud_enabled].present)
            if(!read_uint(suites,plugin_id,effect,particle_layout::cloud_enabled,time,integer) || integer>1)
                return suites.fail(particle_layout::cloud_enabled);
        if(integer) {
            std::uint32_t circles{};double aspect{},density{};
            if(!read_uint(suites,plugin_id,effect,particle_layout::cloud_circles,time,circles) || circles<1 || circles>1000)
                return suites.fail(particle_layout::cloud_circles);
            if(!read_one_d(suites,plugin_id,effect,particle_layout::cloud_aspect,time,aspect) || aspect<1 || aspect>1000)
                return suites.fail(particle_layout::cloud_aspect);
            if(!read_one_d(suites,plugin_id,effect,particle_layout::cloud_density,time,density) || density<0 || density>1000)
                return suites.fail(particle_layout::cloud_density);
            add_value(node,kCloudCircles,circles);add_value(node,kCloudAspect,aspect);add_value(node,kCloudDensity,density);
        }
        integer=0;
        if(!suites.playback || suites.playback->fields[particle_layout::birth_enabled].present)
            if(!read_uint(suites,plugin_id,effect,particle_layout::birth_enabled,time,integer) || integer>1)
                return suites.fail(particle_layout::birth_enabled);
        if(integer) {
            double shift{},chance{};
            if(!read_one_d(suites,plugin_id,effect,particle_layout::seed_shift,time,shift) ||
               shift<double((std::numeric_limits<std::int32_t>::min)()) || shift>double((std::numeric_limits<std::int32_t>::max)()))
                return suites.fail(particle_layout::seed_shift);
            shift=std::round(shift);
            if(!read_one_d(suites,plugin_id,effect,particle_layout::birth_chance,time,chance) || chance<0 || chance>100)
                return suites.fail(particle_layout::birth_chance);
            add_value(node,kParticleSeedShift,static_cast<std::int32_t>(shift));
            add_value(node,kParticleBirthChance,chance);
        }
        if(!read_uint(suites,plugin_id,effect,particle_layout::limit_2d,time,integer) || integer>1)return false;
        add_value(node,kLimitTo2D,integer);
        for(auto [first,key]:{std::pair{particle_layout::angle,kParticleAngles},std::pair{particle_layout::speed,kRotationSpeed}}) {
            if(!read_one_d(suites,plugin_id,effect,first,time,vector.x) ||
               !read_one_d(suites,plugin_id,effect,first+1,time,vector.y) ||
               !read_one_d(suites,plugin_id,effect,first+2,time,vector.z)) return false;
            add_value(node,key,vector);
        }
    }

    const A_long size_curve_count_index = particle_layout::size_curve;
    const A_long size_curve_first_index = size_curve_count_index + 1;
    const A_long opacity_curve_count_index = particle_layout::opacity_curve;
    const A_long opacity_curve_first_index = opacity_curve_count_index + 1;
    core::OpaqueBytes curve{};
    bool has_curve = false;
    if (!read_curve(suites, plugin_id, effect, size_curve_count_index, size_curve_first_index,
                    time, curve, has_curve)) return false;
    if (has_curve) add_value(node, kSizeOverLifeCurve, std::move(curve));
    if (!read_curve(suites, plugin_id, effect, opacity_curve_count_index, opacity_curve_first_index,
                    time, curve, has_curve)) return false;
    if (has_curve) add_value(node, kOpacityOverLifeCurve, std::move(curve));
    if (!read_curve(suites,plugin_id,effect,particle_layout::rotation_curve,particle_layout::rotation_curve+1,
                    time,curve,has_curve,-32768.0,32768.0)) return false;
    if(has_curve)add_value(node,kRotationOverLife,std::move(curve));
    return true;
}

bool read_connections(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                      Kind kind, const A_Time& time, core::NodeId source,
                      std::vector<Connection>& output) {
    std::uint32_t count = 0;
    if (!read_uint(suites, plugin_id, effect, native_nodes::connection_count_index(kind), time, count) ||
        count > static_cast<std::uint32_t>(native_nodes::kMaxOutgoingEdges)) return false;
    for (std::uint32_t slot = 0; slot < count; ++slot) {
        core::Uuid128 destination{};
        core::Uuid128 edge{};
        if (!read_uuid(suites, plugin_id, effect,
                       native_nodes::connection_uuid_index(kind, static_cast<A_long>(slot), 0), time, destination) ||
            !read_uuid(suites, plugin_id, effect,
                       native_nodes::connection_edge_uuid_index(kind, static_cast<A_long>(slot), 0), time, edge)) return false;
        output.push_back(Connection{source, core::EdgeId{edge}, core::NodeId{destination}, kind});
    }
    return true;
}

core::NodeId output_node_id() noexcept {
    core::Uuid128 id{};
    id.bytes.back() = 0xff;
    return core::NodeId{id};
}

core::PortKey source_port(Kind kind) noexcept {
    using namespace core::graph_keys;
    switch (kind) {
        case Kind::emitter: return kEmitterParticles;
        case Kind::particle: return kParticleParticlesOut;
        case Kind::force: return kForceParticlesOut;
        case Kind::transform: return kTransformParticlesOut;
        case Kind::model: return kModelGeometryOut;
    }
    return {};
}

core::PortKey destination_port(Kind kind) noexcept {
    using namespace core::graph_keys;
    switch (kind) {
        case Kind::emitter: return kEmitterParents;
        case Kind::particle: return kParticleParticlesIn;
        case Kind::force: return kForceParticlesIn;
        case Kind::transform: return kTransformParticlesIn;
        case Kind::model: return {}; // Model has no particle input.
    }
    return {};
}

} // namespace

struct NativeAnimationPlan::Impl {
    std::vector<RawNode> nodes;
    std::map<core::NodeId, core::GraphNode> templates;
    std::map<core::NodeId, core::GraphNode> constant_nodes;
    core::LayerUnits units;
    bool decoded{};
    std::vector<NativeControlProof> proofs;
    bool all_constant{};
    std::size_t input_count{},constant_count{};
    std::uint64_t checkout_count{};
    Impl(const core::Graph& graph,A_long width,A_long height,double aspect)
        :units{double(std::max<A_long>(width,1)),double(std::max<A_long>(height,1)),aspect} {
        decoded=std::isfinite(aspect) && aspect>0 && read_binding_record(graph,nodes);
        for(const auto& node:graph.nodes) templates.emplace(node.id,node);
    }
};
NativeAnimationPlan::NativeAnimationPlan(const core::Graph& graph,A_long width,A_long height,double aspect)
    :impl_(std::make_unique<Impl>(graph,width,height,aspect)) {}
NativeAnimationPlan::~NativeAnimationPlan()=default;
bool NativeAnimationPlan::valid() const noexcept {return impl_ && impl_->decoded;}
const std::vector<NativeControlProof>& NativeAnimationPlan::proofs() const noexcept {return impl_->proofs;}
bool NativeAnimationPlan::fully_constant() const noexcept {return valid() && impl_->all_constant;}
std::size_t NativeAnimationPlan::input_count() const noexcept {return impl_->input_count;}
std::size_t NativeAnimationPlan::constant_count() const noexcept {return impl_->constant_count;}
std::uint64_t NativeAnimationPlan::checkout_count() const noexcept {return impl_->checkout_count;}
const core::GraphNode* NativeAnimationPlan::constant_node(core::NodeId id) const noexcept {
    if(!valid())return nullptr;
    const auto found=impl_->constant_nodes.find(id);
    return found==impl_->constant_nodes.end()?nullptr:&found->second;
}
std::optional<double> NativeAnimationPlan::constant_birth_chance(core::NodeId id) const noexcept {
    if(!valid())return {};
    const auto authored=impl_->templates.find(id);
    if(authored==impl_->templates.end() || authored->second.type_key!=core::graph_keys::kParticleNode)return {};
    const auto chance=std::find_if(authored->second.parameters.begin(),authored->second.parameters.end(),
        [](const auto& parameter){return parameter.key==core::graph_keys::kParticleBirthChance;});
    if(chance==authored->second.parameters.end())return 100;
    if(const auto* value=constant_node(id)) {
        for(const auto& parameter:value->parameters)if(parameter.key==core::graph_keys::kParticleBirthChance)
            if(const auto* scalar=std::get_if<double>(&parameter.value))return *scalar;
        return {};
    }
    const auto raw=std::find_if(impl_->nodes.begin(),impl_->nodes.end(),[&](const auto& node){return node.id==id;});
    if(raw==impl_->nodes.end())return {};
    const auto& field=raw->fields[particle_layout::birth_chance];
    if(field.present && (field.slot<0 || field.constant[0]))return field.value[0];
    return {};
}
void NativeAnimationPlan::prepare_constants(PF_InData* data,bool allow_static_bypass) noexcept try {
    if(!valid() || !data) return;
    impl_->constant_nodes.clear();
    std::vector<core::NodeId> ids;for(const auto& [id,node]:impl_->templates)ids.push_back(id);
    impl_->proofs=validated_native_control_proofs(data,ids);
    // The caller's graph already contains current-frame owned-alias values.
    // A simple, completely certified graph needs no second set of checkouts.
    // Partially certified and complex graphs still hoist raw historical inputs.
    if(allow_static_bypass) {
        std::size_t inputs=0,constants=0;
        for(const auto& node:impl_->nodes)if(impl_->templates.contains(node.id))
            for(const auto& field:node.fields)if(field.present && field.slot>=0)
                for(A_long c=0;c<component_count(field.type);++c) {
                    ++inputs;const auto stream=kNativeBindingFirstIndex+field.slot+c;
                    if(std::any_of(impl_->proofs.begin(),impl_->proofs.end(),[&](const auto& p){return p.node==node.id && p.stream==stream && p.constant;}))++constants;
                }
        if(inputs && inputs==constants) {
            impl_->input_count=inputs;impl_->constant_count=constants;impl_->all_constant=true;
            impl_->constant_nodes=impl_->templates;return;
        }
    }
    impl_->all_constant=!impl_->nodes.empty();
    impl_->input_count=impl_->constant_count=0;
    for(auto& node:impl_->nodes) {
      if(!impl_->templates.contains(node.id))continue;
      for(auto& field:node.fields) if(field.present && field.slot>=0) {
        for(A_long c=0;c<component_count(field.type);++c) {
            ++impl_->input_count;field.constant[c]=false;
            const auto stream=kNativeBindingFirstIndex+field.slot+c;
            const auto found=std::find_if(impl_->proofs.begin(),impl_->proofs.end(),[&](const auto& p){return p.node==node.id && p.stream==stream && p.constant;});
            if(found==impl_->proofs.end()) {impl_->all_constant=false;continue;}
            PF_ParamDef value{};
            ++impl_->checkout_count;
            const auto error=PF_CHECKOUT_PARAM(data,stream,data->current_time,data->time_step,data->time_scale,&value);
            if(error) {impl_->all_constant=false;continue;}
            const bool ok=value.param_type==PF_Param_FLOAT_SLIDER && std::isfinite(value.u.fs_d.value) && value.u.fs_d.value!=kNativeBindingUnavailable;
            const double scalar=ok?value.u.fs_d.value:0;
            const auto checkin=PF_CHECKIN_PARAM(data,&value);
            if(!ok || checkin) {impl_->all_constant=false;continue;}
            field.value[c]=scalar;field.constant[c]=true;
            ++impl_->constant_count;
        }
      }
    }
    impl_->all_constant=impl_->input_count>0 && impl_->constant_count==impl_->input_count;
    // Reuse a complete constant node across every distinct birth time. Never
    // infer this from equal samples; every bound component must have a proof.
    for(const auto& node:impl_->nodes)if(impl_->templates.contains(node.id)) {
        bool constant=true;
        for(const auto& field:node.fields)if(field.present && field.slot>=0)
            for(A_long c=0;c<component_count(field.type);++c)constant &= field.constant[c];
        if(!constant)continue;
        core::GraphNode value;
        if(sample(data,node.id,value)==PF_Err_NONE)impl_->constant_nodes.emplace(node.id,std::move(value));
    }
} catch(...) {if(impl_)impl_->all_constant=false;}
PF_Err NativeAnimationPlan::sample(PF_InData* data,core::NodeId id,core::GraphNode& output,
                                  A_long* failed_stream) const noexcept {
    if(failed_stream) *failed_stream=-1;
    try {
        if(!valid()) return PF_Err_BAD_CALLBACK_PARAM;
        const auto original=impl_->templates.find(id);
        if(original==impl_->templates.end()) return PF_Err_BAD_CALLBACK_PARAM;
        if(const auto* constant=constant_node(id)){output=*constant;return PF_Err_NONE;}
        output=original->second;
        const auto found=std::find_if(impl_->nodes.begin(),impl_->nodes.end(),[&](const auto& n){return n.id==id;});
        if(found==impl_->nodes.end()) return PF_Err_NONE;
        if(!data || !data->inter.checkout_param || !data->inter.checkin_param) return PF_Err_BAD_CALLBACK_PARAM;
        auto raw=*found;
        for(auto& field:raw.fields) if(field.present && field.slot>=0) {
            for(A_long component=0;component<component_count(field.type);++component) {
                if(field.constant[component]) continue;
                PF_ParamDef sampled{};
                const A_long index=kNativeBindingFirstIndex+field.slot+component;
                if(failed_stream) *failed_stream=index;
                ++impl_->checkout_count;
                const auto error=PF_CHECKOUT_PARAM(data,index,data->current_time,data->time_step,data->time_scale,&sampled);
                if(error) return error;
                const bool valid_value=sampled.param_type==PF_Param_FLOAT_SLIDER &&
                    std::isfinite(sampled.u.fs_d.value) && sampled.u.fs_d.value!=kNativeBindingUnavailable;
                const double value=valid_value?sampled.u.fs_d.value:0;
                const auto checked_in=PF_CHECKIN_PARAM(data,&sampled);
                if(!valid_value) return PF_Err_BAD_CALLBACK_PARAM;
                if(checked_in) return checked_in;
                field.value[component]=value;
            }
        }
        const auto expected=raw.kind==Kind::emitter?core::graph_keys::kEmitterNode:
            raw.kind==Kind::particle?core::graph_keys::kParticleNode:
            raw.kind==Kind::transform?core::graph_keys::kTransformNode:
            raw.kind==Kind::model?core::graph_keys::kModelNode:core::graph_keys::kForceNode;
        if(output.type_key!=expected) return PF_Err_BAD_CALLBACK_PARAM;
        SuiteSet reader(nullptr);
        reader.playback=&raw;
        output.parameters.clear();
        return read_node_parameters(reader,0,nullptr,raw.kind,{},impl_->units,output)?PF_Err_NONE:PF_Err_BAD_CALLBACK_PARAM;
    } catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
      catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}

PF_Err compile_native_node_graph(PF_InData* in_data, PF_ParamDef* params[],
                                 core::Graph& graph, bool& found_node_effects, AEGP_PluginID plugin_id,
                                 const node_sync::NativeEdit* edit) noexcept {
    graph = {};
    found_node_effects = false;
    if (!in_data || !params || !in_data->pica_basicP || (!in_data->effect_ref && !(edit && edit->layer))) return PF_Err_BAD_CALLBACK_PARAM;
    if (edit && !node_sync::valid_edit(*edit)) return PF_Err_BAD_CALLBACK_PARAM;
    if (plugin_id == 0) return PF_Err_BAD_CALLBACK_PARAM;

    try {
        SuiteSet suites(in_data);
        suites.edit = edit;
        PF_Err error = suites.acquire();
        if (error != PF_Err_NONE) return error;
        AEGP_LayerH layer = edit ? edit->layer : nullptr;
        A_Err ae_error = layer ? 0 : suites.pf_interface->AEGP_GetEffectLayer(in_data->effect_ref, &layer);
        if (ae_error || !layer) return static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);
        suites.owner_layer=layer;

        A_long effect_count = 0;
        ae_error = suites.effect->AEGP_GetLayerNumEffects(layer, &effect_count);
        if (ae_error) return static_cast<PF_Err>(ae_error);
        const A_Time time{in_data->current_time, in_data->time_scale};
        // AEGP stream values are full-resolution layer pixels, unlike PF point
        // checkouts. Do not apply preview downsample scaling to them.
        const core::LayerUnits units{static_cast<double>(std::max<A_long>(in_data->width, 1)),
                                    static_cast<double>(std::max<A_long>(in_data->height, 1)),
                                    in_data->pixel_aspect_ratio.den ?
                                        static_cast<double>(in_data->pixel_aspect_ratio.num) / in_data->pixel_aspect_ratio.den : 1.0};
        std::vector<RawNode> raw_nodes;
        std::vector<RawNode> previous_nodes;
        if(params[kGraphParameterId] && params[kGraphParameterId]->param_type==PF_Param_ARBITRARY_DATA &&
           params[kGraphParameterId]->u.arb_d.value) {
            auto saved=read_graph_parameter(in_data,params[kGraphParameterId]->u.arb_d.value);
            if(!saved.has_value() || !read_binding_record(saved.value(),previous_nodes))return PF_Err_BAD_CALLBACK_PARAM;
        }
        std::vector<Connection> connections;
        std::vector<LayoutEntry> layout_entries;
        graph.nodes.reserve(static_cast<std::size_t>(effect_count) + 1u);
        connections.reserve(static_cast<std::size_t>(effect_count) * 2u);
        layout_entries.reserve(static_cast<std::size_t>(effect_count) + 1u);

        for (A_long effect_index = 0; effect_index < effect_count; ++effect_index) {
            suites.edited_effect = nullptr; // EffectRef handles may be reused after disposal.
            AEGP_EffectRefH raw_effect = nullptr;
            ae_error = suites.effect->AEGP_GetLayerEffectByIndex(plugin_id, layer, effect_index, &raw_effect);
            if (ae_error) return static_cast<PF_Err>(ae_error);
            if (!raw_effect) continue;
            EffectRef effect(suites.effect, raw_effect);
            AEGP_InstalledEffectKey installed_key = AEGP_InstalledEffectKey_NONE;
            char match_name[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
            ae_error = suites.effect->AEGP_GetInstalledKeyFromLayerEffect(effect.value, &installed_key);
            if (!ae_error) ae_error = suites.effect->AEGP_GetEffectMatchName(installed_key, match_name);
            if (ae_error) return static_cast<PF_Err>(ae_error);
            if (std::strcmp(match_name, kRendererMatchName) == 0) continue;

            Kind kind{};
            const char* type_key = nullptr;
            std::uint16_t schema_version = 0;
            if (!decode_node_kind(match_name, kind, type_key, schema_version)) continue;
            double transaction_guard{};
            if(!read_one_d(suites,plugin_id,effect.value,native_nodes::sync_guard_index(kind),time,transaction_guard))
                return PF_Err_BAD_CALLBACK_PARAM;
            if(transaction_guard==transaction_backup_guard)continue;
            found_node_effects = true;
            if (graph.nodes.size() >= core::kMaxGraphNodes - 1u) return PF_Err_BAD_CALLBACK_PARAM;

            core::Uuid128 uuid{};
            if (!read_uuid(suites, plugin_id, effect.value, native_nodes::uuid_first_index(kind), time, uuid))
                return PF_Err_BAD_CALLBACK_PARAM;
            if (edit && static_cast<std::uint32_t>(kind) == edit->node_kind) {
                bool matches = true;
                for (std::size_t chunk = 0; chunk < edit->uuid.size(); ++chunk) {
                    const auto word = static_cast<std::uint16_t>((uuid.bytes[chunk * 2] << 8u) | uuid.bytes[chunk * 2 + 1]);
                    matches = matches && word == edit->uuid[chunk];
                }
                if (matches) {
                    if (suites.edit_matched) return PF_Err_BAD_CALLBACK_PARAM;
                    suites.edit_matched = true;
                    suites.edited_effect = effect.value;
                }
            }
            RawNode raw; raw.id = core::NodeId{uuid}; raw.kind = kind;
            suites.previous_node=nullptr;
            for(const auto& old:previous_nodes)if(old.id==raw.id && old.kind==raw.kind){suites.previous_node=&old;break;}
            suites.recording = &raw;
            core::GraphNode node{core::NodeId{uuid}, type_key, schema_version, {}};
            if (!read_node_parameters(suites, plugin_id, effect.value, kind, time, units, node))
                return PF_Err_BAD_CALLBACK_PARAM;
            suites.recording = nullptr; raw_nodes.push_back(std::move(raw));
            double layout_x = 0.0;
            double layout_y = 0.0;
            if (!read_one_d(suites, plugin_id, effect.value, native_nodes::layout_x_index(kind), time, layout_x) ||
                !read_one_d(suites, plugin_id, effect.value, native_nodes::layout_y_index(kind), time, layout_y) ||
                std::abs(layout_x) > 1000000000.0 || std::abs(layout_y) > 1000000000.0)
                return PF_Err_BAD_CALLBACK_PARAM;
            layout_entries.push_back(LayoutEntry{core::NodeId{uuid}, layout_x, layout_y});
            if (!read_connections(suites, plugin_id, effect.value, kind, time, node.id, connections))
                return PF_Err_BAD_CALLBACK_PARAM;
            graph.nodes.push_back(std::move(node));
        }

        if (edit && !suites.edit_applied) return PF_Err_BAD_CALLBACK_PARAM;
        if (!params[kMaxParticlesId] || params[kMaxParticlesId]->param_type != PF_Param_FLOAT_SLIDER ||
            !std::isfinite(params[kMaxParticlesId]->u.fs_d.value) || params[kMaxParticlesId]->u.fs_d.value < 0.0 ||
            params[kMaxParticlesId]->u.fs_d.value > static_cast<PF_FpLong>(std::numeric_limits<std::uint32_t>::max()) ||
            std::floor(params[kMaxParticlesId]->u.fs_d.value) != params[kMaxParticlesId]->u.fs_d.value) {
            return PF_Err_BAD_CALLBACK_PARAM;
        }
        const core::NodeId output_id = output_node_id();
        if (!params[kLayoutOutputXId] || !params[kLayoutOutputYId] ||
            params[kLayoutOutputXId]->param_type != PF_Param_FLOAT_SLIDER ||
            params[kLayoutOutputYId]->param_type != PF_Param_FLOAT_SLIDER ||
            !std::isfinite(params[kLayoutOutputXId]->u.fs_d.value) ||
            !std::isfinite(params[kLayoutOutputYId]->u.fs_d.value)) return PF_Err_BAD_CALLBACK_PARAM;
        layout_entries.push_back(LayoutEntry{output_id,
            params[kLayoutOutputXId]->u.fs_d.value, params[kLayoutOutputYId]->u.fs_d.value});
        graph.nodes.push_back(core::GraphNode{output_id, core::graph_keys::kOutputNode, 4,
            {{core::graph_keys::kParticleCount,
              static_cast<std::uint32_t>(params[kMaxParticlesId]->u.fs_d.value)}}});
        auto& output = graph.nodes.back();
        if(!params[kTimeSamplingHzId] || params[kTimeSamplingHzId]->param_type!=PF_Param_POPUP ||
           params[kTimeSamplingHzId]->u.pd.value<1 || params[kTimeSamplingHzId]->u.pd.value>3) return PF_Err_BAD_CALLBACK_PARAM;
        add_value(output,core::graph_keys::kTimeSamplingHz,std::uint32_t(30u<<(params[kTimeSamplingHzId]->u.pd.value-1)));
        if(!params[kAccelerationId] || params[kAccelerationId]->param_type!=PF_Param_POPUP ||
            params[kAccelerationId]->u.pd.value<1 || params[kAccelerationId]->u.pd.value>2) return PF_Err_BAD_CALLBACK_PARAM;
        add_value(output,core::graph_keys::kAcceleration,std::uint32_t(params[kAccelerationId]->u.pd.value-1));
        if(const auto motion_error=append_motion_values(params,output);motion_error)return motion_error;
        for (const auto& binding : {std::pair{kTimeRemapEnabledId, core::graph_keys::kTimeRemapEnabled},
                                   std::pair{kPreviewEnabledId, core::graph_keys::kPreviewEnabled}}) {
            if (!params[binding.first] || params[binding.first]->param_type != PF_Param_CHECKBOX) return PF_Err_BAD_CALLBACK_PARAM;
            add_value(output, binding.second, std::uint32_t(params[binding.first]->u.bd.value != 0));
        }
        for (const auto& binding : {std::pair{kTimeRemapSecondsId, core::graph_keys::kTimeRemapSeconds},
                                   std::pair{kPreviewChanceId, core::graph_keys::kPreviewChance}}) {
            if (!params[binding.first] || params[binding.first]->param_type != PF_Param_FLOAT_SLIDER) return PF_Err_BAD_CALLBACK_PARAM;
            add_value(output, binding.second, double(params[binding.first]->u.fs_d.value));
        }

        for (const auto& connection : connections) {
            Kind destination_kind = Kind::emitter;
            bool found_destination = false;
            if (connection.destination == output_id) {
                graph.edges.push_back(core::GraphEdge{connection.edge, connection.source,
                    source_port(connection.source_kind), output_id, core::graph_keys::kOutputParticles});
                continue;
            }
            for (const auto& node : graph.nodes) {
                if (node.id == connection.destination) {
                    found_destination = true;
                    if (node.type_key == core::graph_keys::kParticleNode) destination_kind = Kind::particle;
                    else if (node.type_key == core::graph_keys::kForceNode) destination_kind = Kind::force;
                    else if (node.type_key == core::graph_keys::kTransformNode) destination_kind = Kind::transform;
                    else if (node.type_key == core::graph_keys::kModelNode) destination_kind = Kind::model;
                    break;
                }
            }
            graph.edges.push_back(core::GraphEdge{connection.edge, connection.source,
                source_port(connection.source_kind), connection.destination,
                found_destination ? (connection.source_kind==Kind::model && destination_kind==Kind::particle ?
                    core::graph_keys::kParticleModelsIn : destination_port(destination_kind)) : core::PortKey{1}});
        }

        const auto validation = core::validate_graph(graph, core::particle_node_registry());
        if (validation.ok()) {
            graph.optional_records.push_back(make_layout_record(layout_entries));
            auto bindings = make_binding_record(raw_nodes);
            if (bindings.empty() && !raw_nodes.empty()) return PF_Err_BAD_CALLBACK_PARAM;
            if (!bindings.empty()) graph.optional_records.push_back(std::move(bindings));
        }
        return validation.ok() ? PF_Err_NONE : PF_Err_BAD_CALLBACK_PARAM;
    } catch (const std::bad_alloc&) {
        return PF_Err_OUT_OF_MEMORY;
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
}

struct NativeBindingTransaction::Impl {
    struct Change {
        AEGP_StreamRefH ref{};
        std::u16string previous;
        A_Boolean enabled{};
        double previous_value{};
        A_long index{-1};
        A_long parameter{-1};
        bool changed{};
    };
    PF_InData* data{};
    AEGP_PluginID id{};
    AEGP_EffectRefH renderer{};
    AEGP_LayerH owner_layer{};
    bool owned_renderer{};
    SuiteSet suites;
    const AEGP_MemorySuite1* memory{};
    std::vector<Change> changes;
    struct ResourceChange {AEGP_StreamRefH ref{};AEGP_LayerIDVal previous{};bool count{},changed{};};
    std::vector<ResourceChange> resources;
    std::unique_ptr<ModelMirrorTransaction> models;
    bool accepted{};
    Impl(PF_InData* d, AEGP_PluginID i, AEGP_EffectRefH r, AEGP_LayerH owner)
        : data(d), id(i), renderer(r), owner_layer(owner), suites(d) {}
    ~Impl() {
        models.reset();
        for(auto& change:resources) {
            if(!accepted && change.changed) {
                AEGP_StreamValue2 value{};value.streamH=change.ref;
                if(change.count)value.val.one_d=double(change.previous);else value.val.layer_id=change.previous;
                suites.stream->AEGP_SetStreamValue(id,change.ref,&value);
            }
            if(change.ref)suites.stream->AEGP_DisposeStream(change.ref);
        }
        for (auto& change : changes) {
            if (!accepted && change.changed) {
                AEGP_StreamValue2 value{}; value.streamH = change.ref; value.val.one_d = change.previous_value;
                suites.stream->AEGP_SetStreamValue(id, change.ref, &value);
                suites.stream->AEGP_SetExpression(id, change.ref, reinterpret_cast<const A_UTF16Char*>(change.previous.c_str()));
                suites.stream->AEGP_SetExpressionState(id, change.ref, change.enabled);
            }
            if (change.ref) suites.stream->AEGP_DisposeStream(change.ref);
        }
        if (owned_renderer && renderer) suites.effect->AEGP_DisposeEffect(renderer);
        if (memory) suites.basic->ReleaseSuite(kAEGPMemorySuite, kAEGPMemorySuiteVersion1);
    }
};

NativeBindingTransaction::NativeBindingTransaction(PF_InData* data, AEGP_PluginID id, AEGP_EffectRefH renderer)
    : NativeBindingTransaction(data,id,renderer,nullptr) {}
NativeBindingTransaction::NativeBindingTransaction(PF_InData* data, AEGP_PluginID id,
    AEGP_EffectRefH renderer, AEGP_LayerH owner_layer)
    : impl_(std::make_unique<Impl>(data, id, renderer, owner_layer)) {}
NativeBindingTransaction::~NativeBindingTransaction() = default;
void NativeBindingTransaction::accept() noexcept { impl_->accepted = true; if(impl_->models)impl_->models->accept(); }
PF_Err NativeBindingTransaction::rollback_model_resources() noexcept {
    return impl_->models?impl_->models->rollback():PF_Err_NONE;
}

PF_Err NativeBindingTransaction::install(const core::Graph& graph, A_long* failed_stream,
                                        const char** failed_stage, A_long* failed_parameter) noexcept {
    if (failed_stream) *failed_stream = -1;
    if (failed_parameter) *failed_parameter = -1;
    const auto stage = [&](const char* value) { if (failed_stage) *failed_stage = value; };
    stage("decode binding record");
    try {
        std::vector<RawNode> nodes;
        if (!read_binding_record(graph, nodes)) return PF_Err_BAD_CALLBACK_PARAM;
        auto& tx = *impl_;
        stage("acquire binding suites");
        auto error = tx.suites.acquire(); if (error) return error;
        A_Err ae = tx.suites.basic->AcquireSuite(kAEGPMemorySuite, kAEGPMemorySuiteVersion1,
            reinterpret_cast<const void**>(&tx.memory));
        if (ae) return static_cast<PF_Err>(ae);
        if (!tx.renderer) {
            stage("resolve renderer");
            ae = tx.suites.pf_interface->AEGP_GetNewEffectForEffect(tx.id, tx.data->effect_ref, &tx.renderer);
            tx.owned_renderer = true;
            if (ae || !tx.renderer) return static_cast<PF_Err>(ae ? ae : PF_Err_BAD_CALLBACK_PARAM);
        }
        // Resource slots belong to the renderer: SmartFX cannot checkout a sibling
        // Particle effect's PF_LAYER parameter. Clear only the former occupied tail.
        std::vector<AEGP_LayerIDVal> resource_ids;
        for(const auto& node:nodes)if(node.kind==Kind::particle)
            for(auto index:{particle_layout::texture_front,particle_layout::texture_back}) {
                const auto& field=node.fields[index];if(!field.present)continue;
                const double value=field.value[0];
                if(field.type!=node_sync::ValueKind::scalar || value<0 || value>0x7fffffff || std::floor(value)!=value)
                    return PF_Err_BAD_CALLBACK_PARAM;
                if(value>0)resource_ids.push_back(static_cast<AEGP_LayerIDVal>(value));
            }
        std::sort(resource_ids.begin(),resource_ids.end());
        resource_ids.erase(std::unique(resource_ids.begin(),resource_ids.end()),resource_ids.end());
        if(resource_ids.size()>kTextureResourceCapacity)return PF_Err_BAD_CALLBACK_PARAM;
        if(!resource_ids.empty()) {
            stage("acquire texture source layer suite");
            ae=tx.suites.basic->AcquireSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,reinterpret_cast<const void**>(&tx.suites.layer));
            if(ae || !tx.suites.layer)return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
            stage("resolve texture owner layer");
            AEGP_LayerH owner=tx.owner_layer;AEGP_CompH comp{};
            if(!owner) {
                if(!tx.data || !tx.data->effect_ref)return PF_Err_BAD_CALLBACK_PARAM;
                ae=tx.suites.pf_interface->AEGP_GetEffectLayer(tx.data->effect_ref,&owner);
                if(ae || !owner)return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
            }
            stage("resolve texture owner composition");
            ae=tx.suites.layer->AEGP_GetLayerParentComp(owner,&comp);
            if(ae || !comp)return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
            stage("validate texture source layers");
            for(const auto id:resource_ids) {
                AEGP_LayerH source{};ae=tx.suites.layer->AEGP_GetLayerFromLayerID(comp,id,&source);
                if(ae || !source || source==owner)return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
            }
        }
        tx.resources.reserve(kTextureResourceCapacity+1);
        auto set_resource=[&](A_long index,bool count,AEGP_LayerIDVal desired)->PF_Err {
            tx.resources.emplace_back();auto& change=tx.resources.back();change.count=count;
            if(failed_stream)*failed_stream=index;
            stage("open texture resource stream");
            A_Err result=tx.suites.stream->AEGP_GetNewEffectStreamByIndex(tx.id,tx.renderer,index,&change.ref);
            if(result || !change.ref)return static_cast<PF_Err>(result?result:PF_Err_BAD_CALLBACK_PARAM);
            AEGP_StreamType type{};result=tx.suites.stream->AEGP_GetStreamType(change.ref,&type);
            if(result || type!=(count?AEGP_StreamType_OneD:AEGP_StreamType_LAYER_ID))return static_cast<PF_Err>(result?result:PF_Err_BAD_CALLBACK_PARAM);
            const A_Time time{tx.data->current_time,tx.data->time_scale};AEGP_StreamValue2 previous{};
            result=tx.suites.stream->AEGP_GetNewStreamValue(tx.id,change.ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&previous);
            if(result)return static_cast<PF_Err>(result);
            const double old=count?previous.val.one_d:double(previous.val.layer_id);
            tx.suites.stream->AEGP_DisposeStreamValue(&previous);
            if(!std::isfinite(old) || old<0 || old>(count?kTextureResourceCapacity:0x7fffffff) || std::floor(old)!=old)return PF_Err_BAD_CALLBACK_PARAM;
            change.previous=static_cast<AEGP_LayerIDVal>(old);
            if(change.previous==desired)return PF_Err_NONE;
            AEGP_StreamValue2 value{};value.streamH=change.ref;
            if(count)value.val.one_d=double(desired);else value.val.layer_id=desired;
            change.changed=true;stage("write texture resource stream");
            return static_cast<PF_Err>(tx.suites.stream->AEGP_SetStreamValue(tx.id,change.ref,&value));
        };
        if(auto result=set_resource(kTextureResourceCountIndex,true,static_cast<AEGP_LayerIDVal>(resource_ids.size()));result)return result;
        const auto old_count=tx.resources.front().previous;
        for(A_long slot=0;slot<std::max<A_long>(old_count,static_cast<A_long>(resource_ids.size()));++slot)
            if(auto result=set_resource(kTextureResourceFirstIndex+slot,false,
                slot<static_cast<A_long>(resource_ids.size())?resource_ids[slot]:0);result)return result;
        tx.models=std::make_unique<ModelMirrorTransaction>(tx.data,tx.id,tx.renderer,tx.owner_layer);
        if(const auto result=tx.models->install(graph,failed_stream,failed_stage);result)return result;
        tx.changes.reserve(kNativeBindingCapacity);
        for (const auto& node : nodes) for (A_long index = 1; index < static_cast<A_long>(node.fields.size()); ++index) {
            const auto& field = node.fields[index]; if (!field.present || field.slot < 0) continue;
            for (A_long component = 0; component < component_count(field.type); ++component) {
                tx.changes.emplace_back(); auto& change = tx.changes.back();
                change.index = kNativeBindingFirstIndex + field.slot + component;
                change.parameter = index;
                if (failed_stream) *failed_stream = change.index;
                if (failed_parameter) *failed_parameter = index;
                stage("open alias stream");
                ae = tx.suites.stream->AEGP_GetNewEffectStreamByIndex(tx.id, tx.renderer,
                    kNativeBindingFirstIndex + field.slot + component, &change.ref);
                if (ae || !change.ref) return static_cast<PF_Err>(ae ? ae : PF_Err_BAD_CALLBACK_PARAM);
                AEGP_StreamType type = AEGP_StreamType_NO_DATA;
                stage("read alias type");
                ae = tx.suites.stream->AEGP_GetStreamType(change.ref, &type);
                if (ae || type != AEGP_StreamType_OneD) return static_cast<PF_Err>(ae ? ae : PF_Err_BAD_CALLBACK_PARAM);
                stage("read expression state");
                ae = tx.suites.stream->AEGP_GetExpressionState(tx.id, change.ref, &change.enabled);
                if (ae) return static_cast<PF_Err>(ae);
                AEGP_MemHandle handle{};
                stage("read expression text");
                ae = tx.suites.stream->AEGP_GetExpression(tx.id, change.ref, &handle);
                if (ae) return static_cast<PF_Err>(ae);
                if (handle) {
                    stage("lock expression text");
                    void* text = nullptr; ae = tx.memory->AEGP_LockMemHandle(handle, &text);
                    struct LockedExpression {
                        const AEGP_MemorySuite1* suite;
                        AEGP_MemHandle handle;
                        bool locked;
                        ~LockedExpression() {
                            if (locked) suite->AEGP_UnlockMemHandle(handle);
                            suite->AEGP_FreeMemHandle(handle);
                        }
                    } owned{tx.memory, handle, ae == 0};
                    if (!ae && text) change.previous = reinterpret_cast<const char16_t*>(text);
                    if (ae) return static_cast<PF_Err>(ae);
                }
                const auto desired = binding_expression(node, index, component);
                if (change.previous == desired && change.enabled) continue;
                // Snapshot values only for aliases we will actually mutate.
                // An unchanged, enabled expression was checked against the exact
                // freshly compiled node identity and source parameter above.
                const A_Time sample_time{tx.data->current_time, tx.data->time_scale};
                AEGP_StreamValue2 previous{};
                stage("read previous value");
                ae = tx.suites.stream->AEGP_GetNewStreamValue(tx.id, change.ref, AEGP_LTimeMode_LayerTime, &sample_time, TRUE, &previous);
                if (ae) return static_cast<PF_Err>(ae);
                change.previous_value = previous.val.one_d;
                tx.suites.stream->AEGP_DisposeStreamValue(&previous);
                change.changed = true;
                AEGP_StreamValue2 sentinel{}; sentinel.streamH = change.ref; sentinel.val.one_d = kNativeBindingUnavailable;
                stage("write alias sentinel");
                ae = tx.suites.stream->AEGP_SetStreamValue(tx.id, change.ref, &sentinel);
                if (ae) return static_cast<PF_Err>(ae);
                stage("write expression");
                ae = tx.suites.stream->AEGP_SetExpression(tx.id, change.ref, reinterpret_cast<const A_UTF16Char*>(desired.c_str()));
                if (!ae) {
                    stage("enable expression");
                    ae = tx.suites.stream->AEGP_SetExpressionState(tx.id, change.ref, TRUE);
                }
                if (ae) return static_cast<PF_Err>(ae);
            }
        }
        // Newly installed/repaired expressions must evaluate before acceptance.
        // Matching enabled aliases were not modified: repeatedly evaluating all
        // of them on a constant curve edit adds host work without a new binding.
        const A_Time sample_time{tx.data->current_time, tx.data->time_scale};
        for (const auto& change : tx.changes) {
            if (!change.changed) continue;
            if (failed_stream) *failed_stream = change.index;
            if (failed_parameter) *failed_parameter = change.parameter;
            AEGP_StreamValue2 evaluated{};
            stage("read evaluated value");
            ae = tx.suites.stream->AEGP_GetNewStreamValue(tx.id, change.ref, AEGP_LTimeMode_LayerTime, &sample_time, FALSE, &evaluated);
            if (ae) return static_cast<PF_Err>(ae);
            const double value = evaluated.val.one_d;
            tx.suites.stream->AEGP_DisposeStreamValue(&evaluated);
            A_Boolean enabled = FALSE;
            stage("verify expression enabled");
            ae = tx.suites.stream->AEGP_GetExpressionState(tx.id, change.ref, &enabled);
            if (ae || !enabled) return static_cast<PF_Err>(ae ? ae : PF_Err_BAD_CALLBACK_PARAM);
            stage("verify finite bound value");
            if (!std::isfinite(value) || value == kNativeBindingUnavailable) return PF_Err_BAD_CALLBACK_PARAM;
        }
        stage("accepted bindings");
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

PF_Err read_native_origin_bindings(const core::Graph& graph, std::vector<NativeOriginBinding>& bindings) noexcept {
    try {
        std::vector<RawNode> nodes;
        if(!read_binding_record(graph,nodes)) return PF_Err_BAD_CALLBACK_PARAM;
        for(const auto& node:nodes) if(node.kind==Kind::emitter) {
            const auto& xy=node.fields[4];const auto& z=node.fields[5];
            if(!xy.present || xy.type!=node_sync::ValueKind::point2 || xy.slot<0 ||
               !z.present || z.type!=node_sync::ValueKind::scalar || z.slot<0) return PF_Err_BAD_CALLBACK_PARAM;
            const auto& rate=node.fields[3];
            if(!rate.present || rate.slot<0 || rate.type!=node_sync::ValueKind::scalar) return PF_Err_BAD_CALLBACK_PARAM;
            bindings.push_back({node.id,kNativeBindingFirstIndex+xy.slot,kNativeBindingFirstIndex+xy.slot+1,kNativeBindingFirstIndex+z.slot,
                kNativeBindingFirstIndex+rate.slot});
        }
        return PF_Err_NONE;
    } catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
    catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}

PF_Err read_native_lifetime_bindings(const core::Graph& graph,std::vector<NativeLifetimeBinding>& bindings) noexcept {
    try {
        std::vector<RawNode> nodes;if(!read_binding_record(graph,nodes)) return PF_Err_BAD_CALLBACK_PARAM;
        for(const auto& node:nodes) if(node.kind==Kind::particle) {
            const auto& life=node.fields[2];
            if(!life.present || life.slot<0 || life.type!=node_sync::ValueKind::scalar) return PF_Err_BAD_CALLBACK_PARAM;
            bindings.push_back({node.id,kNativeBindingFirstIndex+life.slot});
        }
        return PF_Err_NONE;
    } catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
      catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}

PF_Err sample_native_node_animation(PF_InData* data, core::Graph& graph, A_long width, A_long height,
                                    A_long* failed_stream, const char** failed_stage, const core::NodeId* node_filter) noexcept {
    if (failed_stream) *failed_stream = -1;
    const auto stage = [failed_stage](const char* value) { if (failed_stage) *failed_stage = value; };
    try {
        std::vector<RawNode> nodes;
        stage("binding record");
        if (!read_binding_record(graph, nodes)) return PF_Err_BAD_CALLBACK_PARAM;
        if (nodes.empty()) return PF_Err_NONE;
        stage("host callbacks");
        // num_params describes the delivered params[] array. SmartFX has no such
        // array; registered streams are available through checkout regardless.
        if (!data || !data->inter.checkout_param || !data->inter.checkin_param) return PF_Err_BAD_CALLBACK_PARAM;
        const core::LayerUnits units{double(std::max<A_long>(width > 0 ? width : data->width, 1)),
            double(std::max<A_long>(height > 0 ? height : data->height, 1)),
            data->pixel_aspect_ratio.den ? double(data->pixel_aspect_ratio.num) / data->pixel_aspect_ratio.den : 1.0};
        SuiteSet reader(nullptr); // No AEGP acquisition, even during destruction.
        for (auto& raw : nodes) {
            if(node_filter && raw.id!=*node_filter) continue;
            for (auto& field : raw.fields) if (field.present && field.slot >= 0) {
                for (A_long component = 0; component < component_count(field.type); ++component) {
                    PF_ParamDef sampled{};
                    if (failed_stream) *failed_stream = kNativeBindingFirstIndex + field.slot + component;
                    stage("parameter checkout");
                    const auto error = PF_CHECKOUT_PARAM(data, kNativeBindingFirstIndex + field.slot + component,
                        data->current_time, data->time_step, data->time_scale, &sampled);
                    if (error) return error;
                    const bool valid = sampled.param_type == PF_Param_FLOAT_SLIDER &&
                        std::isfinite(sampled.u.fs_d.value) && sampled.u.fs_d.value != kNativeBindingUnavailable;
                    const double value = valid ? sampled.u.fs_d.value : 0;
                    const auto checked_in = PF_CHECKIN_PARAM(data, &sampled);
                    stage("parameter value");
                    if (!valid) return PF_Err_BAD_CALLBACK_PARAM;
                    stage("parameter checkin");
                    if (checked_in) return checked_in;
                    field.value[component] = value;
                }
            }
            auto node = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& n) {return n.id == raw.id;});
            stage("node lookup");
            if (node == graph.nodes.end()) return PF_Err_BAD_CALLBACK_PARAM;
            const auto expected = raw.kind == Kind::emitter ? core::graph_keys::kEmitterNode :
                raw.kind == Kind::particle ? core::graph_keys::kParticleNode :
                raw.kind==Kind::transform?core::graph_keys::kTransformNode:
                raw.kind==Kind::model?core::graph_keys::kModelNode:core::graph_keys::kForceNode;
            stage("node type");
            if (node->type_key != expected) return PF_Err_BAD_CALLBACK_PARAM;
            node->parameters.clear(); reader.playback = &raw;
            stage("node conversion");
            if (!read_node_parameters(reader, 0, nullptr, raw.kind, {}, units, *node)) return PF_Err_BAD_CALLBACK_PARAM;
        }
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { stage("allocation"); return PF_Err_OUT_OF_MEMORY; }
    catch (...) { stage("exception"); return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

// UI-only metadata capture. Optional optimization: unavailable metadata retains
// the exact historical path and never interrupts an authored transaction.
PF_Err capture_current_native_temporal_metadata(PF_InData* data,AEGP_PluginID id) noexcept try {
    if(!data || !data->effect_ref || !data->pica_basicP || !id)return PF_Err_BAD_CALLBACK_PARAM;
    SuiteSet suites(data);if(const auto error=suites.acquire();error)return error;
    if(!suites.pf_interface->AEGP_GetNewEffectForEffect || !suites.effect->AEGP_DisposeEffect ||
        !suites.stream->AEGP_GetNewEffectStreamByIndex || !suites.stream->AEGP_GetStreamType ||
        !suites.stream->AEGP_GetNewStreamValue || !suites.stream->AEGP_DisposeStreamValue ||
        !suites.stream->AEGP_DisposeStream)return PF_Err_BAD_CALLBACK_PARAM;
    AEGP_EffectRefH raw_effect{};
    auto error=suites.pf_interface->AEGP_GetNewEffectForEffect(id,data->effect_ref,&raw_effect);
    if(error || !raw_effect)return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
    EffectRef effect(suites.effect,raw_effect);
    AEGP_StreamRefH raw_stream{};
    error=suites.stream->AEGP_GetNewEffectStreamByIndex(id,raw_effect,kGraphParameterId,&raw_stream);
    if(error || !raw_stream)return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
    StreamRef stream(suites.stream,raw_stream);
    AEGP_StreamType type=AEGP_StreamType_NO_DATA;
    error=suites.stream->AEGP_GetStreamType(raw_stream,&type);
    if(error || type!=AEGP_StreamType_ARB)return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
    const A_Time zero{0,1};AEGP_StreamValue2 value{};
    error=suites.stream->AEGP_GetNewStreamValue(id,raw_stream,AEGP_LTimeMode_LayerTime,&zero,TRUE,&value);
    if(error)return static_cast<PF_Err>(error);
    struct ValueRef {
        const AEGP_StreamSuite6* suite;AEGP_StreamValue2* value;
        ~ValueRef(){suite->AEGP_DisposeStreamValue(value);}
    } owned_value{suites.stream,&value};
    // Copy the graph while the returned ARB value is still alive. This is an
    // AEGP UI stream read, never PF parameter checkout/checkin during RESETUP.
    const auto graph=read_graph_parameter(data,reinterpret_cast<PF_ArbitraryH>(value.val.arbH));
    if(!graph.has_value())return PF_Err_BAD_CALLBACK_PARAM;
    capture_native_temporal_metadata(data,graph.value(),id);
    return PF_Err_NONE;
} catch(...) {return PF_Err_BAD_CALLBACK_PARAM;}

void capture_native_temporal_metadata(PF_InData* data,const core::Graph& graph,AEGP_PluginID id) noexcept try {
    struct Trace {
        NativeMetadataTrace value{0,0,0,PF_Err_BAD_CALLBACK_PARAM};
        ~Trace(){record_native_metadata_trace(value);}
    } trace;
    if(!data || !data->effect_ref || !data->pica_basicP || !id) return;
    std::vector<RawNode> nodes;if(!read_binding_record(graph,nodes) || nodes.empty())return;
    SuiteSet suites(data);if(suites.acquire())return;
    struct ExtraSuites {
        SPBasicSuite* basic;const PF_ParamUtilsSuite3* utils{};const AEGP_KeyframeSuite5* keys{};
        ~ExtraSuites() {
            if(utils)basic->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);
            if(keys)basic->ReleaseSuite(kAEGPKeyframeSuite,kAEGPKeyframeSuiteVersion5);
        }
    } extra{data->pica_basicP};
    if(extra.basic->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&extra.utils)) ||
       !extra.utils || !extra.utils->PF_GetCurrentState || !extra.utils->PF_AreStatesIdentical)return;
    (void)extra.basic->AcquireSuite(kAEGPKeyframeSuite,kAEGPKeyframeSuiteVersion5,reinterpret_cast<const void**>(&extra.keys));
    if(!suites.stream->AEGP_CanVaryOverTime || !suites.stream->AEGP_GetExpressionState || !extra.keys || !extra.keys->AEGP_GetStreamNumKFs)return;
    AEGP_LayerH layer{};A_long count{};
    if(suites.pf_interface->AEGP_GetEffectLayer(data->effect_ref,&layer) || !layer ||
        suites.effect->AEGP_GetLayerNumEffects(layer,&count) || count<0 || count>10000)return;
    trace.value.error=PF_Err_NONE;
    // Restored expressions can have lazy dependencies. Options used to be the
    // only route that evaluated these aliases before PF_GetCurrentState. A token
    // captured first can become obsolete on the first render's evaluation.
    // Evaluate EVERY active alias before ANY proof token, using owned AEGP UI
    // values, not the forbidden sequence PF checkout/checkin callbacks.
    AEGP_EffectRefH raw_renderer{};
    auto error=suites.pf_interface->AEGP_GetNewEffectForEffect(id,data->effect_ref,&raw_renderer);
    if(error || !raw_renderer){trace.value.error=static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);return;}
    EffectRef renderer(suites.effect,raw_renderer);
    std::array<bool,kNativeBindingCapacity> evaluated{};
    const A_Time warm_time=data->time_scale?A_Time{data->current_time,data->time_scale}:A_Time{0,1};
    for(const auto& node:nodes) for(const auto& field:node.fields) {
        if(!field.present || field.slot<0)continue;
        for(A_long c=0;c<component_count(field.type);++c) {
            ++trace.value.inputs;
            const auto slot=field.slot+c;
            if(slot<0 || slot>=kNativeBindingCapacity){trace.value.error=PF_Err_BAD_CALLBACK_PARAM;continue;}
            AEGP_StreamRefH raw{};
            error=suites.stream->AEGP_GetNewEffectStreamByIndex(id,raw_renderer,kNativeBindingFirstIndex+slot,&raw);
            if(error || !raw){trace.value.error=static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);continue;}
            StreamRef stream(suites.stream,raw);AEGP_StreamType type{};A_Boolean enabled=FALSE;
            error=suites.stream->AEGP_GetStreamType(raw,&type);
            if(!error && type==AEGP_StreamType_OneD)error=suites.stream->AEGP_GetExpressionState(id,raw,&enabled);
            if(error || type!=AEGP_StreamType_OneD || !enabled){trace.value.error=static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);continue;}
            AEGP_StreamValue2 value{};
            error=suites.stream->AEGP_GetNewStreamValue(id,raw,AEGP_LTimeMode_LayerTime,&warm_time,FALSE,&value);
            if(error){trace.value.error=static_cast<PF_Err>(error);continue;}
            const double number=value.val.one_d;
            suites.stream->AEGP_DisposeStreamValue(&value);
            if(!std::isfinite(number) || number==kNativeBindingUnavailable){trace.value.error=PF_Err_BAD_CALLBACK_PARAM;continue;}
            evaluated[slot]=true;++trace.value.evaluated;
        }
    }
    std::vector<NativeControlProof> result;
    for(A_long i=0;i<count;++i) {
        AEGP_EffectRefH ref{};if(suites.effect->AEGP_GetLayerEffectByIndex(id,layer,i,&ref) || !ref)return;
        EffectRef effect(suites.effect,ref);AEGP_InstalledEffectKey key{};char name[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
        if(suites.effect->AEGP_GetInstalledKeyFromLayerEffect(ref,&key) || suites.effect->AEGP_GetEffectMatchName(key,name))return;
        Kind kind{};const char* type{};std::uint16_t schema{};
        if(!decode_node_kind(name,kind,type,schema))continue;
        core::Uuid128 uuid{};const A_Time zero{0,1};
        if(!read_uuid(suites,id,ref,native_nodes::uuid_first_index(kind),zero,uuid))continue;
        const auto node=std::find_if(nodes.begin(),nodes.end(),[&](const auto& n){return n.id==core::NodeId{uuid} && n.kind==kind;});
        if(node==nodes.end())continue;
        std::uint32_t inherited_resource=1;
        const bool identity_resource=kind==Kind::transform &&
            node->fields[transform_layout::inherit].present && node->fields[transform_layout::inherit].value[0]==0 &&
            read_layer_resource(suites,id,ref,zero,inherited_resource) && inherited_resource==0;
        for(A_long index=1;index<static_cast<A_long>(node->fields.size());++index) {
            const auto& field=node->fields[index];if(!field.present || field.slot<0)continue;
            std::array<PF_State,3> before{};bool eligible=true;
            const auto first=kNativeBindingFirstIndex+field.slot;
            for(A_long c=0;c<component_count(field.type);++c)
                if(!evaluated[field.slot+c] || extra.utils->PF_GetCurrentState(data->effect_ref,first+c,nullptr,nullptr,&before[c]))eligible=false;
            if(!eligible)continue;
            if(kind==Kind::transform && transform_layout::matrix_field(index)) {
                // Synthetic affine fields have no native numeric stream. A
                // selected layer can animate through its ancestors even when
                // the Transform controls have no keys; retain exact sampling.
                if(!identity_resource)continue;
                PF_State after{};A_Boolean same=FALSE;
                if(!extra.utils->PF_GetCurrentState(data->effect_ref,first,nullptr,nullptr,&after) &&
                   !extra.utils->PF_AreStatesIdentical(data->effect_ref,&before[0],&after,&same) && same)
                    result.push_back(NativeControlProof{node->id,first,after,true,{},{}});
                continue;
            }
            AEGP_StreamRefH raw{};if(suites.stream->AEGP_GetNewEffectStreamByIndex(id,ref,index,&raw) || !raw)continue;
            StreamRef stream(suites.stream,raw);A_Boolean can_vary=FALSE,expression=FALSE;A_long keys=0;
            if(suites.stream->AEGP_CanVaryOverTime(raw,&can_vary))continue;
            // AE rejects expression queries on CANNOT_TIME_VARY streams.
            if(can_vary && (suites.stream->AEGP_GetExpressionState(id,raw,&expression) ||
                extra.keys->AEGP_GetStreamNumKFs(raw,&keys) || keys<0))continue;
            if(expression)continue;
            const bool constant=keys==0;
            std::optional<core::EmissionRateProfile> profile;
            const bool scalar_curve=(kind==Kind::emitter && index==3) || (kind==Kind::particle && index==2);
            if(scalar_curve && keys<=4096) {
                core::EmissionRateProfile values;bool ok=true;
                if(!keys) {
                    AEGP_StreamValue2 value{};
                    if(suites.stream->AEGP_GetNewStreamValue(id,raw,AEGP_LTimeMode_LayerTime,&zero,TRUE,&value))ok=false;
                    else {values.constant=value.val.one_d;suites.stream->AEGP_DisposeStreamValue(&value);}
                } else if(!extra.keys->AEGP_GetKeyframeTime || !extra.keys->AEGP_GetNewKeyframeValue || !extra.keys->AEGP_GetKeyframeInterpolation)ok=false;
                else {
                    std::vector<AEGP_KeyframeInterpolationType> incoming;
                    for(A_long k=0;k<keys && ok;++k) {
                        A_Time time{};AEGP_StreamValue2 value{};AEGP_KeyframeInterpolationType in{},out{};
                        if(extra.keys->AEGP_GetKeyframeTime(raw,k,AEGP_LTimeMode_LayerTime,&time) || time.scale<=0 ||
                            extra.keys->AEGP_GetKeyframeInterpolation(raw,k,&in,&out) ||
                            extra.keys->AEGP_GetNewKeyframeValue(id,raw,k,&value)) {ok=false;break;}
                        const double number=value.val.one_d;suites.stream->AEGP_DisposeStreamValue(&value);
                        if(k<keys-1 && out!=AEGP_KeyInterp_LINEAR && out!=AEGP_KeyInterp_HOLD) {ok=false;break;}
                        values.keys.push_back({double(time.value)/time.scale,number,out==AEGP_KeyInterp_HOLD?core::RateInterpolation::hold:core::RateInterpolation::linear});
                        incoming.push_back(in);
                    }
                    for(std::size_t k=1;k<values.keys.size() && ok;++k)
                        if(values.keys[k-1].outgoing==core::RateInterpolation::linear && incoming[k]!=AEGP_KeyInterp_LINEAR)ok=false;
                }
                core::EmissionTimeline validator;
                if(ok && validator.configure(30,&values).has_value()) profile=std::move(values);
            }
            if(!constant && !profile)continue;
            for(A_long c=0;c<component_count(field.type);++c) {
                PF_State after{};A_Boolean same=FALSE;
                if(extra.utils->PF_GetCurrentState(data->effect_ref,first+c,nullptr,nullptr,&after) ||
                    extra.utils->PF_AreStatesIdentical(data->effect_ref,&before[c],&after,&same) || !same)continue;
                NativeControlProof proof{node->id,first+c,after,constant,{},{}};
                if(profile && kind==Kind::emitter)proof.rate=profile;
                if(profile && kind==Kind::particle) {
                    double maximum=profile->constant;for(const auto& p:profile->keys)maximum=std::max(maximum,p.value);
                    if(maximum<=core::kMaxLifetimeSeconds)proof.life_bound=maximum;
                }
                result.push_back(std::move(proof));
            }
        }
    }
    trace.value.proofs=result.size();
    remember_native_control_proofs(data,std::move(result));
} catch(...) {}
} // namespace starfield::adapter
