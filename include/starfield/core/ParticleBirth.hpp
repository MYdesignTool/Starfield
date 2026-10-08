#pragma once

#include "starfield/core/Random.hpp"
#include "starfield/core/Settings.hpp"
#include <cmath>
#include <cstdint>

namespace starfield::core {

[[nodiscard]] inline bool valid_particle_birth_controls(const ParticleBirthControls& controls) noexcept {
    return std::isfinite(controls.chance_percent) && controls.chance_percent>=0 && controls.chance_percent<=100;
}

// Unsigned addition defines wrap even at signed extremes. Apply only after
// validating the public emitter Settings; internal RNG seeds span all32 bits.
[[nodiscard]] constexpr std::uint32_t shifted_particle_seed(std::uint32_t emitter_seed,
    std::int32_t shift) noexcept {
    return emitter_seed+static_cast<std::uint32_t>(shift);
}

// The caller supplies an emitter-scoped ORIGINAL birth ordinal identity. It
// excludes the Particle UUID, frame time and count of accepted particles.
// Validate controls before entering an evaluator; malformed input is not an
// authored zero-chance branch. Endpoints need no random lookup.
[[nodiscard]] inline bool particle_birth_allowed(const ParticleBirthControls& controls,
    std::uint32_t emitter_seed,std::uint64_t source_birth_identity) noexcept {
    if(!valid_particle_birth_controls(controls) || controls.chance_percent==0)return false;
    if(controls.chance_percent==100)return true;
    return unit_value(shifted_particle_seed(emitter_seed,controls.seed_shift),source_birth_identity,
        RandomPurpose::particle_birth_chance)<controls.chance_percent/100;
}

} // namespace starfield::core
