// Host-independent core self-tests. They cover the M2 acceptance criteria that can
// be checked without After Effects: rational-time normalization and overflow,
// settings validation, deterministic simulation across frame order, and the CPU
// renderer's geometry, region of interest, alpha, and format contracts.
//
// Build with tests/RunCoreTests.ps1 (MSVC) or CMake's `starfield_core_tests`.

#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/Geometry.hpp"
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/Random.hpp"
#include "starfield/core/Render.hpp"
#include "starfield/core/Settings.hpp"
#include "starfield/core/Time.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(condition)                                                                        \
    do {                                                                                        \
        ++g_checks;                                                                             \
        if (!(condition)) {                                                                     \
            ++g_failures;                                                                       \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                    \
        }                                                                                       \
    } while (0)

using namespace starfield::core;

constexpr double kMicroseconds = 1000000.0;

RationalTime time_from_seconds(double seconds) {
    const auto value = *make_rational(static_cast<std::int64_t>(std::llround(seconds * kMicroseconds)),
                                     static_cast<std::int64_t>(kMicroseconds));
    return value;
}

struct Scene {
    Settings settings{};
    std::uint32_t layer_width{64};
    std::uint32_t layer_height{64};
    std::uint32_t frame_width{64};
    std::uint32_t frame_height{64};
    RectI roi{0, 0, 64, 64};
    PixelFormat format{PixelFormat::rgba8};
    double pixel_aspect{1.0};
    double time_seconds{1.0};
    std::shared_ptr<const PixelBuffer> source;
};

RenderRequest build_request(const Scene& scene) {
    RenderRequest request;
    request.settings = validate_settings(scene.settings);

    FrameSpec& frame = request.frame;
    frame.layer_width = scene.layer_width;
    frame.layer_height = scene.layer_height;
    frame.frame_width = scene.frame_width;
    frame.frame_height = scene.frame_height;
    frame.region_of_interest = scene.roi;
    frame.time = time_from_seconds(scene.time_seconds);
    frame.frame_duration = *make_rational(1, 24);
    frame.format = scene.format;
    frame.color_space = ColorSpace::ae_working_space;
    frame.alpha_mode = AlphaMode::premultiplied;
    frame.pixel_aspect_ratio = scene.pixel_aspect;
    frame.quality = Quality::full;
    request.source = scene.source;
    return request;
}

struct Rgba8 {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{0};
};

Rgba8 pixel8(const RenderOutput& output, std::uint32_t x, std::uint32_t y) {
    const std::byte* pixel = output.pixels.data() + static_cast<std::size_t>(y) * output.row_bytes +
                             static_cast<std::size_t>(x) * 4;
    return Rgba8{std::to_integer<std::uint8_t>(pixel[0]), std::to_integer<std::uint8_t>(pixel[1]),
                 std::to_integer<std::uint8_t>(pixel[2]), std::to_integer<std::uint8_t>(pixel[3])};
}

std::uint16_t channel16(const RenderOutput& output, std::uint32_t x, std::uint32_t y,
                        std::uint32_t channel) {
    const std::byte* pixel = output.pixels.data() + static_cast<std::size_t>(y) * output.row_bytes +
                             static_cast<std::size_t>(x) * 8 + channel * 2;
    std::uint16_t value = 0;
    std::memcpy(&value, pixel, sizeof(value));
    return value;
}

float channel32(const RenderOutput& output, std::uint32_t x, std::uint32_t y, std::uint32_t channel) {
    const std::byte* pixel = output.pixels.data() + static_cast<std::size_t>(y) * output.row_bytes +
                             static_cast<std::size_t>(x) * 16 + channel * 4;
    float value = 0.0f;
    std::memcpy(&value, pixel, sizeof(value));
    return value;
}

PixelBuffer make_source(std::uint32_t width, std::uint32_t height, std::int32_t origin_x,
                        std::int32_t origin_y, Rgba8 color, AlphaMode alpha_mode) {
    PixelBuffer buffer;
    buffer.width = width;
    buffer.height = height;
    buffer.row_bytes = width * 4;
    buffer.format = PixelFormat::rgba8;
    buffer.alpha_mode = alpha_mode;
    buffer.origin_x = origin_x;
    buffer.origin_y = origin_y;
    buffer.pixels.resize(static_cast<std::size_t>(buffer.row_bytes) * height);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            std::byte* pixel = buffer.pixels.data() + static_cast<std::size_t>(y) * buffer.row_bytes +
                               static_cast<std::size_t>(x) * 4;
            pixel[0] = static_cast<std::byte>(color.r);
            pixel[1] = static_cast<std::byte>(color.g);
            pixel[2] = static_cast<std::byte>(color.b);
            pixel[3] = static_cast<std::byte>(color.a);
        }
    }
    return buffer;
}

class CancellingAfterFirstPoll final : public Cancellation {
public:
    explicit CancellingAfterFirstPoll(bool cancel) : cancel_(cancel) {}
    [[nodiscard]] bool is_cancelled() const noexcept override { return cancel_; }

private:
    bool cancel_{false};
};

void test_rational_time() {
    CHECK(make_rational(2, 4)->value == 1 && make_rational(2, 4)->scale == 2);
    CHECK(make_rational(-2, 4)->value == -1 && make_rational(-2, 4)->scale == 2);
    CHECK(make_rational(0, 5)->value == 0 && make_rational(0, 5)->scale == 1);
    CHECK(!make_rational(1, 0).has_value());

    const RationalTime half = *make_rational(1, 2);
    const RationalTime third = *make_rational(1, 3);
    CHECK(add(half, third)->value == 5 && add(half, third)->scale == 6);
    CHECK(subtract(half, third)->value == 1 && subtract(half, third)->scale == 6);
    CHECK(multiply(*make_rational(2, 3), *make_rational(3, 4))->value == 1);
    CHECK(compare(half, *make_rational(2, 4)) == 0);
    CHECK(compare(third, half) == -1);
    CHECK(compare(half, third) == 1);

    const RationalTime maximum{std::numeric_limits<std::int64_t>::max(), 1};
    const RationalTime one{1, 1};
    CHECK(!add(maximum, one).has_value());
    CHECK(!multiply(maximum, maximum).has_value());

    CHECK(is_negative(*make_rational(-1, 3)));
    CHECK(is_zero(*make_rational(0, 7)));
    CHECK(std::abs(to_seconds(*make_rational(1, 2)) - 0.5) < 1e-12);
    CHECK(std::abs(to_seconds(*make_rational(3, 8)) - 0.375) < 1e-12);
}

void test_layer_point_conversion() {
    // Documented host delivery: absolute layer pixels with the origin at the layer
    // top-left, x growing right and y growing down.
    const LayerUnits units{1920.0, 1080.0, 1.0};
    const Vec3 centre = layer_point_to_world(960.0, 540.0, 540.0, units);
    CHECK(std::abs(centre.x) < 1e-12);
    CHECK(std::abs(centre.y) < 1e-12);
    CHECK(std::abs(centre.z) < 1e-12);

    const Vec3 top_left = layer_point_to_world(0.0, 0.0, 0.0, units);
    CHECK(std::abs(top_left.x + (1920.0 / 1080.0) * 0.5) < 1e-12);
    CHECK(std::abs(top_left.y - 0.5) < 1e-12);
    CHECK(std::abs(top_left.z + 0.5) < 1e-12);

    // Anamorphic hosts: one layer width equals (width * aspect / height) layer heights.
    const LayerUnits anamorphic{720.0, 480.0, 2.0};
    const Vec3 right_edge = layer_point_to_world(720.0, 240.0, 240.0, anamorphic);
    CHECK(std::abs(right_edge.x - 1.5) < 1e-12);
    CHECK(std::abs(right_edge.y) < 1e-12);

    // Unit ladder: plausible pixels pass through (the documented convention), an
    // implausible magnitude falls back to the legacy percentage, and a fixed-point
    // scaled delivery is un-scaled first.
    CHECK(host_point_component_to_layer_pixels(960.0, 1920.0) == 960.0);
    CHECK(host_point_component_to_layer_pixels(50.0, 1920.0) == 50.0);
    CHECK(std::abs(host_point_component_to_layer_pixels(50.0, 10.0) - 5.0) < 1e-12);
    CHECK(std::abs(host_point_component_to_layer_pixels(50.0 * 65536.0, 1920.0) - 50.0) < 1e-9);
    CHECK(host_point_component_to_layer_pixels(-540.0, 1080.0) == -540.0);
}

void test_settings_validation() {
    Settings settings;
    settings.birth_rate = std::numeric_limits<double>::quiet_NaN();
    settings.particle_count = kMaxParticleCount + 10;
    settings.seed = kMaxSeed + 5;
    settings.opacity = 2.0;
    settings.particle_size = -1.0;
    settings.emitter_shape = static_cast<EmitterShape>(9);
    settings.emitter_origin = Vec3{1.0e6, -1.0e6, 0.0};
    settings.velocity = Vec3{1.0e9, 0.0, std::numeric_limits<double>::infinity()};

    const ValidatedSettings validated = validate_settings(settings);
    CHECK(std::isfinite(validated.value.birth_rate));
    CHECK(validated.value.birth_rate == 30.0); // documented default replaces non-finite input
    CHECK(validated.value.particle_count == kMaxParticleCount);
    CHECK(validated.value.seed == kMaxSeed);
    CHECK(validated.value.opacity == 1.0);
    CHECK(validated.value.particle_size == 0.0);
    CHECK(validated.value.emitter_shape == EmitterShape::point);
    CHECK(validated.value.emitter_origin.x == kMaxEmitterOffset);
    CHECK(validated.value.emitter_origin.y == -kMaxEmitterOffset);
    CHECK(validated.value.velocity.x == kMaxVelocity);
    CHECK(validated.value.velocity.z == 0.0); // non-finite replaced, then clamped
    CHECK(validated.notices.size() >= 8);
}

void test_simulation_emitter_origin() {
    const CancellingAfterFirstPoll never(false);

    Settings settings;
    settings.particle_count = 8; // population cap must not hide the travelled particle
    settings.birth_rate = 1.0;
    settings.particle_lifetime_seconds = 4.0;
    settings.emitter_origin = Vec3{0.25, -0.5, 0.125};
    settings.velocity = Vec3{0.0, 0.0, 0.0};
    settings.velocity_spread = 0.0; // isolate the closed-form origin math

    const auto stationary = simulate_particles(validate_settings(settings), 0.5, never);
    CHECK(stationary.has_value() && stationary.value().size() == 1);
    if (stationary.has_value() && stationary.value().size() == 1) {
        const ParticleInstance& particle = stationary.value().front();
        CHECK(std::abs(particle.age_seconds - 0.5) < 1e-12);
        CHECK(std::abs(particle.position.x - 0.25) < 1e-12);
        CHECK(std::abs(particle.position.y + 0.5) < 1e-12);
        CHECK(std::abs(particle.position.z - 0.125) < 1e-12);
    }

    // Straight-line motion is added on top of the origin: the newest particle sits at
    // the origin and the oldest has travelled for its whole life.
    Settings moving = settings;
    moving.velocity = Vec3{0.4, 0.2, 0.0};
    const auto travelled = simulate_particles(validate_settings(moving), 1.0, never);
    CHECK(travelled.has_value() && travelled.value().size() == 2);
    if (travelled.has_value() && travelled.value().size() == 2) {
        const ParticleInstance& newest = travelled.value().back();
        CHECK(std::abs(newest.age_seconds) < 1e-12);
        CHECK(std::abs(newest.position.x - 0.25) < 1e-9);
        CHECK(std::abs(newest.position.y + 0.5) < 1e-9);

        const ParticleInstance& oldest = travelled.value().front();
        CHECK(std::abs(oldest.age_seconds - 1.0) < 1e-9);
        CHECK(std::abs(oldest.position.x - 0.65) < 1e-9);
        CHECK(std::abs(oldest.position.y + 0.3) < 1e-9);
    }
}

void test_random_streams() {
    CHECK(mix64(0) == mix64(0));
    CHECK(mix64(1) != mix64(2));

    const std::uint64_t bits = stream_bits(1, 1, RandomPurpose::velocity_x);
    CHECK(bits == stream_bits(1, 1, RandomPurpose::velocity_x));
    CHECK(bits != stream_bits(2, 1, RandomPurpose::velocity_x)); // seed matters
    CHECK(bits != stream_bits(1, 2, RandomPurpose::velocity_x)); // particle id matters
    CHECK(bits != stream_bits(1, 1, RandomPurpose::velocity_y)); // purpose matters

    for (std::uint64_t id = 0; id < 128; ++id) {
        const double unit = unit_value(1, id, RandomPurpose::position_x);
        const double symmetric = symmetric_value(1, id, RandomPurpose::position_x);
        CHECK(unit >= 0.0 && unit < 1.0);
        CHECK(symmetric >= -1.0 && symmetric < 1.0);
    }
}

void test_emitter_shapes_and_spread() {
    const CancellingAfterFirstPoll never(false);

    Settings settings;
    settings.particle_count = 256;
    settings.birth_rate = 256.0;
    settings.particle_lifetime_seconds = 1.0;
    settings.emitter_size = 0.2; // half extent 0.1 layer heights
    settings.velocity = Vec3{0.0, 0.3, 0.0};
    settings.velocity_spread = 0.2;

    // Point without spread reproduces the closed-form behaviour M2 shipped.
    Settings flat = settings;
    flat.emitter_shape = EmitterShape::point;
    flat.velocity_spread = 0.0;
    const auto point = simulate_particles(validate_settings(flat), 1.0, never);
    CHECK(point.has_value() && !point.value().empty());
    if (point.has_value() && !point.value().empty()) {
        for (const ParticleInstance& particle : point.value()) {
            CHECK(std::abs(particle.position.x) < 1e-12);
            CHECK(std::abs(particle.position.y - 0.3 * particle.age_seconds) < 1e-12);
        }
    }

    // Every shape stays inside its advertised extent and actually spreads.
    const EmitterShape shapes[3] = {EmitterShape::box, EmitterShape::sphere, EmitterShape::disc};
    for (const EmitterShape shape : shapes) {
        Settings shaped = settings;
        shaped.emitter_shape = shape;
        shaped.velocity = Vec3{0.0, 0.0, 0.0}; // isolate the birth distribution
        shaped.velocity_spread = 0.0;
        const auto particles = simulate_particles(validate_settings(shaped), 1.0, never);
        CHECK(particles.has_value());
        if (!particles.has_value()) {
            continue;
        }
        CHECK(particles.value().size() > 16);
        double furthest = 0.0;
        for (const ParticleInstance& particle : particles.value()) {
            if (shape == EmitterShape::disc) {
                CHECK(std::abs(particle.position.z) < 1e-12);
            }
            CHECK(std::abs(particle.position.x) <= 0.1 + 1e-12);
            CHECK(std::abs(particle.position.y) <= 0.1 + 1e-12);
            CHECK(std::abs(particle.position.z) <= 0.1 + 1e-12);
            furthest = std::max(furthest, std::abs(particle.position.x));
        }
        CHECK(furthest > 0.02); // the seeded distribution is not collapsed onto the centre
    }

    // Velocity spread is bounded per axis, actually applied, and sticks to a particle.
    const auto jittered = simulate_particles(validate_settings(settings), 1.0, never);
    CHECK(jittered.has_value() && !jittered.value().empty());
    if (!jittered.has_value() || jittered.value().empty()) {
        return;
    }
    bool deviates = false;
    for (const ParticleInstance& particle : jittered.value()) {
        if (particle.age_seconds <= 0.0) {
            continue;
        }
        const double velocity_y = particle.position.y / particle.age_seconds;
        CHECK(std::abs(velocity_y - 0.3) <= 0.2 + 1e-9);
        if (std::abs(velocity_y - 0.3) > 0.05) {
            deviates = true;
        }
    }
    CHECK(deviates);

    const auto later = simulate_particles(validate_settings(settings), 1.2, never);
    CHECK(later.has_value());
    if (later.has_value()) {
        for (const ParticleInstance& particle : jittered.value()) {
            if (particle.age_seconds <= 0.0) {
                continue;
            }
            for (const ParticleInstance& moved : later.value()) {
                if (moved.id != particle.id || moved.age_seconds <= 0.0) {
                    continue;
                }
                const double before = particle.position.y / particle.age_seconds;
                const double after = moved.position.y / moved.age_seconds;
                CHECK(std::abs(before - after) < 1e-9);
                break;
            }
            break; // one stable particle is enough to pin the contract
        }
    }

    // Random Seed is observable now: same request reproduces exactly, another seed
    // moves the particles.
    Settings other_seed = settings;
    other_seed.seed = 7;
    const auto repeated = simulate_particles(validate_settings(settings), 1.0, never);
    const auto reseeded = simulate_particles(validate_settings(other_seed), 1.0, never);
    CHECK(repeated.has_value() && reseeded.has_value());
    if (repeated.has_value() && reseeded.has_value()) {
        CHECK(repeated.value().size() == jittered.value().size());
        bool identical = true;
        bool different = false;
        for (std::size_t i = 0; i < reseeded.value().size() && i < jittered.value().size(); ++i) {
            if (repeated.value()[i].position.y != jittered.value()[i].position.y) {
                identical = false;
            }
            if (reseeded.value()[i].position.y != jittered.value()[i].position.y) {
                different = true;
            }
        }
        CHECK(identical);
        CHECK(different);
    }
}

void test_simulation_boundaries() {
    Settings settings;
    settings.birth_rate = 10.0;
    settings.particle_lifetime_seconds = 1.0;
    settings.particle_count = 1000;
    settings.velocity = Vec3{0.5, 0.25, 0.0};
    settings.velocity_spread = 0.0; // the boundary cases assert exact positions

    const ValidatedSettings validated = validate_settings(settings);
    const CancellingAfterFirstPoll never(false);

    // Half-open lifetime: at t = 1.0 slot 0 has age 1.0 and is gone, slot 10 is born.
    const auto at_one = simulate_particles(validated, 1.0, never);
    CHECK(at_one.has_value());
    CHECK(at_one.value().size() == 10);
    CHECK(at_one.value().front().id == 1);
    CHECK(at_one.value().back().id == 10);
    CHECK(std::abs(at_one.value().back().age_seconds) < 1e-12);
    CHECK(std::abs(at_one.value().back().position.y) < 1e-12);
    CHECK(std::abs(at_one.value().front().age_seconds - 0.9) < 1e-9);
    CHECK(std::abs(at_one.value().front().position.y - 0.225) < 1e-9);
    CHECK(at_one.value().front().size_pixels == settings.particle_size);
    CHECK(at_one.value().front().lifetime_seconds == settings.particle_lifetime_seconds);

    // The clock is anchored at host time zero.
    const auto at_zero = simulate_particles(validated, 0.0, never);
    CHECK(at_zero.has_value() && at_zero.value().size() == 1 && at_zero.value().front().id == 0);

    const auto before_zero = simulate_particles(validated, -0.5, never);
    CHECK(before_zero.has_value() && before_zero.value().empty());

    Settings no_births = settings;
    no_births.birth_rate = 0.0;
    CHECK(simulate_particles(validate_settings(no_births), 5.0, never).value().empty());

    Settings instant = settings;
    instant.particle_lifetime_seconds = 0.0;
    CHECK(simulate_particles(validate_settings(instant), 5.0, never).value().empty());

    Settings capped = settings;
    capped.birth_rate = 1000.0;
    capped.particle_count = 5;
    const auto capped_result = simulate_particles(validate_settings(capped), 1.0, never);
    CHECK(capped_result.value().size() == 5);
    CHECK(capped_result.value().front().id == 996); // the newest slots survive
    CHECK(capped_result.value().back().id == 1000);

    // Frame order must not matter: repeated and inverted evaluation agree.
    const auto forward = simulate_particles(validated, 2.0, never);
    const auto earlier = simulate_particles(validated, 1.5, never);
    const auto repeated = simulate_particles(validated, 2.0, never);
    CHECK(forward.value().size() == repeated.value().size());
    for (std::size_t i = 0; i < forward.value().size(); ++i) {
        CHECK(forward.value()[i].id == repeated.value()[i].id);
        CHECK(forward.value()[i].age_seconds == repeated.value()[i].age_seconds);
        CHECK(forward.value()[i].position.y == repeated.value()[i].position.y);
    }
    CHECK(!earlier.value().empty());

    const CancellingAfterFirstPoll cancelled(true);
    const auto cancelled_result = simulate_particles(validated, 1.0, cancelled);
    CHECK(!cancelled_result.has_value());
    CHECK(cancelled_result.error().code == ErrorCode::cancelled);
}

void test_renderer_determinism_and_geometry() {
    const CpuParticleRenderer renderer;
    const CancellingAfterFirstPoll never(false);

    Scene scene;
    scene.settings.particle_count = 8;
    scene.settings.birth_rate = 8.0;
    scene.settings.particle_lifetime_seconds = 1.0;
    scene.settings.particle_size = 6.0;
    scene.settings.velocity = Vec3{0.0, 0.5, 0.0};
    scene.settings.velocity_spread = 0.0; // the trail rows below are asserted exactly
    scene.time_seconds = 0.95;

    const RenderRequest request = build_request(scene);
    const auto first = renderer.render(request, never);
    const auto second = renderer.render(request, never);
    CHECK(first.has_value() && second.has_value());
    CHECK(first.value().pixels == second.value().pixels);
    CHECK(first.value().row_bytes == 64u * 4u);

    // Eight particles, velocity 0.5 layer-heights/s, at t = 0.95 form a trail from
    // y = 0.075 * 32 px up to y = 0.95 * 32 px above the center. Rows above the
    // center must carry alpha, rows below the trail must stay empty, and the sprite
    // must never touch the frame corners.
    CHECK(pixel8(first.value(), 32, 24).a > 0);
    CHECK(pixel8(first.value(), 32, 40).a == 0);
    CHECK(pixel8(first.value(), 0, 0).a == 0);
    CHECK(pixel8(first.value(), 63, 0).a == 0);
    CHECK(pixel8(first.value(), 63, 63).a == 0);
}

void test_renderer_region_of_interest() {
    const CpuParticleRenderer renderer;
    const CancellingAfterFirstPoll never(false);

    Scene full;
    full.settings.particle_count = 6;
    full.settings.birth_rate = 6.0;
    full.settings.particle_lifetime_seconds = 1.0;
    full.settings.particle_size = 8.0;
    full.settings.velocity = Vec3{0.4, 0.2, 0.0};
    full.time_seconds = 0.9;

    Scene partial = full;
    partial.roi = RectI{32, 0, 64, 64};

    const auto full_output = renderer.render(build_request(full), never);
    const auto partial_output = renderer.render(build_request(partial), never);
    CHECK(full_output.has_value() && partial_output.has_value());
    CHECK(partial_output.value().width() == 32 && partial_output.value().height() == 64);

    bool identical = true;
    for (std::uint32_t y = 0; y < 64 && identical; ++y) {
        for (std::uint32_t x = 0; x < 32; ++x) {
            const Rgba8 expected = pixel8(full_output.value(), x + 32, y);
            const Rgba8 actual = pixel8(partial_output.value(), x, y);
            if (expected.r != actual.r || expected.g != actual.g || expected.b != actual.b ||
                expected.a != actual.a) {
                identical = false;
                break;
            }
        }
    }
    CHECK(identical);
}

void test_renderer_downsampled_frame_mapping() {
    // The frame grid is the render-resolution layer grid and may be smaller than the
    // full-resolution layer size (downsampled preview). World-to-pixel mapping must
    // follow the frame grid, otherwise particles drift or get clipped at low
    // preview resolutions (ADR 0005).
    const CpuParticleRenderer renderer;
    const CancellingAfterFirstPoll never(false);

    Scene scene;
    scene.layer_width = 256;
    scene.layer_height = 256;
    scene.frame_width = 128; // half-resolution preview
    scene.frame_height = 128;
    scene.roi = RectI{0, 0, 128, 128};
    scene.settings.particle_count = 1;
    scene.settings.birth_rate = 1.0;
    scene.settings.particle_lifetime_seconds = 4.0;
    scene.settings.particle_size = 8.0;   // radius 4 full-res pixels -> 2 at half res
    scene.settings.velocity = Vec3{0.0, 0.5, 0.0};
    scene.settings.velocity_spread = 0.0; // the mapped row is asserted exactly
    scene.time_seconds = 0.5;             // one particle, age 0.5 s, y = +0.25 layer heights

    const auto output = renderer.render(build_request(scene), never);
    CHECK(output.has_value());
    if (!output.has_value()) {
        return;
    }
    CHECK(output.value().width() == 128 && output.value().height() == 128);

    // (0.5 - 0.25) * 128 = row 32, so the sprite center sits on row 32 and nothing
    // reaches row 40. At full resolution the same scene would land on row 64.
    CHECK(pixel8(output.value(), 64, 32).a > 0);
    CHECK(pixel8(output.value(), 64, 40).a == 0);
    CHECK(pixel8(output.value(), 64, 0).a == 0);

    Scene full = scene;
    full.frame_width = 256;
    full.frame_height = 256;
    full.roi = RectI{0, 0, 256, 256};
    const auto full_output = renderer.render(build_request(full), never);
    CHECK(full_output.has_value());
    if (full_output.has_value()) {
        CHECK(pixel8(full_output.value(), 128, 64).a > 0);
        CHECK(pixel8(full_output.value(), 128, 32).a == 0);
    }
}

void test_renderer_source_compositing() {
    const CpuParticleRenderer renderer;
    const CancellingAfterFirstPoll never(false);

    Scene scene;
    scene.settings.particle_count = 1;
    scene.settings.birth_rate = 1.0;
    scene.settings.particle_lifetime_seconds = 4.0;
    scene.settings.particle_size = 0.0;                  // invisible: isolates the source path
    scene.settings.velocity = Vec3{0.0, 0.0, 0.0};       // motion is not under test here
    scene.time_seconds = 1.0;

    auto source = make_source(64, 64, 0, 0, Rgba8{255, 0, 0, 255}, AlphaMode::premultiplied);
    scene.source = std::make_shared<const PixelBuffer>(source);

    const auto passthrough = renderer.render(build_request(scene), never);
    CHECK(passthrough.has_value());
    const Rgba8 corner = pixel8(passthrough.value(), 5, 5);
    CHECK(corner.r == 255 && corner.g == 0 && corner.b == 0 && corner.a == 255);

    // A straight-alpha source is premultiplied on input.
    Scene straight_scene = scene;
    straight_scene.source = std::make_shared<const PixelBuffer>(
        make_source(64, 64, 0, 0, Rgba8{255, 0, 0, 128}, AlphaMode::straight));
    const auto straight = renderer.render(build_request(straight_scene), never);
    CHECK(straight.has_value());
    const Rgba8 straight_pixel = pixel8(straight.value(), 5, 5);
    CHECK(straight_pixel.a == 128);
    CHECK(straight_pixel.r <= 129); // premultiplied red: 255 * 128 / 255

    // Source placement is explicit: a source that starts at x = 32 only covers the
    // right half of the frame.
    Scene placed = scene;
    placed.source = std::make_shared<const PixelBuffer>(
        make_source(32, 64, 32, 0, Rgba8{255, 0, 0, 255}, AlphaMode::premultiplied));
    const auto placed_output = renderer.render(build_request(placed), never);
    CHECK(placed_output.has_value());
    CHECK(pixel8(placed_output.value(), 10, 10).a == 0);
    CHECK(pixel8(placed_output.value(), 40, 10).a == 255);

    // A visible sprite composites over the opaque source and adds white.
    Scene sprite = scene;
    sprite.settings.particle_size = 10.0;
    sprite.settings.opacity = 0.5;
    sprite.source = std::make_shared<const PixelBuffer>(source);
    const auto composited = renderer.render(build_request(sprite), never);
    CHECK(composited.has_value());
    const Rgba8 center = pixel8(composited.value(), 32, 32);
    CHECK(center.r == 255);
    CHECK(center.g > 100 && center.g < 200);
    CHECK(center.a == 255);
}

void test_renderer_formats_and_limits() {
    const CancellingAfterFirstPoll never(false);
    const CpuParticleRenderer renderer;

    Scene scene;
    scene.settings.particle_count = 1;
    scene.settings.birth_rate = 1.0;
    scene.settings.particle_lifetime_seconds = 4.0;
    scene.settings.particle_size = 8.0;
    scene.settings.velocity = Vec3{0.0, 0.0, 0.0}; // the sprite must sit at the layer centre
    scene.settings.velocity_spread = 0.0;
    scene.time_seconds = 1.0;

    Scene deep = scene;
    deep.format = PixelFormat::rgba16;
    const auto output16 = renderer.render(build_request(deep), never);
    CHECK(output16.has_value());
    CHECK(output16.value().row_bytes == 64u * 8u);
    CHECK(channel16(output16.value(), 32, 32, 3) == 32768);

    Scene floating = scene;
    floating.format = PixelFormat::rgba32f;
    const auto output32 = renderer.render(build_request(floating), never);
    CHECK(output32.has_value());
    CHECK(output32.value().row_bytes == 64u * 16u);
    CHECK(std::abs(channel32(output32.value(), 32, 32, 3) - 1.0f) < 1e-5f);
    CHECK(std::abs(channel32(output32.value(), 0, 0, 3)) < 1e-6f);

    // Invalid geometry is rejected before any allocation.
    Scene broken = scene;
    broken.roi = RectI{0, 0, 128, 64};
    const auto invalid = renderer.render(build_request(broken), never);
    CHECK(!invalid.has_value());
    CHECK(invalid.error().code == ErrorCode::invalid_request);

    Scene no_duration = scene;
    RenderRequest no_duration_request = build_request(no_duration);
    no_duration_request.frame.frame_duration = RationalTime{0, 24};
    CHECK(!renderer.render(no_duration_request, never).has_value());

    // An empty region of interest is legal and produces an empty output.
    Scene empty_roi = scene;
    empty_roi.roi = RectI{10, 10, 10, 40};
    const auto empty = renderer.render(build_request(empty_roi), never);
    CHECK(empty.has_value());
    CHECK(empty.value().pixels.empty());

    // The bounded-work budget is reported instead of being silently truncated.
    const CpuParticleRenderer budgeted(RenderLimits{256});
    Scene heavy = scene;
    heavy.settings.particle_count = 64;
    heavy.settings.birth_rate = 64.0;
    heavy.settings.particle_size = 64.0;
    const auto limited = budgeted.render(build_request(heavy), never);
    CHECK(!limited.has_value());
    CHECK(limited.error().code == ErrorCode::work_limit_exceeded);

    // Cancellation reaches the rasterizer as well.
    const CancellingAfterFirstPoll cancelled(true);
    const auto cancelled_output = renderer.render(build_request(scene), cancelled);
    CHECK(!cancelled_output.has_value());
    CHECK(cancelled_output.error().code == ErrorCode::cancelled);
}

} // namespace

int main() {
    test_rational_time();
    test_layer_point_conversion();
    test_settings_validation();
    test_random_streams();
    test_simulation_emitter_origin();
    test_emitter_shapes_and_spread();
    test_simulation_boundaries();
    test_renderer_determinism_and_geometry();
    test_renderer_region_of_interest();
    test_renderer_downsampled_frame_mapping();
    test_renderer_source_compositing();
    test_renderer_formats_and_limits();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
