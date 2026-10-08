#pragma once
#include "Parameters.hpp"
#include "WorldBridge.hpp"
#include "starfield/core/Graph.hpp"
#include "starfield/core/PluginApi.h"
#include "starfield/core/Render.hpp"
#include <array>
#include <vector>

namespace starfield::adapter {
inline constexpr std::size_t kMotionByteBudget=512u*1024u*1024u;
inline constexpr std::array<A_long,8> kMotionParameterIds{611,612,613,614,615,616,617,618};
inline constexpr std::array<core::ParameterKey,8> kMotionParameterKeys{
    core::graph_keys::kMotionBlur,core::graph_keys::kShutterAngle,core::graph_keys::kShutterPhase,core::graph_keys::kMotionBlurType,
    core::graph_keys::kMotionBlurLevels,core::graph_keys::kLinearAccuracy,core::graph_keys::kOpacityBoost,core::graph_keys::kMotionBlurDisregard};
inline constexpr std::array<double,8> kMotionDefaults{1,360,0,0,8,70,0,0};
inline constexpr std::array<double,8> kMotionMinimum{0,0,-720,0,2,1,0,0};
inline constexpr std::array<double,8> kMotionMaximum{2,720,720,1,64,100,1000,1};
inline constexpr bool motion_popup(std::size_t i){return i==0 || i==3 || i==7;}
[[nodiscard]] PF_Err append_motion_parameters(PF_InData*,PF_OutData*) noexcept;
[[nodiscard]] PF_Err update_motion_ui(PF_InData*,PF_ParamDef*[]) noexcept;
[[nodiscard]] inline PF_Err append_motion_values(PF_ParamDef* params[],core::GraphNode& node) {
    for(std::size_t i=0;i<8;++i) {
        const auto* p=params[kMotionParameterIds[i]];if(!p)return PF_Err_BAD_CALLBACK_PARAM;
        if(p->param_type!=(motion_popup(i)?PF_Param_POPUP:PF_Param_FLOAT_SLIDER))return PF_Err_BAD_CALLBACK_PARAM;
        const double value=motion_popup(i)?double(p->u.pd.value-1):p->u.fs_d.value;
        if(!(value>=kMotionMinimum[i] && value<=kMotionMaximum[i]))return PF_Err_BAD_CALLBACK_PARAM;
        node.parameters.push_back({kMotionParameterKeys[i],motion_popup(i)?core::ParameterValue{std::uint32_t(value)}:core::ParameterValue{value}});
    }
    return PF_Err_NONE;
}
struct MotionSample {core::RationalTime time;core::OpaqueBytes graph;SfCoreRenderRequest camera{};};
struct MotionExposure {
    std::array<double,8> values{};
    bool enabled{};
    std::vector<MotionSample> samples;
    double gain{1};
};
[[nodiscard]] PF_Err prepare_motion_exposure(PF_InData*,PF_OutData*,const core::Graph&,
    A_long width,A_long height,const core::Cancellation&,MotionExposure&) noexcept;
[[nodiscard]] PF_Err render_motion_cpu(PF_InData*,PF_OutData*,const SfCoreApi&,SfCoreRenderRequest,
    const MotionExposure&,const WorldLayout&,PF_EffectWorld*,HostBitDepth,const core::Cancellation&) noexcept;
struct MotionGpuStorage {
    std::vector<SfGpuSprite> sprites;
    std::vector<SfGpuCloudCircle> cloud_circles;
    std::vector<std::uint32_t> offsets,indices;
};
[[nodiscard]] PF_Err prepare_motion_gpu(PF_InData*,PF_OutData*,const SfCoreApi&,SfCoreRenderRequest,
    const MotionExposure&,SfCoreGpuSceneResult&,MotionGpuStorage&,const core::Cancellation&) noexcept;
void apply_motion_sample(SfCoreRenderRequest&,const MotionSample&) noexcept;
}
