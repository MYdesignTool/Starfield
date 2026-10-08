// Included inside the standalone driver's anonymous namespace. No AE process.
int cloud_prepare_count{},cloud_release_count{},cloud_bad_scene{};
bool cloud_lease_live{};
SfGpuSprite cloud_sample_sprite{};
SfGpuCloudCircle cloud_sample_circle{};
std::uint32_t cloud_sample_offsets[2]{0,1},cloud_sample_indices[1]{0};
SfCoreStatus SF_CORE_CALL cloud_sample_prepare(const SfCoreRenderRequest*,SfCoreGpuSceneResult* scene) {
    check(!cloud_lease_live,"previous sample lease released before next prepare");
    cloud_lease_live=true;++cloud_prepare_count;
    cloud_sample_sprite={};cloud_sample_sprite.shape=2;cloud_sample_sprite.reserved[2]=1;
    if(cloud_bad_scene==1)cloud_sample_sprite.reserved[1]=1;
    if(cloud_bad_scene==2)cloud_sample_sprite.reserved[2]=core::kMaxCloudCircles+1;
    cloud_sample_circle={float(cloud_prepare_count),0,1,0};
    *scene={};scene->struct_size=sizeof(*scene);scene->status=SF_CORE_OK;scene->region={0,0,16,16};
    scene->tile_size=16;scene->tiles_x=scene->tiles_y=scene->sprite_count=scene->index_count=scene->cloud_circle_count=1;
    scene->sprites=&cloud_sample_sprite;scene->tile_offsets=cloud_sample_offsets;scene->tile_indices=cloud_sample_indices;
    scene->cloud_circles=cloud_bad_scene==3?nullptr:&cloud_sample_circle;
    scene->opaque_handle=&cloud_lease_live;return SF_CORE_OK;
}
void SF_CORE_CALL cloud_sample_release(SfCoreGpuSceneResult* scene) {
    check(cloud_lease_live && scene->opaque_handle==&cloud_lease_live,"release only live sample lease");
    cloud_lease_live=false;++cloud_release_count;*scene={};scene->struct_size=sizeof(*scene);
}
void test_cloud_motion_merge() {
    adapter::MotionExposure exposure;exposure.samples.resize(2);exposure.samples[0].time={0,1};exposure.samples[1].time={1,1};
    SfCoreApi api{};api.prepare_gpu_scene=cloud_sample_prepare;api.release_gpu_scene=cloud_sample_release;
    SfCoreRenderRequest request{};request.struct_size=sizeof(request);PF_OutData out{};
    core::NeverCancelled never;
    for(int invalid:{0,1,2,3}) {
        cloud_bad_scene=invalid;cloud_prepare_count=cloud_release_count=0;adapter::MotionGpuStorage storage;SfCoreGpuSceneResult merged{};
        const auto error=adapter::prepare_motion_gpu(nullptr,&out,api,request,exposure,merged,storage,never);
        check((error==0)==(invalid==0),"shutter Cloud ranges/null array checked");
        check(cloud_release_count==cloud_prepare_count && !cloud_lease_live,"shutter releases every sample on success/failure");
        if(invalid==0) {
            check(merged.sprite_count==2 && merged.cloud_circle_count==2,"merged exposure keeps both Cloud groups");
            check(merged.sprites[0].reserved[1]==0 && merged.sprites[1].reserved[1]==1,"Cloud offsets remapped per sample");
            check(merged.cloud_circles[0].x==1 && merged.cloud_circles[1].x==2,"member arrays copied before lease release");
            check(merged.tile_indices[0]==0 && merged.tile_indices[1]==1 && merged.tile_offsets[2]==1,"sample tile identity preserved");
        }
    }
    struct Stop:core::Cancellation {bool is_cancelled() const noexcept override{return true;}} stop;
    cloud_prepare_count=cloud_release_count=0;adapter::MotionGpuStorage storage;SfCoreGpuSceneResult merged{};
    check(adapter::prepare_motion_gpu(nullptr,&out,api,request,exposure,merged,storage,stop)==PF_Interrupt_CANCEL,"shutter cancel before Core lease");
    check(!cloud_prepare_count && !cloud_release_count,"cancel creates no lease");
}
