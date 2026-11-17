#include "starfield/core/AgeCurve.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    using namespace starfield::core;
    unsigned checks{};
    for(unsigned mode=0;mode<4;++mode)for(unsigned count=2;count<=kMaxAgeCurvePoints;++count) {
        AgeCurve curve{};curve.count=static_cast<decltype(curve.count)>(count);
        curve.interpolation=static_cast<CurveInterpolation>(mode);
        for(unsigned i=0;i<count;++i)curve.points[i]={double(i)/(count-1),100.*i/(count-1)};
        if(!valid_age_curve(curve,0,100))return 1;
        for(unsigned i=0;i<=1024;++i) {
            const double age=double(i)/1024,value=evaluate_age_curve(curve,age,0,100);
            if(!std::isfinite(value) || value<0 || value>100)std::abort();
            if(curve.interpolation!=CurveInterpolation::hold && std::abs(value-age*100)>1e-10)std::abort();
            ++checks;
        }
        if(evaluate_age_curve(curve,-1,0,100)!=0 || evaluate_age_curve(curve,2,0,100)!=100)std::abort();
        checks+=2;
    }
    for(const unsigned count:{0u,1u,unsigned(kMaxAgeCurvePoints)+1}) {
        AgeCurve invalid{};invalid.count=static_cast<decltype(invalid.count)>(count);
        if(evaluate_age_curve(invalid,.5,10,20)!=15)std::abort();++checks;
    }
    std::printf("Age curve bounds: %u checks, 0 failures\n",checks);
}
