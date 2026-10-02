#pragma once
#include "starfield/core/Error.hpp"
#include "starfield/core/Graph.hpp"
#include "starfield/core/Settings.hpp"

namespace starfield::core {
inline constexpr std::size_t kMaxEmitterOriginSamples = 1'000'000;
struct EmitterOriginSample { NodeId emitter; double birth_seconds{}; Vec3 origin; };
class EmitterOriginSampler {
public:
    virtual ~EmitterOriginSampler() = default;
    [[nodiscard]] virtual Result<Vec3> sample(NodeId emitter, double birth_seconds) = 0;
};
class EmitterOriginHistory final : public EmitterOriginSampler {
public:
    bool present{};
    std::vector<EmitterOriginSample> samples;
    [[nodiscard]] Result<Vec3> sample(NodeId emitter, double birth_seconds) override;
};
[[nodiscard]] Result<EmitterOriginHistory> decode_emitter_origin_history(const Graph&);
[[nodiscard]] Result<OpaqueBytes> encode_emitter_origin_history(std::vector<EmitterOriginSample>);
} // namespace starfield::core
