#pragma once
#include "ModelMirrorParameter.hpp"
#include "AE_GeneralPlug.h"
#include <memory>

namespace starfield::adapter {
// UI only. Owns each borrowed stream/value until acceptance or verified rollback.
class ModelMirrorTransaction {
public:
    ModelMirrorTransaction(PF_InData*,AEGP_PluginID,AEGP_EffectRefH,AEGP_LayerH);
    ~ModelMirrorTransaction();
    ModelMirrorTransaction(const ModelMirrorTransaction&)=delete;
    ModelMirrorTransaction& operator=(const ModelMirrorTransaction&)=delete;
    [[nodiscard]] PF_Err install(const core::Graph&,A_long* failed_stream=nullptr,const char** failed_stage=nullptr) noexcept;
    [[nodiscard]] PF_Err rollback() noexcept;
    void accept() noexcept;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
