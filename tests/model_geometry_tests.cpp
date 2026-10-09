#include "starfield/core/ModelGeometry.hpp"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <map>
#include <new>
#include <sstream>

static bool fail_allocation=false;
void* operator new(std::size_t size) {if(fail_allocation)throw std::bad_alloc();if(auto p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t size) {return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
namespace {
using namespace starfield::core;
int checks{},failures{};
void check(bool value,const char* label){++checks;if(!value){++failures;std::printf("FAILED: %s\n",label);}}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
bool near(double a,double b){return std::abs(a-b)<1e-9;}
const std::string triangle="v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
NeverCancelled never;
Result<ModelGeometry> parse(std::string_view text,ModelParseLimits limits={}){return parse_model_obj(text,never,limits);}
void rejected(std::string_view source,ErrorCode expected){const auto r=parse(source);check(!r.has_value() && r.error().code==expected,"malformed or unsupported OBJ rejects with typed reason");}
struct Cancel final:Cancellation {
    mutable std::size_t calls{};std::size_t stop{};
    explicit Cancel(std::size_t at):stop(at){}
    bool is_cancelled() const noexcept override {return ++calls>=stop;}
};
std::string polygon(unsigned count,bool reversed=false,double tx=0,double by=1) {
    std::ostringstream text;text<<std::setprecision(17);
    for(unsigned i=0;i<count;++i) {
        const double a=i*6.2831853071795864769/count,r=(i%2 && count>4)?0.6:1;
        text<<"v "<<tx+by*r*std::cos(a)<<" "<<tx+by*r*std::sin(a)<<" 0\n";
    }
    text<<"f";for(unsigned i=0;i<count;++i)text<<" "<<(reversed?count-i:i+1);text<<"\n";return text.str();
}
}
int main(){
    using namespace starfield::core;
    auto cube=make_unit_cube();check(cube.has_value(),"default cube created");if(!cube.has_value())return 1;
    const auto& c=cube.value();check(c.positions.size()==8 && c.normals.size()==6 && c.texture_coordinates.size()==4 && c.triangles.size()==12,"cube has complete indexed topology and face attributes");
    check(validate_model_geometry(c,never).has_value() && near(c.bounds.minimum.x,-.5) && near(c.bounds.maximum.z,.5),"unit cube validates and has side1 bounds");
    std::map<std::pair<unsigned,unsigned>,unsigned> edges;
    for(const auto& t:c.triangles) {
        const auto a=c.positions[t.corners[0].position].value,b=c.positions[t.corners[1].position].value,d=c.positions[t.corners[2].position].value;
        const auto n=cross(sub(b,a),sub(d,a)),centre=Vec3{(a.x+b.x+d.x)/3,(a.y+b.y+d.y)/3,(a.z+b.z+d.z)/3};
        check(dot(n,centre)>0 && dot(n,c.normals[t.corners[0].normal])>0,"cube winding and supplied face normal agree outward");
        for(unsigned i=0;i<3;++i) {
            const auto& corner=t.corners[i];check(corner.texture<4 && corner.normal<6,"cube corners retain UV and normal indices");
            auto x=corner.position,y=t.corners[(i+1)%3].position;if(x>y)std::swap(x,y);++edges[{x,y}];
        }
    }
    check(edges.size()==18 && std::all_of(edges.begin(),edges.end(),[](const auto& edge){return edge.second==2;}),"cube closed edges are shared exactly twice");
    auto basic=parse(triangle);check(basic.has_value() && basic.value().triangles.size()==1,"triangle OBJ reads");
    const std::string attrs="v 0 0 0 2\nv 1 0 0 0\nv 0 1 0 -2\nvt .25 .5 .75\nvt 1\nvt 0 1\nvn 0 0 2\n";
    for(const auto form:{"f 1 2 3\n","f 1/1 2/2 3/3\n","f 1//1 2//1 3//1\n","f 1/1/1 2/2/1 3/3/1\n",
                        "f -3/-3/-1 -2/-2/-1 -1/-1/-1\n"}) {
        auto r=parse(attrs+form);check(r.has_value(),"all four OBJ corner forms and relative indices read");if(!r.has_value())continue;
        const auto& mesh=r.value();check(mesh.positions[0].weight==2 && mesh.positions[2].weight==-2 && mesh.positions[1].value.x==1,
            "optional rational weights are retained without dividing polygon XYZ");
        check(mesh.texture_coordinates[0].z==.75 && mesh.texture_coordinates[1].y==0,"UVW and one-dimensional texture coordinates are retained");
    }
    auto full=parse(attrs+"f +1/+1/+1 +2/+2/+1 +3/+3/+1\n");
    check(full.has_value() && full.value().triangles[0].corners[2].texture==2 && full.value().triangles[0].corners[2].normal==0,"positive signs and corner attributes retained");
    const std::string continuation="\xef\xbb\xbf# OBJ data\r\nv +0 0 0\r\nv 1e0 0 0 # vertex\r\nv 0 1 0\r\no ignored\r\ng ignored\r\ns 1\r\nmtllib material.mtl\r\nusemtl flat\r\nf 1 \\\r\n 2 3 #face\r\n";
    check(parse(continuation).has_value(),"BOM CRLF comments names and continuation parse as data");
    const auto immutable=triangle;auto copy=parse(immutable);check(copy.has_value() && immutable==triangle,"parser does not modify input bytes");
    const std::string square="v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nv 500 500 500\nf 1 2 3 4\n";
    auto quad=parse(square);check(quad.has_value() && quad.value().triangles.size()==2 && quad.value().bounds.maximum.x==1 && quad.value().bounds.maximum.z==0,
        "bounds cover referenced faces rather than unrelated positions");
    const std::string concave_attributes="v 0 0 0\nv 2 0 0\nv 2 2 0\nv 1 1 0\nv 0 2 0\n"
        "vt 0\nvt .25\nvt .5\nvt .75\nvt 1\nvn 0 0 1\nvn 0 0 2\nvn 0 0 3\nvn 0 0 4\nvn 0 0 5\nf 1/5/1 2/4/2 3/3/3 4/2/4 5/1/5\n";
    auto with_attributes=parse(concave_attributes);check(with_attributes.has_value(),"concave face with distinct per-corner attributes parses");
    if(with_attributes.has_value())for(const auto& t:with_attributes.value().triangles)for(const auto& corner:t.corners)
        check(corner.normal==corner.position && corner.texture==4-corner.position,"ear clipping retains each original corner's separate attribute indices");
    for(unsigned size:{3u,4u,5u,7u,12u,32u,64u,128u,256u})for(bool reverse:{false,true}) {
        const auto source=polygon(size,reverse);auto a=parse(source),b=parse(source);
        check(a.has_value() && b.has_value(),"simple concave polygons in either winding triangulate");if(!a.has_value() || !b.has_value())continue;
        check(a.value().triangles.size()==size-2,"polygon triangulation preserves n-2 triangles");
        double triangle_area=0,polygon_area=0;
        for(unsigned i=0;i<size;++i){auto p=a.value().positions[i].value,q=a.value().positions[(i+1)%size].value;polygon_area+=p.x*q.y-p.y*q.x;}
        for(std::size_t i=0;i<a.value().triangles.size();++i) {
            const auto& t=a.value().triangles[i];const auto& other=b.value().triangles[i];
            const auto x=a.value().positions[t.corners[0].position].value,y=a.value().positions[t.corners[1].position].value,z=a.value().positions[t.corners[2].position].value;
            const auto area=cross(sub(y,x),sub(z,x)).z;triangle_area+=area;
            check(reverse?area<0:area>0,"triangle winding matches source polygon");
            for(unsigned j=0;j<3;++j)check(t.corners[j].position==other.corners[j].position,"ear selection and corner ordering are deterministic");
        }
        check(near(std::abs(triangle_area),std::abs(polygon_area)),"concave area is preserved without overlapping outside ears");
    }
    for(const auto source:{"","v nan 0 0\n","v inf 0 0\n","v 0 0\n","v 0 0 0 1 2\n","vn 0 0 0\n","vt\n"})rejected(source,ErrorCode::invalid_request);
    const std::string verts="v 0 0 0\nv 1 0 0\nv 0 1 0\n";
    for(const auto face:{"f 0 2 3\n","f 1 2 4\n","f -4 2 3\n","f 1 1 3\n","f 1 2\n","f 1/ 2 3\n","f 1// 2 3\n",
                         "f 1/1 2/1 3/1\n","f 1//1 2//1 3//1\n","f 1.0 2 3\n","f 9223372036854775808 2 3\n"})rejected(verts+face,ErrorCode::invalid_request);
    rejected("v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3\n",ErrorCode::invalid_request);
    rejected("v 0 0 0\nv 1 1 0\nv 0 1 0\nv 1 0 0\nf 1 2 3 4\n",ErrorCode::invalid_request);
    rejected("v 0 0 0\nv 1 0 0\nv 1 1 1\nv 0 1 0\nf 1 2 3 4\n",ErrorCode::invalid_request);
    rejected(verts+"f 1 2 \\\n",ErrorCode::invalid_request);
    for(const auto command:{"l 1 2","p 1","curv 0 1 1 2","surf 0 1 0 1 1 2 3","call file.obj","csh arbitrary text","unknown"})
        rejected(verts+command+"\nf 1 2 3\n",ErrorCode::unsupported_format);
    auto nul=triangle;nul.insert(3,1,'\0');rejected(nul,ErrorCode::invalid_request);
    rejected("v 1000000001 0 0\n",ErrorCode::invalid_request);
    for(unsigned which=0;which<7;++which) {
        ModelParseLimits limits;std::string source=triangle;
        switch(which){case 0:limits.source_bytes=3;break;case 1:limits.logical_line_bytes=3;break;case 2:limits.positions=2;break;
            case 3:limits.triangles=0;break;case 4:limits.face_corners=2;break;case 5:limits.triangulation_work=0;break;case 6:limits.attributes=0;source=attrs+"f 1 2 3\n";break;}
        auto limited=parse(source,limits);check(!limited.has_value() && limited.error().code==ErrorCode::work_limit_exceeded,"each configured parse budget is enforced with typed failure");
    }
    ModelParseLimits invalid;invalid.face_corners=257;check(!parse(triangle,invalid).has_value() && parse(triangle,invalid).error().code==ErrorCode::invalid_request,"caller cannot relax hard face bound");
    for(std::size_t at:{1u,4u,12u,50u}) {
        Cancel cancel(at);auto stopped=parse_model_obj(polygon(64),cancel);check(!stopped.has_value() && stopped.error().code==ErrorCode::cancelled,"cancelled polygon parsing publishes no partial geometry");
    }
    Cancel validation_cancel(2);check(!validate_model_geometry(c,validation_cancel).has_value(),"validation also cooperates with cancellation");
    ModelParseLocation location;auto located=parse_model_obj(verts+"f 1 2 4\n",never,{},&location);
    check(!located.has_value() && location.line==4,"malformed OBJ reports physical start line");
    check(parse_model_obj(triangle,never,{},&location).has_value() && location.line==0,"successful parsing clears diagnostic location");
    auto damaged=c;damaged.triangles[0].corners[0].normal=20;check(!validate_model_geometry(damaged,never).has_value(),"validation checks explicit normal indices");
    damaged=c;damaged.triangles[0].corners[0].texture=20;check(!validate_model_geometry(damaged,never).has_value(),"validation checks explicit UV indices");
    damaged=c;damaged.positions[0].value.x=std::numeric_limits<double>::quiet_NaN();check(!validate_model_geometry(damaged,never).has_value(),"validation rejects nonfinite positions");
    for(const auto source:{polygon(7,false,1e8,1),polygon(7,false,0,1e-5),polygon(7,false,0,1e-309)})
        check(parse(source).has_value(),"translation and tiny finite scales do not destabilize polygon predicates");
    check(parse("v 0 0 0\nv 1 0 0\nv 2 0 0\nv 2 1 0\nv 0 1 0\nf 1 2 3 4 5\n").has_value(),
        "collinear boundary corners triangulate without degenerate ears");
    rejected("v 0 0 0\nv 2 0 0\nv 2 2 0\nv 1 0 0\nv 0 2 0\nf 1 2 3 4 5\n",ErrorCode::invalid_request);
    const auto many_corners=polygon(257);auto over_face=parse(many_corners);
    check(!over_face.has_value() && over_face.error().code==ErrorCode::work_limit_exceeded,"hard face corner limit rejects at257");
    ModelParseLimits continuation_limit;continuation_limit.logical_line_bytes=8;
    auto long_continuation=parse("# ignored\nf 1 \\\n2 3 4 5\n",continuation_limit);
    check(!long_continuation.has_value() && long_continuation.error().code==ErrorCode::work_limit_exceeded,"continued logical line obeys combined length bound");
    damaged=c;damaged.triangles[0].corners[0].position=kMaxModelVertices;
    check(!validate_model_geometry(damaged,never).has_value(),"validation rejects invalid position indices");
    damaged=c;damaged.normals[0]={};check(!validate_model_geometry(damaged,never).has_value(),"validation rejects a zero source normal");
    damaged=c;damaged.texture_coordinates[0].x=std::numeric_limits<double>::infinity();
    check(!validate_model_geometry(damaged,never).has_value(),"validation rejects nonfinite UV attributes");
    damaged=c;damaged.triangles[0].corners[1]=damaged.triangles[0].corners[0];
    check(!validate_model_geometry(damaged,never).has_value(),"validation rejects degenerate triangles supplied directly");
    fail_allocation=true;auto allocation=parse_model_obj(triangle,never);auto allocation_cube=make_unit_cube();fail_allocation=false;
    check(!allocation.has_value() && allocation.error().code==ErrorCode::allocation_failed && !allocation_cube.has_value() && allocation_cube.error().code==ErrorCode::allocation_failed,
        "allocation failures are caught as typed errors");
    std::printf("Model geometry: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
