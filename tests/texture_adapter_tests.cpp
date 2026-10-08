#include "TextureResources.hpp"
#include "NodeRecord.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "starfield/core/Settings.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
using namespace starfield;
namespace {
int checks{},failures{},leases{},pixels_out{},pixels_in{},checkout_fail=-1;
bool reverse_clock{},cancelled{},missing{},self_reference{},cancel_after_checkout{};
AEGP_PFInterfaceSuite1 pf{};AEGP_LayerSuite9 layers{};AEGP_ItemSuite9 items{};AEGP_CompSuite11 comps{};
PF_EffectWorld world{};std::array<PF_PixelFloat,4> pixels{{{1,.2f,.4f,.6f},{1,.3f,.5f,.7f},{1,.4f,.6f,.8f},{1,.5f,.7f,.9f}}};
std::map<A_long,A_long> times;
void check(bool value,const char* label){++checks;if(!value){++failures;std::printf("FAILED: %s\n",label);}}
template<class T>T handle(int id){return reinterpret_cast<T>(static_cast<std::intptr_t>(id));}
A_Err acquire(const char* name,int32,const void** out) {
    if(!std::strcmp(name,kAEGPPFInterfaceSuite))*out=&pf;
    else if(!std::strcmp(name,kAEGPLayerSuite))*out=&layers;
    else if(!std::strcmp(name,kAEGPItemSuite))*out=&items;
    else if(!std::strcmp(name,kAEGPCompSuite))*out=&comps;
    else return 1;
    ++leases;return 0;
}
A_Err release(const char*,int32){--leases;return 0;}
struct Cancel:core::Cancellation {bool is_cancelled()const noexcept override{return cancelled;}} cancel;
core::Uuid128 uuid(int id){core::Uuid128 value{};value.bytes.back()=static_cast<std::uint8_t>(id);return value;}
core::Graph graph() {
    core::Settings settings;settings.birth_rate=4;
    auto result=core::make_emitter_particle_output_graph(settings,core::NodeId{uuid(1)},core::NodeId{uuid(2)},
        core::NodeId{uuid(3)},core::EdgeId{uuid(4)},core::EdgeId{uuid(5)});
    auto scene=result.take_value();
    auto& particle=scene.nodes[1];
    bool shape=false;
    for(auto& p:particle.parameters)if(p.key==core::graph_keys::kParticleShape){p.value=std::uint32_t(3);shape=true;}
    if(!shape)particle.parameters.push_back({core::graph_keys::kParticleShape,std::uint32_t(3)});
    particle.parameters.push_back({core::graph_keys::kTextureFront,std::uint32_t(55)});
    return scene;
}
void reset(){times.clear();pixels_out=pixels_in=0;checkout_fail=-1;reverse_clock=cancelled=missing=self_reference=cancel_after_checkout=false;}
}
int main() {
    namespace nodes=adapter::native_nodes;
    check(nodes::parameter_count(nodes::Kind::particle)==528,"append count");
    check(nodes::uuid_first_index(nodes::Kind::particle)==510 && nodes::sync_guard_index(nodes::Kind::particle)==518,"legacy identity fixed");
    check(nodes::authored_parameter(nodes::Kind::particle,521) && !nodes::authored_parameter(nodes::Kind::particle,520),"layer authored / topic excluded");
    pf.AEGP_GetEffectLayer=[](PF_ProgPtr,AEGP_LayerH* out)->A_Err{*out=handle<AEGP_LayerH>(1);return 0;};
    layers.AEGP_GetLayerParentComp=[](AEGP_LayerH,AEGP_CompH* out)->A_Err{*out=handle<AEGP_CompH>(10);return 0;};
    layers.AEGP_GetLayerFromLayerID=[](AEGP_CompH,AEGP_LayerIDVal id,AEGP_LayerH* out)->A_Err{*out=missing?nullptr:handle<AEGP_LayerH>(self_reference?1:2);return id==55?0:1;};
    layers.AEGP_GetLayerSourceItem=[](AEGP_LayerH,AEGP_ItemH* out)->A_Err{*out=handle<AEGP_ItemH>(11);return 0;};
    layers.AEGP_GetLayerStretch=[](AEGP_LayerH,A_Ratio* out)->A_Err{*out={reverse_clock?-1:1,1};return 0;};
    layers.AEGP_GetLayerInPoint=[](AEGP_LayerH,AEGP_LTimeMode,A_Time* out)->A_Err{*out={0,30};return 0;};
    layers.AEGP_GetLayerDuration=[](AEGP_LayerH,AEGP_LTimeMode,A_Time* out)->A_Err{*out={60,30};return 0;};
    layers.AEGP_ConvertCompToLayerTime=[](AEGP_LayerH,const A_Time* in,A_Time* out)->A_Err{*out={reverse_clock?static_cast<A_long>(4*in->scale-in->value):in->value,in->scale};return 0;};
    items.AEGP_GetItemDimensions=[](AEGP_ItemH,A_long* w,A_long* h)->A_Err{*w=2;*h=2;return 0;};
    items.AEGP_GetItemPixelAspectRatio=[](AEGP_ItemH,A_Ratio* out)->A_Err{*out={2,1};return 0;};
    comps.AEGP_GetCompFramerate=[](AEGP_CompH,A_FpLong* out)->A_Err{*out=30;return 0;};
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData in{};in.pica_basicP=&basic;in.current_time=30;in.time_step=1;in.time_scale=30;PF_OutData out{};
    PF_PreRenderCallbacks callbacks{};
    callbacks.GuidMixInPtr=[](PF_ProgPtr,A_u_long,const void*)->PF_Err{return 0;};
    callbacks.checkout_layer=[](PF_ProgPtr,PF_ParamIndex slot,A_long id,const PF_RenderRequest* request,A_long time,A_long,A_u_long,PF_CheckoutResult* result)->PF_Err{
        check(slot==626,"renderer owns checkout slot");check(request->rect.right==2 && request->rect.bottom==2,"full source canvas requested");
        if(id==checkout_fail)return PF_Err_BAD_CALLBACK_PARAM;
        times[id]=time;result->result_rect={0,0,2,2};result->max_result_rect=result->result_rect;
        result->ref_width=result->ref_height=2;result->par={2,1};return 0;
    };
    PF_PreRenderInput input{};PF_PreRenderOutput output{};PF_PreRenderExtra extra{&input,&output,&callbacks};
    adapter::MotionExposure motion;
    for(std::uint32_t mode=0;mode<8;++mode) {
        reset();auto scene=graph();scene.nodes[1].parameters.push_back({core::graph_keys::kTextureTimeMode,mode});
        adapter::TexturePreparation prepared;
        auto error=adapter::prepare_texture_resources(&in,&out,&extra,scene,motion,1080,1,cancel,prepared);
        check(!error && !prepared.checkouts.empty(),"all eight modes plan host frames");
        check(prepared.sources.size()==1 && prepared.sources[0].pixel_aspect_ratio==2,"source metadata captured");
        check(times.size()==prepared.checkouts.size(),"unique checkout IDs");check(leases==0,"pre-render releases suites");
        if(mode==0)check(times.size()==1 && times.begin()->second==30,"Current Time deduplicates all particles");
        if(mode==7)check(times.size()>1,"Freeze Frame retains birth times");
    }
    reset();auto scene=graph();adapter::TexturePreparation prepared;
    auto encoded=core::serialize_graph(scene,core::particle_node_registry()).take_value();
    motion.enabled=true;motion.samples.push_back({{30,30},encoded,{}});motion.samples.push_back({{30,30},encoded,{}});
    check(!adapter::prepare_texture_resources(&in,&out,&extra,scene,motion,1080,1,cancel,prepared) && prepared.checkouts.size()==1,"shutter samples share duplicate requests");
    motion={};reset();prepared={};
    check(!adapter::prepare_texture_resources(&in,&out,&extra,scene,motion,1080,1,cancel,prepared),"prepare ordinary frame");
    if(prepared.checkouts.empty()){std::printf("Fixture has no texture checkouts; %s\n",out.return_msg);return 1;}
    PF_SmartRenderCallbacks render_callbacks{};
    render_callbacks.checkout_layer_pixels=[](PF_ProgPtr,A_long id,PF_EffectWorld** out)->PF_Err{
        if(id==checkout_fail)return PF_Err_BAD_CALLBACK_PARAM;++pixels_out;*out=&world;if(cancel_after_checkout)cancelled=true;return 0;};
    render_callbacks.checkin_layer_pixels=[](PF_ProgPtr,A_long)->PF_Err{++pixels_in;return 0;};
    PF_SmartRenderExtra render_extra{};render_extra.cb=&render_callbacks;
    world.width=world.height=2;world.rowbytes=2*sizeof(PF_PixelFloat);world.data=reinterpret_cast<PF_Pixel*>(pixels.data());
    adapter::TextureStaging staging;
    check(!adapter::stage_texture_resources(&in,&out,&render_extra,prepared,adapter::HostBitDepth::bpc32,cancel,staging),"F32 staging");
    check(staging.frames.size()==1 && staging.pixels[0][0]==.2f && staging.pixels[0][3]==1,"ARGB converted to owned premult RGBA");
    check(pixels_in==pixels_out && pixels_out==1,"checkout/checkin paired");
    std::array<PF_Pixel,4> pixels8{};pixels8[0]={255,128,64,32};world.data=pixels8.data();world.rowbytes=2*sizeof(PF_Pixel);staging={};
    check(!adapter::stage_texture_resources(&in,&out,&render_extra,prepared,adapter::HostBitDepth::bpc8,cancel,staging) &&
        std::abs(staging.pixels[0][0]-128.f/255)<1e-6 && staging.pixels[0][3]==1,"8bpc host normalization");
    std::array<PF_Pixel16,4> pixels16{};pixels16[0]={32768,16384,8192,4096};world.data=reinterpret_cast<PF_Pixel*>(pixels16.data());world.rowbytes=2*sizeof(PF_Pixel16);staging={};
    check(!adapter::stage_texture_resources(&in,&out,&render_extra,prepared,adapter::HostBitDepth::bpc16,cancel,staging) &&
        staging.pixels[0][0]==.5f && staging.pixels[0][3]==1,"16bpc uses AE 32768 scale");
    world.data=reinterpret_cast<PF_Pixel*>(pixels.data());world.rowbytes=2*sizeof(PF_PixelFloat);
    cancel_after_checkout=true;staging={};
    check(adapter::stage_texture_resources(&in,&out,&render_extra,prepared,adapter::HostBitDepth::bpc32,cancel,staging)==PF_Interrupt_CANCEL &&
        pixels_in==pixels_out,"cancelled pixel copy checks in");cancel_after_checkout=cancelled=false;
    world.origin_x=-1;staging={};
    check(!adapter::stage_texture_resources(&in,&out,&render_extra,prepared,adapter::HostBitDepth::bpc32,cancel,staging),"negative source origin clipped");
    check(staging.pixels[0][0]==.3f && staging.pixels[0][7]==0,"crop placement preserves source canvas");
    world.origin_x=0;world.width=world.height=1;world.rowbytes=sizeof(PF_PixelFloat);staging={};
    check(!adapter::stage_texture_resources(&in,&out,&render_extra,prepared,adapter::HostBitDepth::bpc32,cancel,staging),"downsampled source");
    check(staging.frames[0].width==1 && staging.frames[0].height==1,"observed world/rect ratio used");
    world.data=nullptr;staging={};
    check(!adapter::stage_texture_resources(&in,&out,&render_extra,prepared,adapter::HostBitDepth::bpc32,cancel,staging) && staging.pixels[0][3]==0,"empty world becomes transparent");
    world.data=reinterpret_cast<PF_Pixel*>(pixels.data());world.width=32768;world.height=32768;world.rowbytes=32768*sizeof(PF_PixelFloat);staging={};
    auto huge=prepared;huge.sources[0].width=huge.sources[0].height=32768;huge.checkouts[0].result.result_rect={0,0,32768,32768};
    check(adapter::stage_texture_resources(&in,&out,&render_extra,huge,adapter::HostBitDepth::bpc32,cancel,staging)!=0,"byte budget before pixel access");
    check(pixels_in==pixels_out,"failed staging checks pixels in");
    reset();missing=true;prepared={};
    check(adapter::prepare_texture_resources(&in,&out,&extra,scene,motion,1080,1,cancel,prepared)!=0 && leases==0,"deleted resource rejects without suite leak");
    reset();self_reference=true;prepared={};
    check(adapter::prepare_texture_resources(&in,&out,&extra,scene,motion,1080,1,cancel,prepared)!=0,"self-reference rejects");
    reset();cancelled=true;prepared={};
    check(adapter::prepare_texture_resources(&in,&out,&extra,scene,motion,1080,1,cancel,prepared)==PF_Interrupt_CANCEL && leases==0,"cancel releases suites");
    reset();reverse_clock=true;in.current_time=90;prepared={};
    check(!adapter::prepare_texture_resources(&in,&out,&extra,scene,motion,1080,1,cancel,prepared),"reversed owner clock");
    check(std::abs(prepared.sources[0].start_seconds-(2+1./30))<1e-8 &&
        std::abs(prepared.sources[0].end_seconds-(4+1./30))<1e-8,"reverse grid excludes comp out-point and includes comp in-point");
    for(const auto& [id,time]:times)check(time>60 && time<=120,"reversed checkout stays within visible comp interval");
    std::printf("texture_adapter_tests: %d checks, %d failures; AE2023 qualification remains open\n",checks,failures);
    return failures?1:0;
}
