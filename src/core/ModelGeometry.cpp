#include "starfield/core/ModelGeometry.hpp"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <new>

namespace starfield::core {
Result<ModelGeometryLease> compile_model_geometry(const ModelGeometry& mesh,const Cancellation& cancel) noexcept {
    const auto valid=validate_model_geometry(mesh,cancel);
    if(!valid.has_value())return Result<ModelGeometryLease>::failure(valid.error());
    return Result<ModelGeometryLease>::success(ModelGeometryLease(mesh));
}
namespace {
using MeshResult=Result<ModelGeometry>;
constexpr double epsilon=1e-12;
bool whitespace(char c) noexcept {return c==' ' || c=='\t' || c=='\r' || c=='\n' || c=='\f' || c=='\v';}
bool bounded(double v) noexcept {return std::isfinite(v) && std::abs(v)<=kMaxModelCoordinate;}
bool bounded(Vec3 v) noexcept {return bounded(v.x) && bounded(v.y) && bounded(v.z);}
Vec3 subtract(Vec3 a,Vec3 b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 cross(Vec3 a,Vec3 b) noexcept {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double dot(Vec3 a,Vec3 b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
double maximum(Vec3 v) noexcept {return std::max({std::abs(v.x),std::abs(v.y),std::abs(v.z)});}
Vec3 scale(Vec3 v,double by) noexcept {return {v.x*by,v.y*by,v.z*by};}
Vec3 divide(Vec3 v,double by) noexcept {return {v.x/by,v.y/by,v.z/by};}
bool triangle_valid(Vec3 a,Vec3 b,Vec3 c) noexcept {
    auto u=subtract(b,a),v=subtract(c,a);const auto extent=std::max(maximum(u),maximum(v));
    return extent>0 && maximum(cross(divide(u,extent),divide(v,extent)))>epsilon;
}
struct Tokens {
    std::string_view remaining;
    std::string_view next() noexcept {
        while(!remaining.empty() && whitespace(remaining.front()))remaining.remove_prefix(1);
        const auto size=remaining.find_first_of(" \t\r\n\f\v");
        const auto token=remaining.substr(0,size);
        remaining.remove_prefix(token.size());return token;
    }
};
bool number(std::string_view token,double& value) noexcept {
    if(!token.empty() && token.front()=='+')token.remove_prefix(1);
    if(token.empty())return false;
    const auto parsed=std::from_chars(token.data(),token.data()+token.size(),value,std::chars_format::general);
    return parsed.ec==std::errc{} && parsed.ptr==token.data()+token.size() && bounded(value);
}
bool index(std::string_view token,std::size_t size,std::uint32_t& result) noexcept {
    if(!token.empty() && token.front()=='+')token.remove_prefix(1);
    if(token.empty())return false;
    std::int64_t raw{};const auto parsed=std::from_chars(token.data(),token.data()+token.size(),raw);
    if(parsed.ec!=std::errc{} || parsed.ptr!=token.data()+token.size() || raw==0)return false;
    if(raw>0) {if(static_cast<std::uint64_t>(raw)>size)return false;result=static_cast<std::uint32_t>(raw-1);}
    else {if(raw < -static_cast<std::int64_t>(size))return false;result=static_cast<std::uint32_t>(static_cast<std::int64_t>(size)+raw);}
    return true;
}
bool corner(std::string_view token,const ModelGeometry& mesh,ModelCorner& result) noexcept {
    const auto slash=token.find('/');
    if(!index(token.substr(0,slash),mesh.positions.size(),result.position))return false;
    if(slash==std::string_view::npos)return true;
    token.remove_prefix(slash+1);const auto second=token.find('/');
    const auto uv=token.substr(0,second);
    if(!uv.empty() && !index(uv,mesh.texture_coordinates.size(),result.texture))return false;
    if(second==std::string_view::npos)return !uv.empty();
    token.remove_prefix(second+1);
    return !token.empty() && index(token,mesh.normals.size(),result.normal);
}
struct Point {double x{},y{};};
double orient(Point a,Point b,Point c) noexcept {return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);}
bool on_segment(Point a,Point b,Point p) noexcept {
    return std::abs(orient(a,b,p))<=epsilon && p.x>=std::min(a.x,b.x)-epsilon &&
        p.x<=std::max(a.x,b.x)+epsilon && p.y>=std::min(a.y,b.y)-epsilon && p.y<=std::max(a.y,b.y)+epsilon;
}
bool intersects(Point a,Point b,Point c,Point d) noexcept {
    const auto x=orient(a,b,c),y=orient(a,b,d),z=orient(c,d,a),w=orient(c,d,b);
    if(((x>epsilon && y<-epsilon) || (x<-epsilon && y>epsilon)) &&
       ((z>epsilon && w<-epsilon) || (z<-epsilon && w>epsilon)))return true;
    return on_segment(a,b,c) || on_segment(a,b,d) || on_segment(c,d,a) || on_segment(c,d,b);
}
struct Budget {
    const Cancellation& cancellation;
    std::uint64_t remaining;
    CoreError error{};
    bool spend() noexcept {
        if(cancellation.is_cancelled()){error=make_error(ErrorCode::cancelled,"OBJ parsing cancelled");return false;}
        if(!remaining){error=make_error(ErrorCode::work_limit_exceeded,"OBJ polygon work budget exceeded");return false;}
        --remaining;return true;
    }
};
Result<bool> triangulate(ModelGeometry& mesh,const std::array<ModelCorner,256>& corners,
                        std::size_t count,const ModelParseLimits& limits,Budget& budget) {
    using R=Result<bool>;
    if(count<3)return R::failure(ErrorCode::invalid_request,"OBJ face needs at least three corners");
    if(count-2>limits.triangles-mesh.triangles.size())return R::failure(ErrorCode::work_limit_exceeded,"OBJ triangle limit exceeded");
    const auto origin=mesh.positions[corners[0].position].value;
    std::array<Vec3,256> local{};double extent=0;
    for(std::size_t i=0;i<count;++i) {local[i]=subtract(mesh.positions[corners[i].position].value,origin);extent=std::max(extent,maximum(local[i]));}
    if(!(extent>0))return R::failure(ErrorCode::invalid_request,"OBJ face has no area");
    for(std::size_t i=0;i<count;++i)local[i]=divide(local[i],extent);
    Vec3 normal{};
    for(std::size_t i=0;i<count;++i) {
        const auto term=cross(local[i],local[(i+1)%count]);normal.x+=term.x;normal.y+=term.y;normal.z+=term.z;
        for(std::size_t j=i+1;j<count;++j) {
            if(!budget.spend())return R::failure(budget.error);
            if(maximum(subtract(local[i],local[j]))<=epsilon)return R::failure(ErrorCode::invalid_request,"OBJ face repeats a position");
        }
    }
    const double magnitude=std::sqrt(dot(normal,normal));
    if(!(magnitude>epsilon))return R::failure(ErrorCode::invalid_request,"OBJ face has no oriented area");
    normal=scale(normal,1/magnitude);
    std::array<Point,256> points{};const auto ax=std::abs(normal.x),ay=std::abs(normal.y),az=std::abs(normal.z);
    for(std::size_t i=0;i<count;++i) {
        if(std::abs(dot(normal,local[i]))>1e-8)return R::failure(ErrorCode::invalid_request,"OBJ polygon is not planar");
        points[i]=ax>=ay && ax>=az?Point{local[i].y,local[i].z}:ay>=az?Point{local[i].x,local[i].z}:Point{local[i].x,local[i].y};
    }
    double area=0;
    for(std::size_t i=0;i<count;++i)area+=points[i].x*points[(i+1)%count].y-points[i].y*points[(i+1)%count].x;
    if(std::abs(area)<=epsilon)return R::failure(ErrorCode::invalid_request,"OBJ face projection has no area");
    const double sign=area>0?1:-1;
    for(std::size_t i=0;i<count;++i)for(std::size_t j=i+1;j<count;++j) {
        if(j==i+1 || (i==0 && j==count-1))continue;
        if(!budget.spend())return R::failure(budget.error);
        if(intersects(points[i],points[(i+1)%count],points[j],points[(j+1)%count]))
            return R::failure(ErrorCode::invalid_request,"OBJ face intersects itself");
    }
    std::array<std::size_t,256> live{};for(std::size_t i=0;i<count;++i)live[i]=i;
    std::size_t remaining=count;
    while(remaining>3) {
        bool found=false;
        for(std::size_t i=0;i<remaining;++i) {
            if(!budget.spend())return R::failure(budget.error);
            const auto a=live[(i+remaining-1)%remaining],b=live[i],c=live[(i+1)%remaining];
            if(sign*orient(points[a],points[b],points[c])<=epsilon)continue;
            bool empty=true;
            for(std::size_t j=0;j<remaining;++j) {
                const auto p=live[j];if(p==a || p==b || p==c)continue;
                if(!budget.spend())return R::failure(budget.error);
                if(sign*orient(points[a],points[b],points[p])>=-epsilon &&
                   sign*orient(points[b],points[c],points[p])>=-epsilon &&
                   sign*orient(points[c],points[a],points[p])>=-epsilon){empty=false;break;}
            }
            if(!empty)continue;
            mesh.triangles.push_back({{corners[a],corners[b],corners[c]}});
            for(std::size_t j=i;j+1<remaining;++j)live[j]=live[j+1];
            --remaining;found=true;break;
        }
        if(!found)return R::failure(ErrorCode::invalid_request,"OBJ polygon cannot be triangulated");
    }
    if(sign*orient(points[live[0]],points[live[1]],points[live[2]])<=epsilon)
        return R::failure(ErrorCode::invalid_request,"OBJ final triangle has no area");
    mesh.triangles.push_back({{corners[live[0]],corners[live[1]],corners[live[2]]}});
    return R::success(true);
}
Result<bool> parse_line(std::string_view line,ModelGeometry& mesh,const ModelParseLimits& limits,Budget& budget) {
    using R=Result<bool>;Tokens tokens{line};const auto command=tokens.next();
    if(command.empty())return R::success(true);
    if(command=="v" || command=="vt" || command=="vn") {
        std::array<double,4> values{};std::size_t count=0;
        for(auto token=tokens.next();!token.empty();token=tokens.next()) {
            if(count==values.size() || !number(token,values[count]))return R::failure(ErrorCode::invalid_request,"Invalid OBJ coordinate");
            ++count;
        }
        if(command=="v") {
            if(count<3 || count>4)return R::failure(ErrorCode::invalid_request,"OBJ position needs XYZ and optional weight");
            if(mesh.positions.size()>=limits.positions)return R::failure(ErrorCode::work_limit_exceeded,"OBJ position limit exceeded");
            mesh.positions.push_back({{values[0],values[1],values[2]},count==4?values[3]:1});
        } else if(command=="vt") {
            if(!count || count>3)return R::failure(ErrorCode::invalid_request,"OBJ texture coordinate needs one to three values");
            if(mesh.texture_coordinates.size()>=limits.attributes)return R::failure(ErrorCode::work_limit_exceeded,"OBJ texture coordinate limit exceeded");
            mesh.texture_coordinates.push_back({values[0],values[1],values[2]});
        } else {
            if(count!=3 || maximum({values[0],values[1],values[2]})==0)return R::failure(ErrorCode::invalid_request,"Invalid OBJ normal");
            if(mesh.normals.size()>=limits.attributes)return R::failure(ErrorCode::work_limit_exceeded,"OBJ normal limit exceeded");
            mesh.normals.push_back({values[0],values[1],values[2]});
        }
        return R::success(true);
    }
    if(command=="f") {
        std::array<ModelCorner,256> corners{};std::size_t count=0;
        for(auto token=tokens.next();!token.empty();token=tokens.next()) {
            if(count>=limits.face_corners)return R::failure(ErrorCode::work_limit_exceeded,"OBJ face corner limit exceeded");
            if(!corner(token,mesh,corners[count]))return R::failure(ErrorCode::invalid_request,"Invalid OBJ face index");
            ++count;
        }
        return triangulate(mesh,corners,count,limits,budget);
    }
    if(command=="o" || command=="g" || command=="s" || command=="mg" || command=="usemtl" || command=="mtllib")return R::success(true);
    return R::failure(ErrorCode::unsupported_format,"Unsupported OBJ statement");
}
bool valid_limits(const ModelParseLimits& limits) noexcept {
    return limits.source_bytes<=8*1024*1024 && limits.logical_line_bytes<=64*1024 &&
        limits.positions<=kMaxModelVertices && limits.attributes<=kMaxModelVertices &&
        limits.triangles<=kMaxModelTriangles && limits.face_corners<=256 && limits.triangulation_work<=16'000'000;
}
} // namespace

Result<ModelBounds> validate_model_geometry(const ModelGeometry& mesh,const Cancellation& cancellation) noexcept {
    using R=Result<ModelBounds>;
    if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model validation cancelled");
    if(mesh.positions.empty() || mesh.triangles.empty())return R::failure(ErrorCode::invalid_request,"Model has no polygon geometry");
    if(mesh.positions.size()>kMaxModelVertices || mesh.normals.size()>kMaxModelVertices || mesh.texture_coordinates.size()>kMaxModelVertices ||
       mesh.triangles.size()>kMaxModelTriangles)return R::failure(ErrorCode::work_limit_exceeded,"Model geometry limit exceeded");
    std::size_t operations=0;const auto cancelled=[&]{return ((operations++ & 255)==0) && cancellation.is_cancelled();};
    for(const auto& p:mesh.positions) {
        if(cancelled())return R::failure(ErrorCode::cancelled,"Model validation cancelled");
        if(!bounded(p.value) || !bounded(p.weight))return R::failure(ErrorCode::invalid_request,"Invalid model position");
    }
    for(const auto& n:mesh.normals) {
        if(cancelled())return R::failure(ErrorCode::cancelled,"Model validation cancelled");
        if(!bounded(n) || maximum(n)==0)return R::failure(ErrorCode::invalid_request,"Invalid model normal");
    }
    for(const auto& uv:mesh.texture_coordinates) {
        if(cancelled())return R::failure(ErrorCode::cancelled,"Model validation cancelled");
        if(!bounded(uv))return R::failure(ErrorCode::invalid_request,"Invalid model texture coordinate");
    }
    ModelBounds bounds{};bool first=true;
    for(const auto& triangle:mesh.triangles) {
        if(cancelled())return R::failure(ErrorCode::cancelled,"Model validation cancelled");
        for(const auto& c:triangle.corners) {
            if(c.position>=mesh.positions.size() || (c.texture!=kMissingModelAttribute && c.texture>=mesh.texture_coordinates.size()) ||
               (c.normal!=kMissingModelAttribute && c.normal>=mesh.normals.size()))return R::failure(ErrorCode::invalid_request,"Invalid model corner index");
            const auto p=mesh.positions[c.position].value;
            if(first){bounds={p,p};first=false;}
            else {
                bounds.minimum={std::min(bounds.minimum.x,p.x),std::min(bounds.minimum.y,p.y),std::min(bounds.minimum.z,p.z)};
                bounds.maximum={std::max(bounds.maximum.x,p.x),std::max(bounds.maximum.y,p.y),std::max(bounds.maximum.z,p.z)};
            }
        }
        if(!triangle_valid(mesh.positions[triangle.corners[0].position].value,mesh.positions[triangle.corners[1].position].value,
            mesh.positions[triangle.corners[2].position].value))return R::failure(ErrorCode::invalid_request,"Degenerate model triangle");
    }
    return R::success(bounds);
}

Result<ModelGeometry> make_unit_cube() noexcept try {
    ModelGeometry mesh;
    for(const auto v:std::array<Vec3,8>{{{-.5,-.5,-.5},{.5,-.5,-.5},{.5,.5,-.5},{-.5,.5,-.5},
                                     {-.5,-.5,.5},{.5,-.5,.5},{.5,.5,.5},{-.5,.5,.5}}})mesh.positions.push_back({v,1});
    mesh.normals={{0,0,-1},{0,0,1},{0,-1,0},{0,1,0},{-1,0,0},{1,0,0}};
    mesh.texture_coordinates={{0,0,0},{1,0,0},{1,1,0},{0,1,0}};
    const std::array<std::array<std::uint32_t,4>,6> faces{{{0,3,2,1},{4,5,6,7},{0,1,5,4},{3,7,6,2},{0,4,7,3},{1,2,6,5}}};
    for(std::uint32_t face=0;face<faces.size();++face) {
        const auto& f=faces[face];
        for(const auto indices:std::array<std::array<std::uint32_t,3>,2>{{{0,1,2},{0,2,3}}}) {
            ModelTriangle triangle;
            for(std::size_t i=0;i<3;++i)triangle.corners[i]={f[indices[i]],indices[i],face};
            mesh.triangles.push_back(triangle);
        }
    }
    mesh.bounds={{-.5,-.5,-.5},{.5,.5,.5}};
    return MeshResult::success(std::move(mesh));
} catch(const std::bad_alloc&) {return MeshResult::failure(ErrorCode::allocation_failed,"Cube allocation failed");}
  catch(...) {return MeshResult::failure(ErrorCode::internal_failure,"Cube construction failed");}

Result<ModelGeometry> parse_model_obj(std::string_view source,const Cancellation& cancellation,
                                     ModelParseLimits limits,ModelParseLocation* location) noexcept try {
    if(location)*location={};
    if(cancellation.is_cancelled())return MeshResult::failure(ErrorCode::cancelled,"OBJ parsing cancelled");
    if(!valid_limits(limits))return MeshResult::failure(ErrorCode::invalid_request,"Invalid OBJ parse limits");
    if(source.size()>limits.source_bytes)return MeshResult::failure(ErrorCode::work_limit_exceeded,"OBJ source size exceeded");
    if(source.find('\0')!=std::string_view::npos)return MeshResult::failure(ErrorCode::invalid_request,"OBJ contains a null byte");
    if(source.starts_with("\xef\xbb\xbf"))source.remove_prefix(3);
    ModelGeometry mesh;Budget budget{cancellation,limits.triangulation_work};
    std::string logical;std::size_t line=0,first_line=1;bool continued=false;
    while(!source.empty()) {
        if(cancellation.is_cancelled())return MeshResult::failure(ErrorCode::cancelled,"OBJ parsing cancelled");
        ++line;if(logical.empty())first_line=line;if(location)location->line=first_line;
        const auto end=source.find('\n');auto physical=source.substr(0,end);
        source.remove_prefix(physical.size()+(end==std::string_view::npos?0:1));
        physical=physical.substr(0,physical.find('#'));
        while(!physical.empty() && whitespace(physical.back()))physical.remove_suffix(1);
        continued=!physical.empty() && physical.back()=='\\';if(continued)physical.remove_suffix(1);
        if(physical.size()>limits.logical_line_bytes || logical.size()>limits.logical_line_bytes-physical.size())
            return MeshResult::failure(ErrorCode::work_limit_exceeded,"OBJ logical line size exceeded");
        logical.append(physical);
        if(continued) {
            if(logical.size()>=limits.logical_line_bytes)return MeshResult::failure(ErrorCode::work_limit_exceeded,"OBJ logical line size exceeded");
            logical.push_back(' ');continue;
        }
        const auto parsed=parse_line(logical,mesh,limits,budget);
        if(!parsed.has_value())return MeshResult::failure(parsed.error());
        logical.clear();
    }
    if(continued)return MeshResult::failure(ErrorCode::invalid_request,"OBJ continuation has no next line");
    const auto checked=validate_model_geometry(mesh,cancellation);
    if(!checked.has_value())return MeshResult::failure(checked.error());
    mesh.bounds=checked.value();if(location)*location={};
    return MeshResult::success(std::move(mesh));
} catch(const std::bad_alloc&) {return MeshResult::failure(ErrorCode::allocation_failed,"OBJ allocation failed");}
  catch(...) {return MeshResult::failure(ErrorCode::internal_failure,"OBJ parser failed");}
} // namespace starfield::core
