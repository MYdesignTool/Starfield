#include "starfield/core/MotionGeometry.hpp"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <numbers>
#include <thread>

namespace {
std::atomic<std::size_t> allocations{},fail_allocation{};
}
void* operator new(std::size_t size){
    const auto count=++allocations;
    if(fail_allocation.load()==count)throw std::bad_alloc{};
    if(auto* value=std::malloc(std::max<std::size_t>(1,size)))return value;
    throw std::bad_alloc{};
}
void operator delete(void* value) noexcept {std::free(value);}
void operator delete(void* value,std::size_t) noexcept {std::free(value);}
using namespace starfield::core;
namespace {
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
constexpr double pi=std::numbers::pi;
constexpr double quiet_nan=std::numeric_limits<double>::quiet_NaN();
constexpr double infinity=std::numeric_limits<double>::infinity();
bool near(double a,double b,double tolerance=1e-9){return std::isfinite(a) && std::isfinite(b) && std::abs(a-b)<=tolerance;}
bool near(Vec3 a,Vec3 b,double tolerance=1e-9){return near(a.x,b.x,tolerance) && near(a.y,b.y,tolerance) && near(a.z,b.z,tolerance);}
Vec3 mapped(const std::array<double,9>& m,Vec3 p){return {m[0]*p.x+m[1]*p.y+m[2]*p.z,m[3]*p.x+m[4]*p.y+m[5]*p.z,m[6]*p.x+m[7]*p.y+m[8]*p.z};}
double norm(Vec3 p){return std::hypot(p.x,p.y,p.z);}
template<class T> void error(const Result<T>& result,ErrorCode code){CHECK(!result.has_value());CHECK(result.error().code==code);}
void proper(const std::array<double,9>& matrix){
    for(unsigned r=0;r<3;++r)for(unsigned s=0;s<3;++s){double dot=0;for(unsigned c=0;c<3;++c)dot+=matrix[r*3+c]*matrix[s*3+c];CHECK(near(dot,r==s?1.:0.));}
    CHECK(near(matrix[0]*(matrix[4]*matrix[8]-matrix[5]*matrix[7])-matrix[1]*(matrix[3]*matrix[8]-matrix[5]*matrix[6])+matrix[2]*(matrix[3]*matrix[7]-matrix[4]*matrix[6]),1));
}
struct Interrupt final:Cancellation{
    mutable unsigned calls{};unsigned limit;
    explicit Interrupt(unsigned value):limit(value){}
    bool is_cancelled() const noexcept override {return calls++>=limit;}
};
}
int main(){
    NeverCancelled cancellation;
    CHECK(near(rotate_motion_circle({3,0,0},{},{0,0,2},pi/2).value(),{0,3,0}));
    CHECK(near(rotate_motion_circle({3,0,0},{},{0,0,2},-pi/2).value(),{0,-3,0}));
    CHECK(near(rotate_motion_circle({3,0,0},{},{0,1,0},pi/2).value(),{0,0,-3}));
    CHECK(near(rotate_motion_circle({13,20,7},{10,20,7},{0,0,1},pi/2).value(),{10,23,7}));
    CHECK(near(rotate_motion_circle({3,0,0},{},{0,0,1e-320},pi/2).value(),{0,3,0}));
    const double tiny=std::numeric_limits<double>::denorm_min();
    CHECK(near(rotate_motion_circle({3,0,0},{},{tiny,tiny,tiny},pi/3).value(),
        rotate_motion_circle({3,0,0},{},{1,1,1},pi/3).value()));
    const Vec3 original{.1,-.7,3.14159};
    for(double angle:{0.,2*pi,-2*pi}){const auto v=rotate_motion_circle(original,{.3,.4,.5},{1,2,3},angle).value();CHECK(v.x==original.x && v.y==original.y && v.z==original.z);}
    for(double angle:{quiet_nan,infinity,-infinity,1e12+1})error(rotate_motion_circle({1,2,3},{},{0,0,1},angle),ErrorCode::invalid_request);
    error(rotate_motion_circle({1,2,3},{},{},1),ErrorCode::invalid_request);
    error(rotate_motion_circle({quiet_nan,0,0},{},{0,0,1},1),ErrorCode::invalid_request);
    error(rotate_motion_circle({1e9,0,0},{-1e9,0,0},{0,0,1},pi),ErrorCode::invalid_request);
    for(unsigned i=0;i<201;++i){const double angle=(static_cast<double>(i)-100)*.017;
        const auto p=rotate_motion_circle(original,{.3,.4,.5},{1,2,3},angle);CHECK(p.has_value());
        CHECK(near(norm({p.value().x-.3,p.value().y-.4,p.value().z-.5}),norm({original.x-.3,original.y-.4,original.z-.5})));
        CHECK(near(rotate_motion_circle(p.value(),{.3,.4,.5},{1,2,3},-angle).value(),original));
    }
    const std::array<double,9> identity{1,0,0,0,1,0,0,0,1};
    CHECK(motion_look_at_rotation({0,0,1},{},{},1).value()==identity);
    CHECK(motion_look_at_rotation({0,0,1},{},{1,2,3},0).value()==identity);
    for(Vec3 forward:std::array<Vec3,5>{{{1,0,0},{0,1,0},{0,0,1},{1,2,3},{1e-320,0,0}}}){
        const double length=norm(forward);const Vec3 unit{forward.x/length,forward.y/length,forward.z/length};
        for(Vec3 target:std::array<Vec3,6>{{{1,2,3},{-4,-2,8},{0,0,1},{0,0,-1},unit,{-unit.x,-unit.y,-unit.z}}}){
            const auto result=motion_look_at_rotation(forward,{},target,1);CHECK(result.has_value());proper(result.value());
            const auto target_norm=norm(target);CHECK(near(mapped(result.value(),unit),{target.x/target_norm,target.y/target_norm,target.z/target_norm}));
        }
    }
    CHECK(near(mapped(motion_look_at_rotation({0,0,1},{},{1,0,0},.5).value(),{0,0,1}),{std::sqrt(.5),0,std::sqrt(.5)}));
    const auto tiny_rotation=motion_look_at_rotation({tiny,tiny,0},{},{tiny,0,tiny},1);CHECK(tiny_rotation.has_value());proper(tiny_rotation.value());
    CHECK(near(mapped(tiny_rotation.value(),{std::sqrt(.5),std::sqrt(.5),0}),{std::sqrt(.5),0,std::sqrt(.5)}));
    for(double weight:{-.01,1.01,quiet_nan,infinity})error(motion_look_at_rotation({0,0,1},{},{1,0,0},weight),ErrorCode::invalid_request);
    error(motion_look_at_rotation({},{},{1,0,0},1),ErrorCode::invalid_request);
    error(motion_look_at_rotation({0,0,1},{},{quiet_nan,0,0},1),ErrorCode::invalid_request);
    error(compile_motion_path({},cancellation),ErrorCode::invalid_request);
    std::array<Vec3,kMaxMotionPathControlPoints+1> too_many{};
    error(compile_motion_path(too_many,cancellation),ErrorCode::work_limit_exceeded);
    for(Vec3 bad:std::array<Vec3,3>{{{quiet_nan,0,0},{infinity,0,0},{1e9+1,0,0}}})error(compile_motion_path(std::span(&bad,1),cancellation),ErrorCode::invalid_request);
    std::array<Vec3,1> fixed{{{8,-3,11}}};
    auto fixed_result=compile_motion_path(fixed,cancellation);CHECK(fixed_result.has_value());CHECK(fixed_result.value().length()==0);
    for(double d:{-infinity, quiet_nan,infinity}){error(fixed_result.value().position_at_distance(d),ErrorCode::invalid_request);error(fixed_result.value().tangent_at_distance(d),ErrorCode::invalid_request);}
    for(double d:{-1e308,0.,1e308}){CHECK(near(fixed_result.value().position_at_distance(d).value(),fixed[0]));CHECK(near(fixed_result.value().tangent_at_distance(d).value(),{}));}
    std::array<Vec3,2> line{{{2,3,4},{12,3,4}}};
    auto line_result=compile_motion_path(line,cancellation);CHECK(line_result.has_value());CHECK(near(line_result.value().length(),10));
    CHECK(near(line_result.value().position_at_distance(-1).value(),line.front()));CHECK(near(line_result.value().position_at_distance(11).value(),line.back()));
    for(unsigned i=0;i<=100;++i){const auto d=i*.1;CHECK(near(line_result.value().position_at_distance(d).value(),{2+d,3,4}));CHECK(near(line_result.value().tangent_at_distance(d).value(),{1,0,0}));}
    std::array<Vec3,3> quadratic{{{0,0,0},{1,1,0},{2,0,0}}};
    const auto quadratic_result=compile_motion_path(quadratic,cancellation);CHECK(quadratic_result.has_value());
    CHECK(near(quadratic_result.value().length(),std::sqrt(2.)+std::asinh(1.),.000004));
    CHECK(near(quadratic_result.value().position_at_distance(quadratic_result.value().length()/2).value(),{1,.5,0},.000004));
    CHECK(near(quadratic_result.value().tangent_at_distance(quadratic_result.value().length()/2).value(),{1,0,0},.000004));
    std::array<Vec3,7> multiple_spans{};
    for(unsigned i=0;i<multiple_spans.size();++i)multiple_spans[i]={static_cast<double>(i),0,0};
    const auto multi_result=compile_motion_path(multiple_spans,cancellation);CHECK(multi_result.has_value());CHECK(near(multi_result.value().length(),6));
    for(unsigned i=0;i<=120;++i){CHECK(near(multi_result.value().position_at_distance(i*.05).value(),{i*.05,0,0},.000006));CHECK(near(multi_result.value().tangent_at_distance(i*.05).value(),{1,0,0}));}
    std::array<Vec3,4> uneven{{{0,0,0},{0,0,0},{0,0,0},{100,0,0}}};
    auto uneven_result=compile_motion_path(uneven,cancellation);CHECK(uneven_result.has_value());CHECK(near(uneven_result.value().length(),100));
    CHECK(uneven_result.value().sample_count()>5);CHECK(near(uneven_result.value().tangent_at_distance(0).value(),{}));
    for(unsigned i=1;i<=100;++i)CHECK(near(uneven_result.value().position_at_distance(i).value(),{static_cast<double>(i),0,0},.00011));
    std::array<Vec3,4> bezier{{{0,0,0},{0,1,0},{1,1,0},{1,0,0}}};
    auto path_result=compile_motion_path(bezier,cancellation);CHECK(path_result.has_value());auto path=path_result.take_value();
    CHECK(near(path.length(),2.,.000003));CHECK(near(path.position_at_distance(path.length()/2).value(),{.5,.75,0},.000003));
    CHECK(near(path.tangent_at_distance(0).value(),{0,1,0}));CHECK(near(path.tangent_at_distance(path.length()).value(),{0,-1,0}));
    CHECK(path.sample_count()<=kMaxMotionPathSamples);
    std::array<Vec3,4> inflection{{{0,0,0},{0,1,0},{1,-1,0},{1,0,0}}};
    const auto inflected_result=compile_motion_path(inflection,cancellation);CHECK(inflected_result.has_value());
    // Independent composite Simpson integration of the analytical derivative
    // (no de Boor or control-polygon approximation in the oracle).
    double analytic_length=0;constexpr unsigned oracle_intervals=20000;
    for(unsigned i=0;i<=oracle_intervals;++i){const double t=static_cast<double>(i)/oracle_intervals;
        const double speed=std::hypot(6*t*(1-t),3*(1-6*t+6*t*t));
        analytic_length+=speed*(i==0 || i==oracle_intervals?1:i%2?4:2);}
    analytic_length/=3*oracle_intervals;
    CHECK(near(inflected_result.value().length(),analytic_length,.000008));
    CHECK(near(inflected_result.value().position_at_distance(inflected_result.value().length()/2).value(),{.5,0,0},.000008));
    std::array<Vec3,4> stationary{{{0,0,0},{1,0,0},{1,0,0},{0,0,0}}};
    const auto stationary_result=compile_motion_path(stationary,cancellation);CHECK(stationary_result.has_value());CHECK(near(stationary_result.value().length(),1.5,.000003));
    CHECK(near(stationary_result.value().position_at_distance(stationary_result.value().length()/2).value(),{.75,0,0},.000003));
    CHECK(near(stationary_result.value().tangent_at_distance(stationary_result.value().length()/2).value(),{}));
    CHECK(near(stationary_result.value().position_at_distance(1.125).value(),{.375,0,0},.000003));
    CHECK(near(stationary_result.value().tangent_at_distance(1.125).value(),{-1,0,0}));
    std::array<Vec3,4> translated{{{999999999,999999999,0},{999999999,1000000000,0},{1000000000,1000000000,0},{1000000000,999999999,0}}};
    const auto translated_result=compile_motion_path(translated,cancellation);CHECK(translated_result.has_value());
    CHECK(near(translated_result.value().length(),path.length(),1e-12));CHECK(translated_result.value().sample_count()==path.sample_count());
    CHECK(near(translated_result.value().position_at_distance(path.length()/2).value(),{999999999.5,999999999.75,0}));
    CHECK(near(translated_result.value().tangent_at_distance(path.length()/2).value(),path.tangent_at_distance(path.length()/2).value()));
    CHECK(path_result.value().length()==0);error(path_result.value().position_at_distance(0),ErrorCode::invalid_request);error(path_result.value().tangent_at_distance(0),ErrorCode::invalid_request);
    auto owned=path;bezier[0]={200,200,200};CHECK(near(owned.position_at_distance(0).value(),{}));
    std::array<Vec3,4> degenerate{{fixed[0],fixed[0],fixed[0],fixed[0]}};
    auto flat=compile_motion_path(degenerate,cancellation);CHECK(flat.has_value());CHECK(flat.value().length()==0);CHECK(near(flat.value().tangent_at_distance(0).value(),{}));
    Interrupt immediate(0),later(7);error(compile_motion_path(line,immediate),ErrorCode::cancelled);error(compile_motion_path(uneven,later),ErrorCode::cancelled);
    CHECK(uneven[3].x==100);
    const auto before_compile=allocations.load();const auto allocation_probe=compile_motion_path(uneven,cancellation);
    const auto compilation_allocations=allocations.load()-before_compile;CHECK(allocation_probe.has_value());CHECK(compilation_allocations>=7);
    for(std::size_t fail=1;fail<=compilation_allocations;++fail){fail_allocation=allocations.load()+fail;auto result=compile_motion_path(uneven,cancellation);fail_allocation=0;error(result,ErrorCode::allocation_failed);}
    const auto before_copy=allocations.load();auto copy_probe=path;const auto copy_allocations=allocations.load()-before_copy;CHECK(copy_allocations>0);
    copy_probe=line_result.value();
    for(std::size_t fail=1;fail<=copy_allocations;++fail){bool threw=false;fail_allocation=allocations.load()+fail;
        try{copy_probe=path;}catch(const std::bad_alloc&){threw=true;}fail_allocation=0;
        CHECK(threw);CHECK(copy_probe.length()==10);CHECK(near(copy_probe.position_at_distance(5).value(),{7,3,4}));}
    const auto before_self_copy=allocations.load();copy_probe=copy_probe;CHECK(allocations.load()==before_self_copy);
    copy_probe=path;CHECK(copy_probe.length()==path.length());CHECK(near(copy_probe.position_at_distance(path.length()/2).value(),{.5,.75,0},.000003));
    std::array<Vec3,kMaxMotionPathControlPoints> detailed{};
    for(std::size_t i=0;i<detailed.size();++i)detailed[i]={static_cast<double>(i),i%2?1.:-1.,std::sin(static_cast<double>(i))};
    const auto bounded=compile_motion_path(detailed,cancellation);
    if(bounded.has_value())CHECK(bounded.value().sample_count()<=kMaxMotionPathSamples);else error(bounded,ErrorCode::work_limit_exceeded);
    const auto before_queries=allocations.load();
    for(unsigned i=0;i<1000;++i){const auto d=(i*73%1000)*path.length()/1000;
        const auto p=path.position_at_distance(d);CHECK(p.has_value());CHECK(near(p.value(),owned.position_at_distance(d).value(),0));CHECK(near(norm(path.tangent_at_distance(d).value()),1));}
    CHECK(allocations.load()==before_queries);
    std::atomic<bool> concurrent_ok{true};std::array<std::thread,4> workers;
    for(auto& worker:workers)worker=std::thread([&]{for(unsigned i=0;i<1000;++i){const auto d=i*path.length()/1000;
        if(!near(path.position_at_distance(d).value(),owned.position_at_distance(d).value(),0))concurrent_ok=false;}});
    for(auto& worker:workers)worker.join();CHECK(concurrent_ok.load());
    std::cout<<"motion_geometry_tests: "<<checks<<" checks passed; host-independent geometry only, no Motion author/AE qualification\n";
}
