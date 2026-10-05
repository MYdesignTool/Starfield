#pragma once
#include "starfield/core/Settings.hpp"
#include <algorithm>
#include <cmath>

namespace starfield::adapter::curve_editor {
// Editing policy, not a persistence limit. Inputs are validated UI curves.
inline constexpr unsigned kMaxLinearHandles = 12;
inline constexpr double kRelativeLinearError = 0.04;

[[nodiscard]] inline core::AgeCurve simplify_draw(const core::AgeCurve& source) noexcept {
    auto result=source;result.interpolation=core::CurveInterpolation::linear;
    if(source.count<2 || source.count>core::kMaxAgeCurvePoints)return result;
    double minimum=source.points[0].value,maximum=minimum;
    for(unsigned i=1;i<source.count;++i) {
        minimum=std::min(minimum,source.points[i].value);
        maximum=std::max(maximum,source.points[i].value);
    }
    const auto tolerance=std::max(1e-6,(maximum-minimum)*kRelativeLinearError);
    std::array<bool,core::kMaxAgeCurvePoints> retained{};
    retained[0]=retained[source.count-1]=true;
    unsigned count=2;
    // Split the segment with the largest vertical deviation first. This keeps
    // significant peaks/valleys while avoiding one handle per Draw sample.
    while(count<kMaxLinearHandles) {
        unsigned candidate=source.count,left=0;
        double error=tolerance;
        for(unsigned right=1;right<source.count;++right) {
            if(!retained[right])continue;
            const auto& a=source.points[left];const auto& b=source.points[right];
            for(unsigned i=left+1;i<right;++i) {
                const auto& point=source.points[i];
                const auto amount=(point.age-a.age)/(b.age-a.age);
                const auto deviation=std::abs(point.value-(a.value+(b.value-a.value)*amount));
                if(deviation>error){candidate=i;error=deviation;}
            }
            left=right;
        }
        if(candidate==source.count)break;
        retained[candidate]=true;++count;
    }
    result={};result.interpolation=core::CurveInterpolation::linear;
    for(unsigned i=0;i<source.count;++i)
        if(retained[i])result.points[result.count++]=source.points[i];
    return result;
}
}
