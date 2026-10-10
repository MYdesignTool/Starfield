cmake_minimum_required(VERSION 3.24)

project(StarfieldCore VERSION 0.1.0 LANGUAGES CXX)

add_library(starfield_core STATIC
    src/core/CpuRenderer.cpp
    src/core/Geometry.cpp
    src/core/ModelGeometry.cpp
    src/core/ModelResources.cpp
    src/core/ModelScene.cpp
    src/core/MotionGeometry.cpp
    src/core/MotionPathTravel.cpp
    src/core/Graph.cpp
    src/core/GraphConstruction.cpp
    src/core/GraphEvaluation.cpp
    src/core/EmitterHistory.cpp
    src/core/ParticleSimulation.cpp
    src/core/ParticleTransform.cpp
    src/core/ParticleTexture.cpp
    src/core/Random.cpp
    src/core/Render.cpp
    src/core/SequenceCodec.cpp
    src/core/Settings.cpp
    src/core/SpriteScene.cpp
    src/core/Time.cpp
)

# The AE adapter loads this DLL by versioned filename; it never links an import
# library. The shared target is useful for portable ABI checks outside MSBuild.
add_library(starfield_core_dll SHARED src/core/PluginApi.cpp)
target_link_libraries(starfield_core_dll PRIVATE starfield_core)
target_include_directories(starfield_core_dll PRIVATE include)
target_compile_features(starfield_core_dll PRIVATE cxx_std_20)
target_compile_definitions(starfield_core_dll PRIVATE SF_CORE_BUILD_DLL)
set_target_properties(starfield_core_dll PROPERTIES OUTPUT_NAME StarfieldCore)
target_include_directories(starfield_core PUBLIC include)
target_compile_features(starfield_core PUBLIC cxx_std_20)
set_target_properties(starfield_core PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(starfield_core PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(starfield_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

# Host-independent self-tests. The AE module itself is built by
# ae_plugin/BuildWindows.ps1, which also runs the SDK's PiPL tool.
include(CTest)
if(BUILD_TESTING)
    add_executable(starfield_motion_path_graph_tests tests/motion_path_graph_tests.cpp)
    target_link_libraries(starfield_motion_path_graph_tests PRIVATE starfield_core)
    add_test(NAME starfield_motion_path_graph_tests COMMAND starfield_motion_path_graph_tests)
    add_executable(starfield_motion_path_travel_tests tests/motion_path_travel_tests.cpp)
    target_link_libraries(starfield_motion_path_travel_tests PRIVATE starfield_core)
    add_test(NAME starfield_motion_path_travel_tests COMMAND starfield_motion_path_travel_tests)
    add_executable(starfield_motion_look_at_graph_tests tests/motion_look_at_graph_tests.cpp)
    target_link_libraries(starfield_motion_look_at_graph_tests PRIVATE starfield_core)
    add_test(NAME starfield_motion_look_at_graph_tests COMMAND starfield_motion_look_at_graph_tests)
    add_executable(starfield_motion_circle_graph_tests tests/motion_circle_graph_tests.cpp)
    target_link_libraries(starfield_motion_circle_graph_tests PRIVATE starfield_core)
    add_test(NAME starfield_motion_circle_graph_tests COMMAND starfield_motion_circle_graph_tests)
    add_executable(starfield_motion_geometry_tests tests/motion_geometry_tests.cpp)
    target_link_libraries(starfield_motion_geometry_tests PRIVATE starfield_core)
    add_test(NAME starfield_motion_geometry_tests COMMAND starfield_motion_geometry_tests)
    add_executable(starfield_model_geometry_tests tests/model_geometry_tests.cpp)
    target_link_libraries(starfield_model_geometry_tests PRIVATE starfield_core)
    add_test(NAME starfield_model_geometry_tests COMMAND starfield_model_geometry_tests)
    add_executable(starfield_model_scene_tests tests/model_scene_tests.cpp)
    target_link_libraries(starfield_model_scene_tests PRIVATE starfield_core)
    add_test(NAME starfield_model_scene_tests COMMAND starfield_model_scene_tests)
    add_executable(starfield_model_resource_tests tests/model_resource_tests.cpp)
    target_link_libraries(starfield_model_resource_tests PRIVATE starfield_core)
    add_test(NAME starfield_model_resource_tests COMMAND starfield_model_resource_tests)
    add_executable(starfield_model_transport_tests tests/model_transport_tests.cpp src/core/PluginApi.cpp)
    target_link_libraries(starfield_model_transport_tests PRIVATE starfield_core)
    add_test(NAME starfield_model_transport_tests COMMAND starfield_model_transport_tests)
    add_executable(starfield_model_particle_tests tests/model_particle_tests.cpp src/core/PluginApi.cpp)
    target_link_libraries(starfield_model_particle_tests PRIVATE starfield_core)
    add_test(NAME starfield_model_particle_tests COMMAND starfield_model_particle_tests)
    add_executable(starfield_model_graph_tests tests/model_graph_tests.cpp src/core/PluginApi.cpp)
    target_link_libraries(starfield_model_graph_tests PRIVATE starfield_core)
    add_test(NAME starfield_model_graph_tests COMMAND starfield_model_graph_tests)
    add_executable(starfield_particle_texture_tests tests/particle_texture_tests.cpp src/core/PluginApi.cpp)
    target_link_libraries(starfield_particle_texture_tests PRIVATE starfield_core)
    add_test(NAME starfield_particle_texture_tests COMMAND starfield_particle_texture_tests)
    add_executable(starfield_core_tests tests/core_tests.cpp)
    target_sources(starfield_core_tests PRIVATE src/core/PluginApi.cpp)
    target_link_libraries(starfield_core_tests PRIVATE starfield_core)
    add_test(NAME starfield_core_tests COMMAND starfield_core_tests)
    add_executable(starfield_particle_transform_tests tests/particle_transform_tests.cpp)
    target_link_libraries(starfield_particle_transform_tests PRIVATE starfield_core)
    add_test(NAME starfield_particle_transform_tests COMMAND starfield_particle_transform_tests)
    add_executable(starfield_transform_transport_tests tests/transform_transport_tests.cpp src/core/PluginApi.cpp)
    target_link_libraries(starfield_transform_transport_tests PRIVATE starfield_core)
    add_test(NAME starfield_transform_transport_tests COMMAND starfield_transform_transport_tests)
    add_executable(starfield_transform_graph_tests tests/transform_graph_tests.cpp)
    target_link_libraries(starfield_transform_graph_tests PRIVATE starfield_core)
    add_test(NAME starfield_transform_graph_tests COMMAND starfield_transform_graph_tests)
    add_executable(starfield_transform_binding_tests tests/transform_binding_tests.cpp)
    target_include_directories(starfield_transform_binding_tests PRIVATE ae_plugin)
    target_link_libraries(starfield_transform_binding_tests PRIVATE starfield_core)
    add_test(NAME starfield_transform_binding_tests COMMAND starfield_transform_binding_tests)
    set_tests_properties(starfield_transform_binding_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
