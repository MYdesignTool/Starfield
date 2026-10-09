#include "starfield/core/PluginApi.h"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>

namespace {
using namespace starfield::core;
int checks{},failures{};
void check(bool ok,const char* label){++checks;if(!ok){++failures;std::printf("FAILED: %s\n",label);}}
NodeId node(unsigned b){NodeId id;id.value.bytes[15]=static_cast<std::uint8_t>(b);return id;}
EdgeId edge(unsigned b){EdgeId id;id.value.bytes[15]=static_cast<std::uint8_t>(b);return id;}
struct Cancel {unsigned polls{},stop{1};};
int32_t SF_CORE_CALL cancel(void* opaque){auto& c=*static_cast<Cancel*>(opaque);return ++c.polls>=c.stop;}
}
int main() {
    static_assert(sizeof(SfModelPosition)==32&&sizeof(SfModelAttribute)==24&&sizeof(SfModelTriangle)==36);
    static_assert(SF_CORE_ABI7_RENDER_REQUEST_SIZE+16==sizeof(SfCoreRenderRequest));
    static_assert(SF_CORE_ABI_VERSION==8&&SF_CORE_LEGACY_ABI_VERSION==7);
    SfCoreApi api{},legacy{};check(StarfieldCore_GetApi(8,sizeof(api),&api)==1&&api.abi_version==8,"current ABI8 table negotiates");
    check(StarfieldCore_GetApi(7,sizeof(legacy),&legacy)==1&&legacy.abi_version==7,"ABI7 table remains available");
    check(StarfieldCore_GetApi(6,sizeof(api),&api)==0,"unsupported ABI6 remains rejected");
    check(StarfieldCore_GetApi(8,sizeof(api)-1,&api)==0,"bad API table size rejects");
    Settings settings;settings.particle_count=10;settings.birth_rate=10;settings.particle_lifetime_seconds=2;
    auto graph=make_emitter_particle_output_graph(settings,node(1),node(2),node(3),edge(1),edge(2));check(graph.has_value(),"normal graph fixture constructs");
    auto wire=serialize_graph(graph.value(),particle_node_registry());check(wire.has_value(),"normal graph serializes");
    SfCoreRenderRequest request{};request.struct_size=sizeof(request);
    request.frame={32,32,32,32,{0,0,32,32},1,1,1,60,0,0,0,1,1};
    request.graph_bytes=wire.value().data();request.graph_byte_count=wire.value().size();
    SfCoreRenderResult output{};output.struct_size=sizeof(output);
    check(api.render(&request,&output)==SF_CORE_OK&&output.pixels&&output.pixel_byte_count==4096,"ABI8 old particle graph renders");
    std::vector<std::byte> baseline(static_cast<const std::byte*>(output.pixels),static_cast<const std::byte*>(output.pixels)+output.pixel_byte_count);
    api.release_render_result(&output);check(!output.opaque_handle&&!output.pixels,"result releases in owning API generation");
    // Exactly the old byte span. Sanitized builds catch any accidental tail read.
    auto prefix=std::make_unique<std::byte[]>(SF_CORE_ABI7_RENDER_REQUEST_SIZE);
    request.struct_size=SF_CORE_ABI7_RENDER_REQUEST_SIZE;std::memcpy(prefix.get(),&request,SF_CORE_ABI7_RENDER_REQUEST_SIZE);
    auto* old=reinterpret_cast<const SfCoreRenderRequest*>(prefix.get());
    check(legacy.render(old,&output)==SF_CORE_OK,"ABI7 exact prefix renders without ABI8 tail storage");
    check(output.pixel_byte_count==baseline.size()&&std::memcmp(output.pixels,baseline.data(),baseline.size())==0,"ABI7 output stays byte-identical");legacy.release_render_result(&output);
    SfCoreGpuSceneResult gpu{};gpu.struct_size=sizeof(gpu);
    check(legacy.prepare_gpu_scene(old,&gpu)==SF_CORE_OK,"ABI7 exact prefix prepares GPU scene");legacy.release_gpu_scene(&gpu);
    request.model_source_count=0xffffffffu;request.model_sources=reinterpret_cast<const SfModelSource*>(static_cast<std::uintptr_t>(1));
    check(legacy.render(&request,&output)==SF_CORE_OK,"ABI7 size suppresses poisoned newer tail fields");legacy.release_render_result(&output);
    request.struct_size=sizeof(request);request.model_source_count=0;request.model_sources=nullptr;
    const SfModelPosition positions[]{{0,0,0,1},{1,0,0,1},{0,1,0,1}};
    SfModelTriangle triangle{};for(unsigned c=0;c<3;++c)triangle.corners[c]={c,0xffffffffu,0xffffffffu};
    SfModelSource source{};source.struct_size=sizeof(source);source.resource_id[0]=1;
    source.position_count=3;source.triangle_count=1;source.positions=positions;source.triangles=&triangle;
    request.model_source_count=1;request.model_sources=&source;
    check(api.render(&request,&output)==SF_CORE_OK,"ABI8 validates and copies independent numeric Model source");
    check(output.pixel_byte_count==baseline.size()&&std::memcmp(output.pixels,baseline.data(),baseline.size())==0,"unused resource does not change an existing primitive graph");api.release_render_result(&output);
    const auto valid_source=source;
    source.struct_size=0;check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST&&!output.pixels,"bad source ABI size rejects");source=valid_source;
    source.positions=nullptr;check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"missing position array rejects");source=valid_source;
    source.triangles=nullptr;check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"missing triangle array rejects");source=valid_source;
    source.normal_count=1;source.normals=nullptr;check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"missing optional normal storage rejects");source=valid_source;
    source.texture_coordinate_count=1;source.texture_coordinates=nullptr;check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"missing optional UV storage rejects");source=valid_source;
    source.position_count=65537;source.positions=reinterpret_cast<const SfModelPosition*>(static_cast<std::uintptr_t>(1));
    check(api.render(&request,&output)==SF_CORE_WORK_LIMIT,"count preflight occurs before any position dereference");source=valid_source;
    source.resource_id[0]=0;check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"zero imported source ID rejects");source=valid_source;
    std::array<SfModelSource,2> duplicates{source,source};request.model_source_count=2;request.model_sources=duplicates.data();
    check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"duplicate source IDs reject before copying");
    request.model_source_count=257;request.model_sources=nullptr;check(api.render(&request,&output)==SF_CORE_WORK_LIMIT,"source count limit precedes pointer access");
    request.model_source_count=1;request.model_sources=nullptr;check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"missing source table rejects");request.model_sources=&source;
    std::array<SfModelSource,9> oversized{};
    for(unsigned i=0;i<oversized.size();++i) {
        auto& s=oversized[i];s=source;s.resource_id[0]=static_cast<std::uint8_t>(i+1);
        s.position_count=s.texture_coordinate_count=s.normal_count=s.triangle_count=65536;
        s.positions=reinterpret_cast<const SfModelPosition*>(static_cast<std::uintptr_t>(1));
        s.texture_coordinates=s.normals=reinterpret_cast<const SfModelAttribute*>(static_cast<std::uintptr_t>(1));
        s.triangles=reinterpret_cast<const SfModelTriangle*>(static_cast<std::uintptr_t>(1));
    }
    request.model_source_count=9;request.model_sources=oversized.data();
    check(api.render(&request,&output)==SF_CORE_WORK_LIMIT,"aggregate byte preflight rejects before any numeric-array dereference");
    request.model_source_count=1;request.model_sources=&source;
    std::array<SfModelPosition,3> invalid_positions{positions[0],positions[1],positions[2]};invalid_positions[0].x=std::numeric_limits<double>::quiet_NaN();
    source.positions=invalid_positions.data();check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"copied nonfinite numeric geometry rejects");source=valid_source;
    triangle.corners[0].position=3;check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"copied invalid corner rejects");triangle.corners[0].position=0;
    const SfModelAttribute normal[]{{0,0,0}};source.normal_count=1;source.normals=normal;
    check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"invalid source normal rejects even when not indexed");source=valid_source;
    Cancel stop{0,2};request.is_cancelled=cancel;request.cancel_context=&stop;
    check(api.render(&request,&output)==SF_CORE_CANCELLED&&!output.pixels,"Model numeric copying cooperatively cancels");
    request.is_cancelled=nullptr;request.cancel_context=nullptr;request.struct_size=SF_CORE_ABI7_RENDER_REQUEST_SIZE-1;
    check(api.render(&request,&output)==SF_CORE_INVALID_REQUEST,"shorter-than-ABI7 prefix rejects");
    std::printf("Model transport: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
