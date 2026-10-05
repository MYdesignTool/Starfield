#pragma once
#include <cstdint>
#include "AEConfig.h"
#include "AE_Effect.h"
#include "starfield/core/PluginApi.h"

namespace starfield::adapter {
struct GpuExecutionInfo { bool rendered{}; PF_GPU_Framework framework{}; A_u_long device_index{}; };
struct GpuSetupTiming {std::uint64_t calls{};double last_ms{},max_ms{};};
[[nodiscard]] GpuSetupTiming last_gpu_setup_timing() noexcept;
[[nodiscard]] GpuExecutionInfo last_gpu_execution() noexcept;
void record_cpu_execution() noexcept;
[[nodiscard]] PF_Err gpu_device_setup(PF_InData*,PF_OutData*,PF_GPUDeviceSetupExtra*) noexcept;
[[nodiscard]] PF_Err gpu_device_setdown(PF_InData*,PF_GPUDeviceSetdownExtra*) noexcept;
[[nodiscard]] bool gpu_device_matches(const void*,PF_GPU_Framework,A_u_long) noexcept;
// Never reads or writes PF_LayerDef::data. GPU worlds are borrowed from AE.
[[nodiscard]] PF_Err render_gpu_scene(PF_InData*,PF_OutData*,const void*,PF_GPU_Framework,
    A_u_long,const SfCoreGpuSceneResult&,PF_EffectWorld*,std::int32_t world_left,std::int32_t world_top,bool straight,unsigned samples=1,float gain=1) noexcept;
[[nodiscard]] PF_Err copy_gpu_pixels(PF_InData*,const void*,PF_GPU_Framework,A_u_long,
    PF_EffectWorld* input,PF_EffectWorld* output) noexcept;
}
