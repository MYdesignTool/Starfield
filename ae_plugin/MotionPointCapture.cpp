#include "MotionPointCapture.hpp"
#include "TransformBinding.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <new>

namespace starfield::adapter {
namespace {
struct Suites {
    SPBasicSuite* basic;
    const AEGP_PFInterfaceSuite1* pf{};
    const AEGP_LayerSuite9* layer{};
    bool pf_acquired{},layer_acquired{};
    A_Err close() noexcept {
        A_Err first{};
        if(layer_acquired){layer_acquired=false;first=basic->ReleaseSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);}
        if(pf_acquired){pf_acquired=false;const auto error=basic->ReleaseSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);if(!first)first=error;}
        return first;
    }
    ~Suites(){(void)close();}
};
bool finite(core::Vec3 p) noexcept {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
bool bounded(core::Vec3 p) noexcept {
    return finite(p) && std::abs(p.x)<=1e9 && std::abs(p.y)<=1e9 && std::abs(p.z)<=1e9;
}
transform_binding::Matrix column_matrix(const A_Matrix4& source) noexcept {
    transform_binding::Matrix result{};
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)result[r*4+c]=source.mat[c][r];
    return result;
}
}
PF_Err capture_motion_layer_points(PF_InData* data,core::LayerUnits units,
    std::span<const MotionLayerPointRequest> requests,const core::Cancellation& cancellation,
    std::vector<MotionLayerPoint>& output) noexcept {
    if(!data || !data->effect_ref || !data->time_scale || !data->pica_basicP ||
       !data->pica_basicP->AcquireSuite || !data->pica_basicP->ReleaseSuite ||
       !std::isfinite(units.layer_width) || units.layer_width<=0 ||
       !std::isfinite(units.layer_height) || units.layer_height<=0 ||
       !std::isfinite(units.pixel_aspect_ratio) || units.pixel_aspect_ratio<=0 || requests.size()>kMaxMotionPointRequests)
        return PF_Err_BAD_CALLBACK_PARAM;
    for(const auto& request:requests)
        if(!request.layer_id || request.layer_id>0x7fffffff || !bounded(request.local_pixels))return PF_Err_BAD_CALLBACK_PARAM;
    if(cancellation.is_cancelled())return PF_Interrupt_CANCEL;
    if(requests.empty()){std::vector<MotionLayerPoint>{}.swap(output);return PF_Err_NONE;}
    try {
        Suites suites{data->pica_basicP};
        auto error=suites.basic->AcquireSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,reinterpret_cast<const void**>(&suites.pf));
        if(error)return static_cast<PF_Err>(error);
        suites.pf_acquired=true;
        if(!suites.pf || !suites.pf->AEGP_ConvertEffectToCompTime || !suites.pf->AEGP_GetEffectLayer)return PF_Err_BAD_CALLBACK_PARAM;
        error=suites.basic->AcquireSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,reinterpret_cast<const void**>(&suites.layer));
        if(error)return static_cast<PF_Err>(error);
        suites.layer_acquired=true;
        if(!suites.layer || !suites.layer->AEGP_GetLayerParentComp || !suites.layer->AEGP_GetLayerFromLayerID ||
           !suites.layer->AEGP_GetLayerID || !suites.layer->AEGP_GetLayerToWorldXform)return PF_Err_BAD_CALLBACK_PARAM;
        A_Time time{};AEGP_LayerH owner{};AEGP_CompH comp{};A_Matrix4 owner_world{};
        error=suites.pf->AEGP_ConvertEffectToCompTime(data->effect_ref,data->current_time,data->time_scale,&time);
        if(error)return static_cast<PF_Err>(error);
        if(!time.scale)return PF_Err_BAD_CALLBACK_PARAM;
        error=suites.pf->AEGP_GetEffectLayer(data->effect_ref,&owner);
        if(error)return static_cast<PF_Err>(error);
        if(!owner)return PF_Err_BAD_CALLBACK_PARAM;
        error=suites.layer->AEGP_GetLayerParentComp(owner,&comp);
        if(error)return static_cast<PF_Err>(error);
        if(!comp)return PF_Err_BAD_CALLBACK_PARAM;
        error=suites.layer->AEGP_GetLayerToWorldXform(owner,&time,&owner_world);
        if(error)return static_cast<PF_Err>(error);
        const auto owner_matrix=column_matrix(owner_world);
        transform_binding::Matrix identity{};identity[0]=identity[5]=identity[10]=identity[15]=1;
        const auto owner_check=transform_binding::relative_anchor_affine(identity,owner_matrix,{});
        if(!owner_check.has_value())return PF_Err_BAD_CALLBACK_PARAM;
        struct CachedSource {std::uint32_t id;transform_binding::PixelAffine relative;};
        std::array<CachedSource,kMaxMotionPointRequests> cache{};std::size_t cached{};
        std::vector<MotionLayerPoint> result;result.reserve(requests.size());
        for(const auto& request:requests) {
            if(cancellation.is_cancelled())return PF_Interrupt_CANCEL;
            auto found=std::find_if(cache.begin(),cache.begin()+cached,[&](const auto& value){return value.id==request.layer_id;});
            if(found==cache.begin()+cached){
                AEGP_LayerH source{};A_Matrix4 source_world{};
                error=suites.layer->AEGP_GetLayerFromLayerID(comp,static_cast<AEGP_LayerIDVal>(request.layer_id),&source);
                if(error)return static_cast<PF_Err>(error);
                if(!source)return PF_Err_BAD_CALLBACK_PARAM;
                AEGP_LayerIDVal actual{};AEGP_CompH source_comp{};
                error=suites.layer->AEGP_GetLayerID(source,&actual);
                if(!error)error=suites.layer->AEGP_GetLayerParentComp(source,&source_comp);
                if(error)return static_cast<PF_Err>(error);
                if(actual!=static_cast<AEGP_LayerIDVal>(request.layer_id) || source_comp!=comp)return PF_Err_BAD_CALLBACK_PARAM;
                if(source==owner)source_world=owner_world;
                else {error=suites.layer->AEGP_GetLayerToWorldXform(source,&time,&source_world);if(error)return static_cast<PF_Err>(error);}
                if(source==owner)cache[cached++]={request.layer_id,{1,0,0,0,0,1,0,0,0,0,1,0}};
                else {
                    auto relative=transform_binding::relative_anchor_affine(column_matrix(source_world),owner_matrix,{});
                    if(!relative.has_value())return PF_Err_BAD_CALLBACK_PARAM;
                    cache[cached++]={request.layer_id,relative.value()};
                }
            }
            const auto& matrix=found->relative;const auto p=request.local_pixels;
            const core::Vec3 pixel{matrix[0]*p.x+matrix[1]*p.y+matrix[2]*p.z+matrix[3],
                matrix[4]*p.x+matrix[5]*p.y+matrix[6]*p.z+matrix[7],matrix[8]*p.x+matrix[9]*p.y+matrix[10]*p.z+matrix[11]};
            if(!finite(pixel))return PF_Err_BAD_CALLBACK_PARAM;
            const auto position=core::layer_point_to_world(pixel.x,pixel.y,pixel.z,units);
            if(!bounded(position))return PF_Err_BAD_CALLBACK_PARAM;
            result.push_back({request.layer_id,position});
        }
        if(cancellation.is_cancelled())return PF_Interrupt_CANCEL;
        error=suites.close();if(error)return static_cast<PF_Err>(error);
        if(cancellation.is_cancelled())return PF_Interrupt_CANCEL;
        output.swap(result);return PF_Err_NONE;
    }catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}
    catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
}
