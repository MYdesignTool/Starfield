#include "starfield/core/MotionGeometry.hpp"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <cmath>
#include <new>
#include <numbers>
#include <optional>

namespace starfield::core {
namespace {
constexpr std::array<double,9> identity{1,0,0,0,1,0,0,0,1};
Vec3 add(Vec3 a,Vec3 b) noexcept { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
Vec3 subtract(Vec3 a,Vec3 b) noexcept { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
Vec3 scale(Vec3 a,double factor) noexcept { return {a.x*factor,a.y*factor,a.z*factor}; }
double norm(Vec3 a) noexcept { return std::hypot(a.x,a.y,a.z); }
Vec3 unit(Vec3 a) noexcept {
    // Scale first: hypot of several subnormal components can round down enough
    // that dividing by it would produce a direction longer than one.
    const auto largest=std::max({std::abs(a.x),std::abs(a.y),std::abs(a.z)});
    a={a.x/largest,a.y/largest,a.z/largest};
    const auto magnitude=norm(a);
    return {a.x/magnitude,a.y/magnitude,a.z/magnitude};
}
double distance(Vec3 a,Vec3 b) noexcept { return norm(subtract(a,b)); }
bool valid(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
        std::abs(value.x)<=kMaxMotionCoordinate && std::abs(value.y)<=kMaxMotionCoordinate && std::abs(value.z)<=kMaxMotionCoordinate;
}
Vec3 cross(Vec3 a,Vec3 b) noexcept { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
double dot(Vec3 a,Vec3 b) noexcept { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec3 interpolate(Vec3 a,Vec3 b,double t) noexcept { return add(scale(a,1-t),scale(b,t)); }
std::array<double,9> rotation(Vec3 a,double radians) noexcept {
    const auto c=std::cos(radians),s=std::sin(radians),v=1-c;
    return {c+a.x*a.x*v,a.x*a.y*v-a.z*s,a.x*a.z*v+a.y*s,
        a.y*a.x*v+a.z*s,c+a.y*a.y*v,a.y*a.z*v-a.x*s,
        a.z*a.x*v-a.y*s,a.z*a.y*v+a.x*s,c+a.z*a.z*v};
}
Vec3 transform(const std::array<double,9>& m,Vec3 value) noexcept {
    return {m[0]*value.x+m[1]*value.y+m[2]*value.z,m[3]*value.x+m[4]*value.y+m[5]*value.z,m[6]*value.x+m[7]*value.y+m[8]*value.z};
}
Vec3 spline(std::span<const Vec3> controls,std::span<const double> knots,unsigned degree,double t) noexcept {
    if(controls.size()==1)return controls.front();
    const auto last=controls.size()-1;
    const auto span=t<=0?static_cast<std::size_t>(degree):t>=1?last:
        std::clamp(static_cast<std::size_t>(std::upper_bound(knots.begin(),knots.end(),t)-knots.begin()-1),static_cast<std::size_t>(degree),last);
    std::array<Vec3,4> work{};
    for(unsigned j=0;j<=degree;++j)work[j]=controls[span-degree+j];
    for(unsigned r=1;r<=degree;++r)for(unsigned j=degree;j>=r;--j){
        const auto index=span-degree+j;
        const auto denominator=knots[index+degree-r+1]-knots[index];
        const auto weight=denominator>0?(t-knots[index])/denominator:0;
        work[j]=interpolate(work[j-1],work[j],weight);
    }
    return work[degree];
}
} // namespace

Result<Vec3> rotate_motion_circle(Vec3 position,Vec3 origin,Vec3 axis,double radians) noexcept {
    using R=Result<Vec3>;
    if(!valid(position) || !valid(origin) || !valid(axis) || !std::isfinite(radians) || std::abs(radians)>1e12 || norm(axis)==0)
        return R::failure(ErrorCode::invalid_request,"invalid Motion circle geometry");
    radians=std::remainder(radians,2*std::numbers::pi);
    if(radians==0)return R::success(position);
    const auto value=add(origin,transform(rotation(unit(axis),radians),subtract(position,origin)));
    if(!valid(value))return R::failure(ErrorCode::invalid_request,"Motion circle result exceeds coordinate bound");
    return R::success(value);
}
Result<std::array<double,9>> motion_look_at_rotation(Vec3 forward,Vec3 origin,Vec3 goal,double weight) noexcept {
    using R=Result<std::array<double,9>>;
    if(!valid(forward) || !valid(origin) || !valid(goal) || norm(forward)==0 || !std::isfinite(weight) || weight<0 || weight>1)
        return R::failure(ErrorCode::invalid_request,"invalid Motion look-at geometry");
    const auto delta=subtract(goal,origin);
    if(weight==0 || norm(delta)==0)return R::success(identity);
    forward=unit(forward);const auto desired=unit(delta);
    auto axis=cross(forward,desired);const auto axis_length=norm(axis),cosine=std::clamp(dot(forward,desired),-1.,1.);
    if(axis_length<=1e-12){
        if(cosine>=0)return R::success(identity);
        Vec3 perpendicular=std::abs(forward.x)<=std::abs(forward.y) && std::abs(forward.x)<=std::abs(forward.z)?Vec3{1,0,0}:
            std::abs(forward.y)<=std::abs(forward.z)?Vec3{0,1,0}:Vec3{0,0,1};
        axis=unit(cross(forward,perpendicular));
        return R::success(rotation(axis,std::numbers::pi*weight));
    }
    return R::success(rotation(unit(axis),std::atan2(axis_length,cosine)*weight));
}

CompiledMotionPath& CompiledMotionPath::operator=(const CompiledMotionPath& other) {
    if(this!=&other){
        // Commit all owned tables together: a failed allocation must not leave
        // controls, knots and lookup data from different paths in one object.
        CompiledMotionPath copy(other);
        *this=std::move(copy);
    }
    return *this;
}
double CompiledMotionPath::parameter_at_distance(double value) const noexcept {
    if(value<=0 || distances_.back()==0)return 0;
    if(value>=distances_.back())return 1;
    const auto upper=static_cast<std::size_t>(std::upper_bound(distances_.begin(),distances_.end(),value)-distances_.begin());
    const auto lower=upper-1;
    return parameters_[lower]+(parameters_[upper]-parameters_[lower])*(value-distances_[lower])/(distances_[upper]-distances_[lower]);
}
Result<Vec3> CompiledMotionPath::position_at_distance(double value) const noexcept {
    using R=Result<Vec3>;
    if(controls_.empty() || !std::isfinite(value))return R::failure(ErrorCode::invalid_request,"invalid Motion path or distance");
    const auto parameter=parameter_at_distance(value);
    if(parameter==0)return R::success(origin_);
    if(parameter==1)return R::success(end_);
    return R::success(add(origin_,spline(controls_,knots_,degree_,parameter)));
}
Result<Vec3> CompiledMotionPath::tangent_at_distance(double value) const noexcept {
    using R=Result<Vec3>;
    if(controls_.empty() || !std::isfinite(value))return R::failure(ErrorCode::invalid_request,"invalid Motion path or distance");
    if(derivative_controls_.empty())return R::success({});
    const auto derivative=spline(derivative_controls_,derivative_knots_,degree_-1,parameter_at_distance(value));
    const auto magnitude=norm(derivative);
    return R::success(magnitude>0?unit(derivative):Vec3{});
}
Result<CompiledMotionPath> compile_motion_path(std::span<const Vec3> points,const Cancellation& cancellation) noexcept {
    using R=Result<CompiledMotionPath>;
    if(points.empty())return R::failure(ErrorCode::invalid_request,"Motion path needs a point");
    if(points.size()>kMaxMotionPathControlPoints)return R::failure(ErrorCode::work_limit_exceeded,"Motion path point cap exceeded");
    if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion path compilation cancelled");
    double polygon_length=0;
    for(std::size_t i=0;i<points.size();++i){
        if(!valid(points[i]))return R::failure(ErrorCode::invalid_request,"invalid Motion path point");
        if(i)polygon_length+=distance(points[i-1],points[i]);
    }
    try{
        CompiledMotionPath path;path.origin_=points.front();path.end_=points.back();
        path.controls_.reserve(points.size());
        // Keep interpolation local to the first point. Small paths far from
        // zero must not spend the subdivision budget on world-coordinate ulps.
        for(const auto point:points)path.controls_.push_back(subtract(point,path.origin_));
        path.degree_=static_cast<unsigned>(std::min<std::size_t>(3,points.size()-1));
        path.knots_.resize(points.size()+path.degree_+1);
        for(std::size_t i=0;i<path.knots_.size();++i)path.knots_[i]=i<=path.degree_?0:i>=points.size()?1:
            static_cast<double>(i-path.degree_)/static_cast<double>(points.size()-path.degree_);
        for(std::size_t i=0;i+1<points.size();++i){
            const auto divisor=path.knots_[i+path.degree_+1]-path.knots_[i+1];
            path.derivative_controls_.push_back(scale(subtract(path.controls_[i+1],path.controls_[i]),path.degree_/divisor));
        }
        if(path.degree_)path.derivative_knots_.assign(path.knots_.begin()+1,path.knots_.end()-1);
        path.parameters_.push_back(0);path.distances_.push_back(0);
        if(points.size()==1)return R::success(std::move(path));
        const auto tolerance=std::max(1e-12,polygon_length*1e-6);
        std::size_t evaluations=0;std::optional<CoreError> error;Vec3 previous=path.controls_.front();
        const auto charge=[&](){
            if(cancellation.is_cancelled())error=make_error(ErrorCode::cancelled,"Motion path compilation cancelled");
            else if(++evaluations>kMaxMotionPathEvaluations)error=make_error(ErrorCode::work_limit_exceeded,"Motion path evaluation cap exceeded");
            return !error.has_value();
        };
        const auto append=[&](double t,Vec3 value){
            if(path.parameters_.size()>=kMaxMotionPathSamples){error=make_error(ErrorCode::work_limit_exceeded,"Motion path sample cap exceeded");return false;}
            path.parameters_.push_back(t);path.distances_.push_back(path.distances_.back()+distance(previous,value));previous=value;return true;
        };
        // Each knot span is a polynomial of degree <=3. Endpoints and endpoint
        // derivatives give its exact cubic Bernstein representation (including
        // degree elevation for linear/quadratic spans). Its control polygon is
        // an upper bound on arc length, unlike sampling a few interior points.
        const auto split=[](const std::array<Vec3,4>& curve){
            const auto a=interpolate(curve[0],curve[1],.5),b=interpolate(curve[1],curve[2],.5),c=interpolate(curve[2],curve[3],.5);
            const auto d=interpolate(a,b,.5),e=interpolate(b,c,.5),mid=interpolate(d,e,.5);
            return std::array<std::array<Vec3,4>,2>{{{curve[0],a,d,mid},{mid,e,c,curve[3]}}};
        };
        const auto subdivide=[&](auto&& self,double begin,double end,const std::array<Vec3,4>& curve,unsigned depth)->bool{
            if(!charge())return false;
            const auto width=end-begin;
            const auto polygon=distance(curve[0],curve[1])+distance(curve[1],curve[2])+distance(curve[2],curve[3]);
            const auto linear_error=std::max(distance(curve[1],interpolate(curve[0],curve[3],1./3)),
                distance(curve[2],interpolate(curve[0],curve[3],2./3)));
            const auto halves=split(curve);
            const auto midpoint=begin+width*.5;
            if(polygon-distance(curve[0],curve[3])<=tolerance*width && linear_error<=tolerance){
                const auto first=split(halves[0]),second=split(halves[1]);
                const std::array<double,4> times{begin+width*.25,midpoint,begin+width*.75,end};
                const std::array<Vec3,4> values{first[0][3],halves[0][3],second[0][3],curve[3]};
                for(unsigned i=0;i<4;++i)if(!append(times[i],values[i]))return false;
                return true;
            }
            if(depth==16){error=make_error(ErrorCode::work_limit_exceeded,"Motion path subdivision depth exceeded");return false;}
            return self(self,begin,midpoint,halves[0],depth+1) && self(self,midpoint,end,halves[1],depth+1);
        };
        for(std::size_t span=path.degree_;span<points.size();++span){
            const auto begin=path.knots_[span],end=path.knots_[span+1];
            if(!charge())return R::failure(*error);
            const auto a=spline(path.controls_,path.knots_,path.degree_,begin),b=spline(path.controls_,path.knots_,path.degree_,end);
            const auto start=spline(path.derivative_controls_,path.derivative_knots_,path.degree_-1,begin);
            const auto finish=spline(path.derivative_controls_,path.derivative_knots_,path.degree_-1,end);
            const std::array<Vec3,4> curve{a,add(a,scale(start,(end-begin)/3)),subtract(b,scale(finish,(end-begin)/3)),b};
            if(!subdivide(subdivide,begin,end,curve,0))return R::failure(*error);
        }
        return R::success(std::move(path));
    }catch(const std::bad_alloc&){return R::failure(ErrorCode::allocation_failed,"Motion path allocation failed");}
    catch(...){return R::failure(ErrorCode::internal_failure,"Motion path compilation failed");}
}
} // namespace starfield::core
