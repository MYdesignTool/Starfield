#include "../ae_plugin/CoreLoader.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/SequenceCodec.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
bool check(bool condition, const char* label) {
    if (!condition) std::cerr << "FAIL " << label << '\n';
    return condition;
}

std::filesystem::path executable_directory() {
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return {};
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
}
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 3) return 2;
    try {
        const auto runtime = executable_directory() / L"StarfieldRuntime";
        std::filesystem::create_directories(runtime);
        const auto first = runtime / L"StarfieldCore-loader-a.dll";
        const auto second = runtime / L"StarfieldCore-loader-b.dll";
        const auto wrong = runtime / L"StarfieldCore-loader-wrong.dll";
        std::filesystem::copy_file(argv[1], first, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(argv[1], second, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(argv[2], wrong, std::filesystem::copy_options::overwrite_existing);
        const auto manifest = runtime / L"current.txt";
        const auto select = [&](const char* name) {
            std::ofstream out(manifest, std::ios::binary | std::ios::trunc);
            out << name << '\n';
        };

        select("StarfieldCore-loader-a.dll");
        auto old = starfield::adapter::acquire_core();
        if (!check(static_cast<bool>(old) && old.changed, "initial core load")) return 1;
        auto same = starfield::adapter::reload_core();
        if (!check(static_cast<bool>(same) && !same.changed, "same core is not reloaded")) return 1;

        select("StarfieldCore-loader-b.dll");
        auto current = starfield::adapter::reload_core();
        if (!check(static_cast<bool>(current) && current.changed, "versioned core switch")) return 1;
        if (!check(old.generation->cache_identity() != current.generation->cache_identity(),
                   "cache identity changes")) return 1;
        SfCoreInspectRequest invalid{};
        invalid.struct_size = sizeof(invalid);
        SfCoreInspectResult output{};
        output.struct_size = sizeof(output);
        if (!check(old.generation->api().inspect(&invalid, &output) == SF_CORE_INVALID_REQUEST,
                   "retired generation remains callable while pinned")) return 1;
        if (!check(GetModuleHandleW(first.c_str()) != nullptr, "old DLL pinned in process")) return 1;
        old.generation.reset();
        same.generation.reset();
        if (!check(GetModuleHandleW(first.c_str()) == nullptr,
                   "old DLL unloads after final lease")) return 1;

        select("../outside.dll");
        auto bad = starfield::adapter::reload_core();
        if (!check(static_cast<bool>(bad) && !bad.error.empty() &&
                   bad.generation == current.generation, "bad manifest retains prior core")) return 1;
        select("StarfieldCore-loader-a.dll\nStarfieldCore-loader-b.dll");
        auto extra_line = starfield::adapter::reload_core();
        if (!check(static_cast<bool>(extra_line) &&
                   extra_line.error.find("invalid core runtime manifest filename") != std::string::npos &&
                   extra_line.generation == current.generation,
                   "multi-line manifest retains prior core")) return 1;
        select("StarfieldCore-missing.dll");
        auto missing = starfield::adapter::reload_core();
        if (!check(static_cast<bool>(missing) && !missing.error.empty() &&
                   missing.generation == current.generation, "missing DLL retains prior core")) return 1;
        select("StarfieldCore-loader-wrong.dll");
        auto incompatible = starfield::adapter::reload_core();
        if (!check(static_cast<bool>(incompatible) &&
                   incompatible.error.find("incompatible ABI") != std::string::npos &&
                   incompatible.generation == current.generation,
                   "incompatible DLL retains prior core")) return 1;
        select("StarfieldCore-loader-b.dll");
        bad.generation.reset();
        extra_line.generation.reset();
        missing.generation.reset();
        incompatible.generation.reset();
        current.generation.reset();

        starfield::core::Uuid128 emitter_uuid{};
        starfield::core::Uuid128 particle_uuid{};
        starfield::core::Uuid128 output_uuid{};
        starfield::core::Uuid128 emitter_particle_edge_uuid{};
        starfield::core::Uuid128 particle_output_edge_uuid{};
        emitter_uuid.bytes[0] = 1;
        particle_uuid.bytes[0] = 2;
        output_uuid.bytes[0] = 3;
        emitter_particle_edge_uuid.bytes[0] = 4;
        particle_output_edge_uuid.bytes[0] = 5;
        const auto graph = starfield::core::make_emitter_particle_output_graph(
            starfield::core::Settings{}, starfield::core::NodeId{emitter_uuid},
            starfield::core::NodeId{particle_uuid}, starfield::core::NodeId{output_uuid},
            starfield::core::EdgeId{emitter_particle_edge_uuid},
            starfield::core::EdgeId{particle_output_edge_uuid});
        if (!check(graph.has_value(), "construct render graph for live switch")) return 1;
        const auto bytes = starfield::core::serialize_graph(
            graph.value(), starfield::core::particle_node_registry());
        if (!check(bytes.has_value(), "encode render graph for live switch")) return 1;

        struct RenderGate {
            std::atomic<bool> entered{false};
            std::atomic<bool> proceed{false};
        } gate;
        auto in_flight = starfield::adapter::acquire_core();
        if (!check(static_cast<bool>(in_flight), "acquire generation for live render")) return 1;
        std::atomic<bool> render_ok{false};
        std::jthread rendering([&, generation = in_flight.generation] {
            SfCoreRenderRequest request{};
            request.struct_size = sizeof(request);
            request.frame = SfCoreFrame{64, 64, 64, 64, {0, 0, 64, 64},
                                        1, 1, 1, 24, 0, 0, 1, 1, 1.0};
            request.graph_bytes = bytes.value().data();
            request.graph_byte_count = bytes.value().size();
            request.cancel_context = &gate;
            request.is_cancelled = [](void* context) -> std::int32_t {
                auto& state = *static_cast<RenderGate*>(context);
                state.entered.store(true, std::memory_order_release);
                while (!state.proceed.load(std::memory_order_acquire)) std::this_thread::yield();
                return 0;
            };
            SfCoreRenderResult output{};
            output.struct_size = sizeof(output);
            const auto status = generation->api().render(&request, &output);
            render_ok.store(status == SF_CORE_OK && output.status == SF_CORE_OK &&
                            output.pixel_byte_count > 0 && output.pixels != nullptr);
            generation->api().release_render_result(&output);
        });
        const auto render_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!gate.entered.load(std::memory_order_acquire) &&
               std::chrono::steady_clock::now() < render_deadline) std::this_thread::yield();
        bool live_switch_ok = gate.entered.load(std::memory_order_acquire);
        if (live_switch_ok) {
            select("StarfieldCore-loader-a.dll");
            const auto switched = starfield::adapter::reload_core();
            live_switch_ok = static_cast<bool>(switched) && switched.changed &&
                             GetModuleHandleW(second.c_str()) != nullptr;
        }
        gate.proceed.store(true, std::memory_order_release);
        rendering.join();
        if (!check(live_switch_ok && render_ok.load(),
                   "in-flight render completes on its pinned DLL after reload")) return 1;
        in_flight.generation.reset();
        if (!check(GetModuleHandleW(second.c_str()) == nullptr,
                   "rendered DLL unloads after its final lease")) return 1;

        // Exercise the same lease pattern as SmartFX: a caller owns a generation
        // while another thread selects and publishes a newer DLL. The C API call
        // must remain valid until that caller drops its own lease.
        std::atomic<std::uint64_t> calls{0};
        std::atomic<bool> worker_failed{false};
        std::vector<std::jthread> workers;
        for (int index = 0; index < 4; ++index) {
            workers.emplace_back([&](std::stop_token stop) {
                while (!stop.stop_requested()) {
                    const auto lease = starfield::adapter::acquire_core();
                    if (!lease) {
                        worker_failed.store(true);
                        return;
                    }
                    SfCoreInspectRequest request{};
                    request.struct_size = sizeof(request);
                    SfCoreInspectResult response{};
                    response.struct_size = sizeof(response);
                    if (lease.generation->api().inspect(&request, &response) != SF_CORE_INVALID_REQUEST) {
                        worker_failed.store(true);
                        return;
                    }
                    calls.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (calls.load(std::memory_order_relaxed) == 0 &&
               std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
        bool reloads_ok = calls.load(std::memory_order_relaxed) > 0;
        for (int index = 0; index < 32 && reloads_ok; ++index) {
            select(index % 2 == 0 ? "StarfieldCore-loader-b.dll" : "StarfieldCore-loader-a.dll");
            const auto switched = starfield::adapter::reload_core();
            reloads_ok = static_cast<bool>(switched) && switched.changed;
        }
        workers.clear(); // jthread requests stop and joins before checking results
        if (!check(reloads_ok && !worker_failed.load() && calls.load() > 0,
                   "concurrent core leases survive repeated reloads")) return 1;
        std::cout << "core loader checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
