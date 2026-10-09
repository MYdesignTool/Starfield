#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace starfield::core {

// IDs are serialized into the AE parameter layout. Once exposed to users, never
// renumber or reuse an ID; add new IDs and migrate old sequence data instead.
enum class ParameterId : std::uint32_t {
    particle_count = 1,
    birth_rate = 2,
    seed = 3,
    particle_lifetime = 4,
    emitter_shape = 5,
    emitter_origin = 6,
    velocity_x = 7,
    velocity_y = 8,
    velocity_z = 9,
    size = 10,
    opacity = 11,
    emitter_size = 12,
    velocity_spread = 13,
    graph_data = 14,
    control_source = 15,
    capture_controls = 16,
    emitter_size_x = 17,
    emitter_size_y = 18,
    emitter_size_z = 19,
    particle_size_random = 20,
    opacity_random = 21,
};

enum class EmitterShape : std::uint8_t {
    point,
    box,
    sphere,
    disc,
};

// Bounds are owned by schema/parameters.json and enforced by validate_settings.
// Later stages (simulation, rasterizer) may rely on them without re-checking.
inline constexpr std::uint32_t kDefaultParticleCount = 1'000'000;
inline constexpr std::uint32_t kMaxParticleCount = 2'000'000;
inline constexpr double kMaxBirthRate = 1'000'000.0;
inline constexpr double kMaxLifetimeSeconds = 10'000.0;
inline constexpr double kMaxParticleSize = 100'000.0;
inline constexpr double kMaxParticleRandomPercent = 100.0;
inline constexpr std::uint32_t kMaxSeed = 2'147'483'647;
inline constexpr std::uint32_t kEmitterShapeCount = 4;
// World-space movement bounds, in layer heights and layer heights per second.
inline constexpr double kMaxEmitterOffset = 100.0;
inline constexpr double kMaxVelocity = 1'000.0;
inline constexpr double kMaxEmitterSize = 10.0;
inline constexpr double kMaxEmitterSizePixels = 100'000.0;
inline constexpr double kMaxVelocitySpread = 100.0;
inline constexpr double kMaxGravityMagnitude = 100'000.0;
inline constexpr double kMaxLinearDrag = 100.0;
inline constexpr double kMaxParticleColor = 64.0;
enum class ParticleTransferMode : std::uint32_t { normal=0, add=1, screen=2, stencil=3 };
inline constexpr std::uint32_t kMaxCloudCircles = 1000;
inline constexpr std::size_t kMaxCloudStyles = 4096;
inline constexpr std::size_t kMaxCloudMembers = 2'000'000;
struct ParticleCloudStyle {
    std::uint32_t circles{10};
    double aspect{150};
    double density{66};
};
struct ParticleBirthControls {
    std::int32_t seed_shift{};
    double chance_percent{100};
};
inline constexpr double kMaxEmissionSpeed = 100'000.0;
inline constexpr double kMaxEmissionAngleDegrees = 100'000.0;
inline constexpr double kMaxDirectionSpanDegrees = 180.0;

struct Vec3 {
    double x{};
    double y{};
    double z{};
};

// Model geometry is plain numeric staging. Its render/graph wire integration
// is staged in ADR0038; these values contain no host references or file paths.
inline constexpr std::uint32_t kMissingModelAttribute = 0xffffffffu;
inline constexpr std::uint32_t kMaxModelVertices = 65536;
inline constexpr std::uint32_t kMaxModelTriangles = 65536;
inline constexpr double kMaxModelCoordinate = 1e9;
struct ModelPosition { Vec3 value{}; double weight{1}; };
struct ModelCorner {
    std::uint32_t position{};
    std::uint32_t texture{kMissingModelAttribute};
    std::uint32_t normal{kMissingModelAttribute};
};
struct ModelTriangle { std::array<ModelCorner,3> corners{}; };
struct ModelBounds { Vec3 minimum{}, maximum{}; };
struct ModelGeometry {
    std::vector<ModelPosition> positions;
    std::vector<Vec3> texture_coordinates;
    std::vector<Vec3> normals;
    std::vector<ModelTriangle> triangles;
    ModelBounds bounds{};
};

// Transform node values use the same canonical world frame as particles.
// The adapter converts AE pixels/signs and captures any inherited motion before
// entering the core. Matrices are row-major, applied to column vectors.
struct ParticleTransformSettings {
    Vec3 anchor{};
    Vec3 position{};
    Vec3 rotation_degrees{};
    Vec3 scale_percent{100.0, 100.0, 100.0};
    double particles_scale_percent{100.0};
    double particles_opacity_percent{100.0};
    std::array<double, 16> inherited_motion{
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
};

// Converts full-resolution layer-pixel emitter dimensions to canonical world
// coordinates (one world unit equals one layer height). X accounts for pixel aspect.
struct EmitterDimensionContext {
    double layer_height_pixels{1.0};
    double pixel_aspect_ratio{1.0};
};

inline constexpr std::size_t kMaxAgeCurvePoints = 64;
enum class CurveInterpolation : std::uint8_t { linear, hold, bezier, draw };

struct AgeCurvePoint {
    double age{};
    double value{};
};

// A zero point count uses a straight 100%-to-end-percentage fallback.
// Custom curves use normalized particle age (0..1) and fixed bounded storage.
struct AgeCurve {
    std::array<AgeCurvePoint, kMaxAgeCurvePoints> points{};
    std::uint8_t count{};
    CurveInterpolation interpolation{CurveInterpolation::linear};
};

// How the emission direction is sampled. `directional` uses the Euler angles and the
// cone span; `uniform` samples the whole sphere and ignores both.
enum class DirectionMode : std::uint8_t { directional = 0, uniform = 1 };

struct ForceMotion {
    // gravity is already included in Settings::gravity; keep its identity here
    // only to apply independent per-Force random attenuation.
    Vec3 gravity{};
    double gravity_random_percent{};
    Vec3 wind{};
    double spin_radius{};
    double spin_frequency{};
    double spin_resist{};
    double spin_delay{};
    AgeCurve wind_spin_curve{};
    std::uint32_t random_salt{};
    Vec3 spin_axis_x{1,0,0},spin_axis_y{0,1,0};
};

struct Settings {
    std::uint32_t particle_count{kDefaultParticleCount};
    double birth_rate{30.0};
    std::uint32_t seed{1};
    double particle_lifetime_seconds{2.0};
    EmitterShape emitter_shape{EmitterShape::point};
    // World space for all three: origin at the layer centre, +X right, +Y up,
    // +Z toward the viewer, one unit = one layer height (ADR 0003). The host's
    // "% of layer" point control is converted by the adapter.
    Vec3 emitter_origin{};
    // Layer heights per second; the default keeps a fresh instance visibly alive.
    Vec3 velocity{0.0, 0.3, 0.0};
    double particle_size{10.0};
    double opacity{1.0};
    // Per-particle attenuation after age curves. Zero keeps the exact base value;
    // 100% can attenuate it toward zero using the particle's stable random stream.
    double particle_size_random_percent{0.0};
    double opacity_random_percent{0.0};
    // Diameter/edge of the legacy Disc emitter in layer heights. Box and Sphere use
    // the direct per-axis dimensions below; Point ignores both fields.
    double emitter_size{0.05};
    // Direct full-resolution layer-pixel dimensions used by Box and Sphere. Other
    // emitter shapes can adopt the same vector when their geometry is implemented.
    Vec3 emitter_size_pixels{100.0, 100.0, 100.0};
    // Per-axis uniform jitter applied to each particle's velocity, in layer heights
    // per second. This is what makes a steady emitter animate: identical particles
    // produce a stationary pattern, varied ones produce visible motion.
    double velocity_spread{0.15};
    // Constant acceleration in layer-heights per second squared and linear drag
    // rate in inverse seconds. Zero preserves the M2 straight-line trajectory.
    Vec3 gravity{};
    double linear_drag{0.0};
    std::vector<ForceMotion> forces{};
    // Linear age curves. `particle_size` and `opacity` are the birth values;
    // the curve ordinates are percentages of these base values. Color is stored
    // as three working-space channel values; alpha is controlled by opacity.
    Vec3 color_start{1.0, 1.0, 1.0};
    Vec3 color_end{1.0, 1.0, 1.0};
    double particle_size_end{100.0};
    double opacity_end{100.0};
    AgeCurve size_over_life{};
    AgeCurve opacity_over_life{};
    bool appearance_enabled{false};
    // Emission direction model, aligned with the reference emitter's controls: particles
    // leave at `emission_speed` along an axis built from the Euler angles, sampled inside
    // a cone of `direction_span_degrees`. Angles are degrees, applied as rotations of +Y
    // about X, then Y, then Z, so 0/0/0 points straight up (the reference's default look).
    // `uniform` ignores the axis and samples the whole sphere; a span of 180 or more does
    // the same. `emission_speed_random` jitters the speed per particle in the same unit.
    double emission_speed{0.0};
    double emission_speed_random{0.0};
    Vec3 emission_angles_degrees{};
    Vec3 emitter_shape_angles_degrees{};
    DirectionMode direction_mode{DirectionMode::uniform};
    double direction_span_degrees{60.0};
};

// Host popup controls are one-based while the core enum is zero-based. Out-of-range
// indices fall back to `point`; see docs/parameter-mapping.md for the mapping table.
[[nodiscard]] EmitterShape emitter_shape_from_index(std::uint32_t index) noexcept;

enum class ValidationCode : std::uint8_t {
    particle_count_clamped,
    birth_rate_clamped,
    seed_clamped,
    lifetime_clamped,
    size_clamped,
    opacity_clamped,
    particle_size_random_clamped,
    opacity_random_clamped,
    emitter_shape_replaced,
    emitter_origin_clamped,
    velocity_clamped,
    emitter_size_clamped,
    emitter_size_pixels_clamped,
    velocity_spread_clamped,
    gravity_clamped,
    linear_drag_clamped,
    color_clamped,
    end_size_clamped,
    end_opacity_clamped,
    emission_speed_clamped,
    emission_speed_random_clamped,
    emission_angle_clamped,
    direction_span_clamped,
    direction_mode_replaced,
    age_curve_invalid,
    force_motion_invalid,
    non_finite_replaced,
};

struct ValidationNotice {
    ValidationCode code{};
    std::string field;
};

struct ValidatedSettings {
    Settings value;
    std::vector<ValidationNotice> notices;
};

// Converts untrusted host/UI values into bounded, finite render input.
// This function is pure and safe to call independently for every frame.
[[nodiscard]] ValidatedSettings validate_settings(Settings settings);

} // namespace starfield::core
