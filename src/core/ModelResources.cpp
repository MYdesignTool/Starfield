#include "starfield/core/ModelResources.hpp"
#include "starfield/core/ParticleMotionPose.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <new>
#include <numeric>

namespace starfield::core {
namespace {
bool finite(Vec3 v) noexcept {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
bool zero(const ModelResourceId& id) noexcept {return std::all_of(id.begin(),id.end(),[](auto b){return b==0;});}
bool affine(const std::array<double,16>& m,double bound) noexcept {
    return m[3]==0&&m[7]==0&&m[11]==0&&m[15]==1&&
        std::all_of(m.begin(),m.end(),[&](double x){return std::isfinite(x)&&std::abs(x)<=bound;});
}
Result<std::uint32_t> checksum(std::span<const std::byte> bytes,const Cancellation& cancel) noexcept {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=0;i<bytes.size();++i) {
        if(i%4096==0&&cancel.is_cancelled())return Result<std::uint32_t>::failure(ErrorCode::cancelled,"Model checksum cancelled");
        crc^=std::to_integer<std::uint8_t>(bytes[i]);
        for(unsigned b=0;b<8;++b)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);
    }
    return Result<std::uint32_t>::success(crc^0xffffffffu);
}
Vec3 rotate(Vec3 v,Vec3 angles) noexcept {
    constexpr double radians=3.14159265358979323846/180;
    for(unsigned axis=0;axis<3;++axis) {
        const double a=(axis==0?-angles.x:axis==1?angles.y:angles.z)*radians,c=std::cos(a),s=std::sin(a);
        if(axis==0)v={v.x,c*v.y-s*v.z,s*v.y+c*v.z};
        else if(axis==1)v={c*v.x+s*v.z,v.y,-s*v.x+c*v.z};
        else v={c*v.x-s*v.y,s*v.x+c*v.y,v.z};
    }
    return v;
}
}
bool valid_model_instance(const ParticleModelInstance& instance) noexcept {
    return affine(instance.model_to_particle,kMaxModelLocalMatrixCoefficient);
}
Result<std::array<double,16>> model_local_matrix(const ModelLocalSettings& s,const ModelBounds& bounds) noexcept {
    using R=Result<std::array<double,16>>;
    const auto bounded=[](Vec3 v,double maximum){return finite(v)&&std::abs(v.x)<=maximum&&std::abs(v.y)<=maximum&&std::abs(v.z)<=maximum;};
    if(!bounded(s.origin,kMaxModelCoordinate)||!bounded(s.rotation_degrees,1e9)||!bounded(s.scale_percent,100000)||
        !bounded(bounds.minimum,kMaxModelCoordinate)||!bounded(bounds.maximum,kMaxModelCoordinate)||
        bounds.minimum.x>bounds.maximum.x||bounds.minimum.y>bounds.maximum.y||bounds.minimum.z>bounds.maximum.z)
        return R::failure(ErrorCode::invalid_request,"invalid Model author pose/bounds");
    const double extent=std::max({bounds.maximum.x-bounds.minimum.x,bounds.maximum.y-bounds.minimum.y,bounds.maximum.z-bounds.minimum.z});
    if(s.normalize&&extent<=0)return R::failure(ErrorCode::invalid_request,"cannot normalize empty Model bounds");
    const auto by=[&](double value,bool flip){return (flip?-value:value)/100/(s.normalize?extent:1);};
    const Vec3 scale{by(s.scale_percent.x,s.flip_x),by(s.scale_percent.y,s.flip_y),by(s.scale_percent.z,s.flip_z)};
    const auto x=rotate({scale.x,0,0},s.rotation_degrees),y=rotate({0,scale.y,0},s.rotation_degrees),z=rotate({0,0,scale.z},s.rotation_degrees);
    Vec3 origin=s.origin;
    if(s.center){const Vec3 center{std::midpoint(bounds.minimum.x,bounds.maximum.x),std::midpoint(bounds.minimum.y,bounds.maximum.y),std::midpoint(bounds.minimum.z,bounds.maximum.z)};
        const auto shifted=rotate({center.x*scale.x,center.y*scale.y,center.z*scale.z},s.rotation_degrees);
        origin={origin.x-shifted.x,origin.y-shifted.y,origin.z-shifted.z};}
    const std::array<double,16> result{x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,origin.x,origin.y,origin.z,1};
    if(!affine(result,kMaxModelLocalMatrixCoefficient))return R::failure(ErrorCode::invalid_request,"Model author matrix exceeds finite affine bounds");
    return R::success(result);
}
Result<std::size_t> model_geometry_encoded_size(const ModelGeometry& geometry) noexcept {
    using R=Result<std::size_t>;
    if(geometry.positions.empty()||geometry.triangles.empty())return R::failure(ErrorCode::invalid_request,"Model resource has no polygon geometry");
    if(geometry.positions.size()>kMaxModelVertices||geometry.texture_coordinates.size()>kMaxModelVertices ||
        geometry.normals.size()>kMaxModelVertices||geometry.triangles.size()>kMaxModelTriangles)
        return R::failure(ErrorCode::work_limit_exceeded,"Model resource counts exceeded");
    const auto size=32+32*geometry.positions.size()+24*geometry.texture_coordinates.size()+24*geometry.normals.size()+36*geometry.triangles.size();
    if(size>kMaxModelEncodedBytes)return R::failure(ErrorCode::work_limit_exceeded,"Model encoded byte cap exceeded");
    return R::success(size);
}
Result<std::vector<std::byte>> encode_model_geometry(const ModelGeometry& mesh,const Cancellation& cancel) noexcept {
    using R=Result<std::vector<std::byte>>;
    const auto size=model_geometry_encoded_size(mesh);if(!size.has_value())return R::failure(size.error());
    const auto valid=validate_model_geometry(mesh,cancel);if(!valid.has_value())return R::failure(valid.error());
    try {
        std::vector<std::byte> bytes;bytes.reserve(size.value());
        const auto append=[&](std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)bytes.push_back(static_cast<std::byte>((value>>(8*i))&255));};
        const auto scalar=[&](double value){append(std::bit_cast<std::uint64_t>(value),8);};
        const auto vector=[&](Vec3 value){scalar(value.x);scalar(value.y);scalar(value.z);};
        append(0x474d4653u,4);append(1,2);append(32,2);append(size.value(),4);append(0,4);
        for(auto count:{mesh.positions.size(),mesh.texture_coordinates.size(),mesh.normals.size(),mesh.triangles.size()})append(count,4);
        std::size_t work=0;
        for(const auto& p:mesh.positions) {
            if(work++%256==0&&cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model encoding cancelled");
            vector(p.value);scalar(p.weight);
        }
        for(const auto* attributes:{&mesh.texture_coordinates,&mesh.normals})for(auto v:*attributes) {
            if(work++%256==0&&cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model encoding cancelled");vector(v);
        }
        for(const auto& t:mesh.triangles) {
            if(work++%256==0&&cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model encoding cancelled");
            for(const auto& c:t.corners){append(c.position,4);append(c.texture,4);append(c.normal,4);}
        }
        const auto crc=checksum(std::span<const std::byte>(bytes).subspan(32),cancel);if(!crc.has_value())return R::failure(crc.error());
        for(unsigned i=0;i<4;++i)bytes[12+i]=static_cast<std::byte>((crc.value()>>(8*i))&255);
        return R::success(std::move(bytes));
    } catch(const std::bad_alloc&){return R::failure(ErrorCode::allocation_failed,"Model encoding allocation failed");}
    catch(...){return R::failure(ErrorCode::internal_failure,"Model encoding failed");}
}
Result<ModelGeometry> decode_model_geometry(std::span<const std::byte> bytes,const Cancellation& cancel) noexcept {
    using R=Result<ModelGeometry>;
    if(bytes.size()>kMaxModelEncodedBytes)return R::failure(ErrorCode::work_limit_exceeded,"Model encoded byte cap exceeded");
    if(bytes.size()<32)return R::failure(ErrorCode::invalid_request,"short Model resource header");
    if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model decoding cancelled");
    std::size_t at=0;
    const auto read=[&](unsigned n){std::uint64_t value=0;for(unsigned i=0;i<n;++i)value|=std::uint64_t(std::to_integer<std::uint8_t>(bytes[at++]))<<(8*i);return value;};
    const auto magic=read(4),version=read(2),header=read(2),length=read(4),crc=read(4);
    const auto positions=read(4),uvs=read(4),normals=read(4),triangles=read(4);
    if(magic!=0x474d4653u||version!=1||header!=32||length!=bytes.size()||!positions||!triangles)
        return R::failure(ErrorCode::invalid_request,"invalid Model resource header");
    if(positions>kMaxModelVertices||uvs>kMaxModelVertices||normals>kMaxModelVertices||triangles>kMaxModelTriangles)
        return R::failure(ErrorCode::work_limit_exceeded,"Model resource counts exceeded");
    if(32+32*positions+24*uvs+24*normals+36*triangles!=bytes.size())
        return R::failure(ErrorCode::invalid_request,"Model resource payload length mismatch");
    const auto actual=checksum(bytes.subspan(32),cancel);if(!actual.has_value())return R::failure(actual.error());
    if(actual.value()!=crc)return R::failure(ErrorCode::invalid_request,"Model resource checksum mismatch");
    try {
        ModelGeometry mesh;mesh.positions.resize(static_cast<std::size_t>(positions));
        mesh.texture_coordinates.resize(static_cast<std::size_t>(uvs));mesh.normals.resize(static_cast<std::size_t>(normals));
        mesh.triangles.resize(static_cast<std::size_t>(triangles));
        const auto scalar=[&](){return std::bit_cast<double>(read(8));};
        const auto vector=[&](){return Vec3{scalar(),scalar(),scalar()};};std::size_t work=0;
        for(auto& p:mesh.positions) {
            if(work++%256==0&&cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model decoding cancelled");p.value=vector();p.weight=scalar();
        }
        for(auto* attributes:{&mesh.texture_coordinates,&mesh.normals})for(auto& v:*attributes) {
            if(work++%256==0&&cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model decoding cancelled");v=vector();
        }
        for(auto& t:mesh.triangles) {
            if(work++%256==0&&cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model decoding cancelled");
            for(auto& c:t.corners){c.position=static_cast<std::uint32_t>(read(4));c.texture=static_cast<std::uint32_t>(read(4));c.normal=static_cast<std::uint32_t>(read(4));}
        }
        const auto valid=validate_model_geometry(mesh,cancel);if(!valid.has_value())return R::failure(valid.error());
        mesh.bounds=valid.value();return R::success(std::move(mesh));
    } catch(const std::bad_alloc&){return R::failure(ErrorCode::allocation_failed,"Model decoding allocation failed");}
    catch(...){return R::failure(ErrorCode::internal_failure,"Model decoding failed");}
}
static Result<std::size_t> preflight_model_resources(std::span<const ModelResource> sources,const Cancellation& cancel) noexcept {
    using R=Result<std::size_t>;
    if(sources.size()>kMaxModelSources)return R::failure(ErrorCode::work_limit_exceeded,"Model source count exceeded");
    std::array<ModelResourceId,kMaxModelSources> ids{};std::uint64_t bytes=0;
    for(std::size_t i=0;i<sources.size();++i) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model source validation cancelled");
        if(zero(sources[i].id))return R::failure(ErrorCode::invalid_request,"Model source ID is zero");ids[i]=sources[i].id;
        const auto size=model_geometry_encoded_size(sources[i].geometry);if(!size.has_value())return R::failure(size.error());
        if(size.value()>kMaxModelSourceBytes-bytes)return R::failure(ErrorCode::work_limit_exceeded,"Model source bytes exceeded");bytes+=size.value();
    }
    std::sort(ids.begin(),ids.begin()+static_cast<std::ptrdiff_t>(sources.size()));
    for(std::size_t i=1;i<sources.size();++i)if(ids[i]==ids[i-1])return R::failure(ErrorCode::invalid_request,"duplicate Model source ID");
    return R::success(sources.size());
}
Result<std::size_t> validate_model_resources(std::span<const ModelResource> sources,const Cancellation& cancel) noexcept {
    const auto preflight=preflight_model_resources(sources,cancel);if(!preflight.has_value())return preflight;
    for(const auto& source:sources){const auto valid=validate_model_geometry(source.geometry,cancel);if(!valid.has_value())return Result<std::size_t>::failure(valid.error());}
    return preflight;
}
Result<std::vector<ModelGeometryLease>> compile_model_resources(std::span<const ModelResource> sources,const Cancellation& cancel) noexcept {
    using R=Result<std::vector<ModelGeometryLease>>;
    const auto preflight=preflight_model_resources(sources,cancel);if(!preflight.has_value())return R::failure(preflight.error());
    try {
        std::vector<ModelGeometryLease> leases;leases.reserve(sources.size());
        for(const auto& source:sources) {
            const auto lease=compile_model_geometry(source.geometry,cancel);if(!lease.has_value())return R::failure(lease.error());
            leases.push_back(lease.value());
        }
        return R::success(std::move(leases));
    } catch(const std::bad_alloc&){return R::failure(ErrorCode::allocation_failed,"Model resource lease allocation failed");}
    catch(...){return R::failure(ErrorCode::internal_failure,"Model resource compilation failed");}
}
Result<std::array<double,16>> model_particle_matrix(const ParticleInstance& p,const FrameSpec& frame,
    std::span<const ParticleSpriteBasis> bases,const ParticleModelInstance& instance) noexcept {
    using R=Result<std::array<double,16>>;
    if(!frame.layer_width||!frame.layer_height||!std::isfinite(frame.pixel_aspect_ratio)||frame.pixel_aspect_ratio<=0 ||
        !finite(p.position)||!finite(p.rotation_degrees)||!std::isfinite(p.size_pixels)||p.size_pixels<0 ||
        p.size_pixels>kMaxParticleSize||p.up_axis>2||!std::isfinite(p.anchor_x_percent)||!std::isfinite(p.anchor_y_percent)||
        p.anchor_x_percent<0||p.anchor_x_percent>100||p.anchor_y_percent<0||p.anchor_y_percent>100 || !valid_model_instance(instance)||!valid_particle_motion_pose(p.motion_pose))
        return R::failure(ErrorCode::invalid_request,"invalid Model particle pose");
    ParticleSpriteBasis basis{1,0,0, 0,1,0, 0,0,1};
    if(p.sprite_basis_index) {
        if(p.sprite_basis_index>bases.size()||!valid_particle_sprite_basis(bases[p.sprite_basis_index-1]))
            return R::failure(ErrorCode::invalid_request,"invalid Model Transform basis");basis=bases[p.sprite_basis_index-1];
    }
    auto angles=p.rotation_degrees;if(p.limit_to_2d)angles.x=angles.y=0;
    const auto axis=[&](Vec3 v) {
        if(!p.limit_to_2d) {
            if(p.up_axis==0)v={-v.z,v.y,v.x};else if(p.up_axis==1)v={v.x,v.z,-v.y};
        }
        v=rotate(v,angles);
        Vec3 world{basis[0]*v.x+basis[1]*v.y+basis[2]*v.z,
            basis[3]*v.x+basis[4]*v.y+basis[5]*v.z,basis[6]*v.x+basis[7]*v.y+basis[8]*v.z};
        if(p.motion_pose!=kIdentityMotionPose)world=motion_pose_axis(p.motion_pose,world);
        return Vec3{world.x*p.size_pixels/frame.pixel_aspect_ratio,-world.y*p.size_pixels,world.z*p.size_pixels};
    };
    std::array<double,16> matrix{};matrix[15]=1;
    const auto& local=instance.model_to_particle;
    for(unsigned row=0;row<3;++row) {const auto v=axis({local[row*4],local[row*4+1],local[row*4+2]});matrix[row*4]=v.x;matrix[row*4+1]=v.y;matrix[row*4+2]=v.z;}
    const auto offset=axis({local[12]+.5-p.anchor_x_percent/100,local[13]+.5-p.anchor_y_percent/100,local[14]});
    matrix[12]=frame.layer_width*.5+p.position.x*frame.layer_height/frame.pixel_aspect_ratio+offset.x;
    matrix[13]=(.5-p.position.y)*frame.layer_height+offset.y;matrix[14]=p.position.z*frame.layer_height+offset.z;
    if(!affine(matrix,kMaxModelLayerMatrixCoefficient))return R::failure(ErrorCode::invalid_request,"Model composed pose is nonfinite or outside bounds");
    return R::success(matrix);
}
}
