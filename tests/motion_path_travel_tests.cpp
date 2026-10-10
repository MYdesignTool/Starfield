#include "starfield/core/MotionPathTravel.hpp"
#include "starfield/core/ParticleMotionPose.hpp"
#include "starfield/core/MotionCircle.hpp"
#include "starfield/core/Render.hpp"
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <stdexcept>
#include <thread>
namespace {std::atomic<std::size_t> allocations{},fail_allocation{};}
void* operator new(std::size_t size){const auto n=++allocations;if(n==fail_allocation)throw std::bad_alloc{};
    if(auto* p=std::malloc(std::max<std::size_t>(1,size)))return p;throw std::bad_alloc{};}
void operator delete(void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}
using namespace starfield::core;
namespace {
unsigned checks{};NeverCancelled never;
void check(bool v,const char* text){++checks;if(!v)throw std::runtime_error(text);}
void near(double a,double b,const char* text,double tolerance=1e-8){check(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<tolerance,text);}
void near(Vec3 a,Vec3 b,const char* text,double tolerance=1e-8){near(a.x,b.x,text,tolerance);near(a.y,b.y,text,tolerance);near(a.z,b.z,text,tolerance);}
template<class T>T take(Result<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
const std::array<Vec3,2> line{{{10,20,30},{10,22,30}}};
ParticleInstance particle(){ParticleInstance p;p.position={.3,-.2,.1};p.velocity={.02,.03,.04};p.age_seconds=.5;p.lifetime_seconds=3;p.shape=1;
    p.size_pixels=20;p.size_y_pixels=4;p.opacity=.8;p.rotation_degrees={12,34,56};p.id=17;return p;}
void retained(const ParticleInstance& p,const ParticleInstance& old){check(p.id==old.id&&p.emitter_id==old.emitter_id&&p.shape==old.shape&&p.size_pixels==old.size_pixels&&p.opacity==old.opacity,"identity/style preserved");
    near(p.rotation_degrees,old.rotation_degrees,"Euler unchanged");check(p.sprite_basis_index==old.sprite_basis_index,"shared basis unchanged");}
double quadrature(const AgeCurve& curve,double start,double end,double lifetime){double total=0;
    for(unsigned segment=0;segment+1<curve.count;++segment){const auto a=std::max(start,curve.points[segment].age*lifetime),b=std::min(end,curve.points[segment+1].age*lifetime);if(b<=a)continue;
        constexpr unsigned count=2000;const auto step=(b-a)/count;for(unsigned i=0;i<count;++i)total+=evaluate_age_curve(curve,(a+(i+.5)*step)/lifetime,100,100)/100*step;}
    return total;
}
void clocks(){AgeCurve curve;curve.count=4;curve.points[0]={0,100};curve.points[1]={.25,300};curve.points[2]={.75,50};curve.points[3]={1,0};
    for(unsigned mode=0;mode<4;++mode){curve.interpolation=static_cast<CurveInterpolation>(mode);const auto clock=take(compile_motion_curve_clock(curve));
        near(take(clock.integral_between(.37,1.8,2)),quadrature(curve,.37,1.8,2),"delayed curve integral matches independent quadrature",2e-6);
        near(take(clock.integral_between(0,1.8,2)),take(clock.clock(1.8,2)).integral_seconds,"prefix convention retained");
        for(unsigned i=0;i<=20;++i){const auto age=i*.1;near(take(clock.clock(age,2)).weight,evaluate_age_curve(curve,age/2,100,100)/100,"shared curve value convention");}
        MotionCircleSettings circle;circle.over_life=curve;const auto old=take(compile_motion_circle(circle));
        near(take(old.clock(1.8,2)).integral_seconds,take(clock.clock(1.8,2)).integral_seconds,"Circle uses same clock");
    }
    const auto clock=take(compile_motion_curve_clock({}));const double end=1e6,start=end-1e-4;
    check(take(clock.integral_between(start,end,end))==end-start,"short late interval retains precision");
    check(!clock.integral_between(1,.5,2).has_value(),"reversed interval rejects");check(!clock.clock(-1,2).has_value(),"negative age rejects");
    curve.count=1;check(!compile_motion_curve_clock(curve).has_value(),"invalid curve rejects");
}
void travel(){MotionPathTravelSettings settings;settings.units_per_second=2;settings.delay_seconds=.25;
    const auto path=take(compile_motion_path_travel(line,settings,never));check(path.storage_bytes()>sizeof(path),"owned capacity is reported");
    auto a=take(path.travel(.1,3));near(a.displacement,Vec3{},"waiting keeps distribution");near(a.velocity,Vec3{},"waiting contributes no velocity");
    a=take(path.travel(.5,3));near(a.displacement,Vec3{0,.5,0},"distance after delay");near(a.velocity,Vec3{0,2,0},"instant travel velocity");
    near(take(path.travel(.25,3)).velocity,Vec3{0,2,0},"right derivative at delay boundary");
    a=take(path.travel(2,3));near(a.displacement,Vec3{0,2,0},"current scalar endpoint clamp");near(a.velocity,Vec3{},"clamped endpoint derivative");
    auto p=particle(),old=p;take(path.apply(p,0));near(p.position,Vec3{.3,.3,.1},"emitted offset retained");near(p.velocity,Vec3{.02,2.03,.04},"travel velocity additive");retained(p,old);
    settings.speed_random_percent=100;const auto random=take(compile_motion_path_travel(line,settings,never));near(take(random.travel(.5,3,1)).displacement,Vec3{},"full speed attenuation");
    near(take(random.travel(.5,3,.5)).displacement,Vec3{0,.25,0},"explicit stable sample");
    settings.delay_seconds=4;const auto delayed=take(compile_motion_path_travel(line,settings,never));near(take(delayed.travel(3,3)).displacement,Vec3{},"delay exceeds lifetime");
    check(!path.travel(.5,3,-.1).has_value(),"bad random sample rejects");check(!path.travel_at_distance(1e13,1).has_value(),"distance bound rejects");
    settings={};settings.over_life.count=2;settings.over_life.points[0]={0,0};settings.over_life.points[1]={1,100};settings.delay_seconds=.5;
    const auto curve=take(compile_motion_path_travel(line,settings,never));near(take(curve.travel(1,2)).distance,.1875,"Over Life keeps actual age clock after delay");
    near(take(curve.travel(1,2)).units_per_second,.5,"age weight at current time");
}
void orientation(){MotionPathTravelSettings settings;settings.orient_to_path=true;const auto path=take(compile_motion_path_travel(line,settings,never));
    auto p=particle(),old=p;p.sprite_basis_index=1;old=p;const ParticleSpriteBasis basis{-2,.3,.1,0,1,.2,0,0,1};
    take(path.apply(p,0,{&basis,1}));const auto forward=take(particle_motion_forward(p,{1,0,0},{&basis,1}));near(Vec3{forward.x/std::hypot(forward.x,forward.y,forward.z),forward.y/std::hypot(forward.x,forward.y,forward.z),forward.z/std::hypot(forward.x,forward.y,forward.z)},Vec3{0,1,0},"path alignment keeps authored affine");retained(p,old);
    take(path.apply_at_distance(p,1,-1,{&basis,1}));const auto reverse=take(particle_motion_forward(p,{1,0,0},{&basis,1}));check(reverse.y<0&&std::abs(reverse.x)<1e-8&&std::abs(reverse.z)<1e-8,"signed tangent follows reverse rate");
    const std::array<Vec3,2> tilted{{{0,0,0},{0,1,1}}};const auto flat=take(compile_motion_path_travel(tilted,settings,never));p=particle();p.limit_to_2d=true;
    take(flat.apply(p,0));const auto f=take(particle_motion_forward(p,{1,0,0},{}));near(f,Vec3{0,1,0},"2D tangent projection");
    settings.units_per_second=0;const auto stopped=take(compile_motion_path_travel(line,settings,never));p=particle();p.position.x=-0.;old=p;take(stopped.apply(p,0));
    check(std::bit_cast<std::uint64_t>(p.position.x)==std::bit_cast<std::uint64_t>(old.position.x)&&p.motion_pose==old.motion_pose,"zero motion exact position/pose");
    const std::array<Vec3,1> one{{{100,200,300}}};settings.units_per_second=1;const auto stationary=take(compile_motion_path_travel(one,settings,never));take(stationary.apply(p,0));
    check(std::signbit(p.position.x)&&p.motion_pose==old.motion_pose,"zero-length path preserves signed zero and pose");
}
void failures(){MotionPathTravelSettings settings;const auto path=take(compile_motion_path_travel(line,settings,never));auto p=particle(),old=p;
    p.position.y=1e9;old=p;auto moved=path.apply_at_distance(p,1,1);check(!moved.has_value(),"output bound rejects");near(p.position,old.position,"failed apply leaves center intact");near(p.velocity,old.velocity,"failed apply leaves velocity intact");check(p.motion_pose==old.motion_pose,"failed apply leaves pose intact");
    settings.orient_to_path=true;const auto turn=take(compile_motion_path_travel(line,settings,never));p=particle();p.sprite_basis_index=1;old=p;check(!turn.apply(p,0).has_value(),"missing basis rejects atomically");near(p.position,old.position,"orientation failure leaves center");
    settings.delay_seconds=-1;check(!compile_motion_path_travel(line,settings,never).has_value(),"bad delay rejects");settings.delay_seconds=0;settings.forward={};check(!compile_motion_path_travel(line,settings,never).has_value(),"zero alignment forward rejects");
    settings={};bool complete=false;
    for(unsigned n=1;n<100;++n){fail_allocation=allocations.load()+n;auto compiled=compile_motion_path_travel(line,settings,never);fail_allocation=0;
        if(compiled.has_value()){complete=true;break;}check(compiled.error().code==ErrorCode::allocation_failed,"all compilation allocations typed");}check(complete,"allocation scan reaches success");
    const std::array<Vec3,4> curved{{{0,0,0},{1,2,0},{2,-1,0},{3,0,0}}};const auto curved_path=take(compile_motion_path_travel(curved,settings,never));complete=false;
    for(unsigned n=1;n<100;++n){auto target=path;fail_allocation=allocations.load()+n;try{target=curved_path;fail_allocation=0;complete=true;break;}catch(const std::bad_alloc&){fail_allocation=0;near(take(target.travel(.5,3)).displacement,Vec3{0,.5,0},"copy assignment failure retains path/clock/settings");}}
    check(complete,"copy fault scan reaches success");
    struct Stop:Cancellation{mutable unsigned calls{};unsigned bound;explicit Stop(unsigned n):bound(n){}bool is_cancelled()const noexcept override{return calls++>=bound;}};
    complete=false;for(unsigned n=0;n<100;++n){Stop stop(n);auto compiled=compile_motion_path_travel(line,settings,stop);if(compiled.has_value()){complete=true;break;}check(compiled.error().code==ErrorCode::cancelled,"compilation cancellation typed");}check(complete,"cancellation reaches success");
    const auto before=allocations.load();for(unsigned i=0;i<1000;++i){auto sample=path.travel((i*17%1000)/1000.,3);check(sample.has_value(),"unordered query succeeds");}
    check(before==allocations.load(),"queries allocate no storage");
    std::atomic<bool> good{true};std::array<std::thread,4> workers;for(auto& worker:workers)worker=std::thread([&]{for(unsigned i=0;i<1000;++i){auto sample=path.travel((i*31%1000)/1000.,3);if(!sample.has_value()||sample.value().velocity.y!=1)good=false;}});
    for(auto& worker:workers)worker.join();check(good,"immutable concurrent path queries");
}
}
int main()try{clocks();travel();orientation();failures();std::printf("motion_path_travel_tests: %u checks passed; explicit numeric travel only, reference/graph/AE gates pending\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"Motion path travel fixture after %u checks: %s\n",checks,e.what());return 1;}
