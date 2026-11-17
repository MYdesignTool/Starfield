#pragma once
#include "AE_Effect.h"
#include "AE_EffectSuites.h"
#include "AE_GeneralPlug.h"
#include "NodeRecord.hpp"
#include "SPBasic.h"
#include "UiExclusionClient.hpp"
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace starfield::adapter {
// Read-only, synchronous UI request. Never interprets generic params[] or
// borrows a PF/AEGP handle beyond this call. An absent PF context is optional.
struct NativeSourceStateRequest {
    std::uint32_t magic{0x53535031},bytes{sizeof(NativeSourceStateRequest)},version{1};
    std::uint32_t kind{},compare{},acknowledged{},supported{},same{};
    std::uint32_t reason{}; // 1wire,2scope/context,3kind,4suites,5effect,6stream,7API,8state,9compare
    AEGP_PluginID plugin{};std::int32_t identity_stream{};
    PF_State previous{},current{};
};
static_assert(std::is_trivially_copyable_v<NativeSourceStateRequest> && std::is_standard_layout_v<NativeSourceStateRequest>);
inline PF_Err read_native_source_state(PF_InData* data,std::uint32_t kind,NativeSourceStateRequest& request) noexcept {
    request.acknowledged=1;request.supported=request.same=0;
    request.reason=1;
    if(request.magic!=0x53535031 || request.bytes!=sizeof(request) || request.version!=1 || request.kind!=kind || request.compare>1)return PF_Err_NONE;
    request.reason=2;
    if(
       !request.plugin || !data || !data->effect_ref || !data->pica_basicP || !source_ui_transaction_active())return PF_Err_NONE;
    const char* expected=kind==0?"org.starfieldfx.node.emitter":kind==1?"org.starfieldfx.node.particle":
        kind==3?"org.starfieldfx.node.force":kind==8?"org.starfieldfx.node.turbulence":nullptr;
    request.reason=3;if(!expected)return PF_Err_NONE;
    const AEGP_PFInterfaceSuite1* pf{};const AEGP_EffectSuite4* effects{};const AEGP_StreamSuite6* streams{};
    struct Context {
        SPBasicSuite* basic;const AEGP_PFInterfaceSuite1*& pf;const AEGP_EffectSuite4*& effects;const AEGP_StreamSuite6*& streams;
        AEGP_EffectRefH effect{};AEGP_StreamRefH stream{};
        ~Context(){if(stream)streams->AEGP_DisposeStream(stream);if(effect)effects->AEGP_DisposeEffect(effect);
            if(streams)basic->ReleaseSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
            if(effects)basic->ReleaseSuite(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
            if(pf)basic->ReleaseSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);}
    } context{data->pica_basicP,pf,effects,streams};
    request.reason=4;
    if(data->pica_basicP->AcquireSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,reinterpret_cast<const void**>(&pf)) || !pf ||
       data->pica_basicP->AcquireSuite(kAEGPEffectSuite,kAEGPEffectSuiteVersion4,reinterpret_cast<const void**>(&effects)) || !effects ||
       data->pica_basicP->AcquireSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6,reinterpret_cast<const void**>(&streams)) || !streams ||
       !streams->AEGP_GetUniqueStreamID || !pf->AEGP_GetNewEffectForEffect)return PF_Err_NONE;
    request.reason=5;if(pf->AEGP_GetNewEffectForEffect(request.plugin,data->effect_ref,&context.effect) || !context.effect)return PF_Err_NONE;
    AEGP_InstalledEffectKey key{};char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
    if(effects->AEGP_GetInstalledKeyFromLayerEffect(context.effect,&key) || effects->AEGP_GetEffectMatchName(key,match) ||
       std::strcmp(match,expected) || streams->AEGP_GetNewEffectStreamByIndex(request.plugin,context.effect,
           native_nodes::uuid_first_index(static_cast<native_nodes::Kind>(kind)),&context.stream) || !context.stream)return PF_Err_NONE;
    std::int32_t identity{};
    request.reason=6;if(streams->AEGP_GetUniqueStreamID(context.stream,&identity) || identity!=request.identity_stream)return PF_Err_NONE;
    const PF_ParamUtilsSuite3* suite{};
    const auto error=data->pica_basicP->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&suite));
    struct Release {SPBasicSuite* basic;bool acquired;~Release(){if(acquired)basic->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);}} release{data->pica_basicP,error==0};
    request.reason=7;if(error || !suite || !suite->PF_GetCurrentState || !suite->PF_AreStatesIdentical)return PF_Err_NONE;
    PF_State current{};
    request.reason=8;if(suite->PF_GetCurrentState(data->effect_ref,PF_ParamIndex_CHECK_ALL_EXCEPT_LAYER_PARAMS,nullptr,nullptr,&current))return PF_Err_NONE;
    A_Boolean same=FALSE;
    request.reason=9;if(request.compare && suite->PF_AreStatesIdentical(data->effect_ref,&request.previous,&current,&same))return PF_Err_NONE;
    request.current=current;request.same=same!=FALSE;request.supported=1;request.reason=0;return PF_Err_NONE;
}
}
