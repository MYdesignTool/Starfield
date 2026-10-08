#pragma once
#include "starfield/core/Geometry.hpp"
#include "starfield/core/Error.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <string>

namespace starfield::adapter::transform_binding {
using Matrix=std::array<double,16>;
using PixelAffine=std::array<double,12>;

inline starfield::core::Result<PixelAffine> relative_anchor_affine(
    const Matrix& source,const Matrix& owner,starfield::core::Vec3 anchor) {
    using R=starfield::core::Result<PixelAffine>;
    for(const auto* m:{&source,&owner}) {
        for(double v:*m)if(!std::isfinite(v))return R::failure(starfield::core::ErrorCode::invalid_request,"nonfinite layer transform");
        if((*m)[12]!=0 || (*m)[13]!=0 || (*m)[14]!=0 || (*m)[15]!=1)
            return R::failure(starfield::core::ErrorCode::invalid_request,"nonaffine layer transform");
    }
    for(double v:{anchor.x,anchor.y,anchor.z})if(!std::isfinite(v))
        return R::failure(starfield::core::ErrorCode::invalid_request,"nonfinite layer anchor");
    double rows[3][6]{},maximum=0;
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c) {
        rows[r][c]=owner[r*4+c];maximum=std::max(maximum,std::abs(rows[r][c]));rows[r][c+3]=r==c?1.:0.;
    }
    for(unsigned c=0;c<3;++c) {
        unsigned pivot=c;for(unsigned r=c+1;r<3;++r)if(std::abs(rows[r][c])>std::abs(rows[pivot][c]))pivot=r;
        if(std::abs(rows[pivot][c])<=maximum*1e-14)
            return R::failure(starfield::core::ErrorCode::invalid_request,"singular effect layer frame");
        for(unsigned j=0;j<6;++j)std::swap(rows[c][j],rows[pivot][j]);
        const auto divisor=rows[c][c];for(double& v:rows[c])v/=divisor;
        for(unsigned r=0;r<3;++r)if(r!=c) {
            const auto factor=rows[r][c];for(unsigned j=0;j<6;++j)rows[r][j]-=factor*rows[c][j];
        }
    }
    const double a[]{anchor.x,anchor.y,anchor.z};double displacement[3]{};
    for(unsigned r=0;r<3;++r) {
        displacement[r]=source[r*4+3]-owner[r*4+3];
        for(unsigned c=0;c<3;++c)displacement[r]+=source[r*4+c]*a[c];
    }
    PixelAffine result{};
    for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k) {
        for(unsigned c=0;c<3;++c)result[r*4+c]+=rows[r][k+3]*source[k*4+c];
        result[r*4+3]+=rows[r][k+3]*displacement[k];
    }
    for(double v:result)if(!std::isfinite(v))return R::failure(starfield::core::ErrorCode::invalid_request,"invalid relative layer transform");
    return R::success(std::move(result));
}

inline starfield::core::Result<Matrix> canonical_matrix(const PixelAffine& pixel,
                                                       starfield::core::LayerUnits units) {
    using R=starfield::core::Result<Matrix>;
    if(!std::isfinite(units.layer_width) || !std::isfinite(units.layer_height) ||
       !std::isfinite(units.pixel_aspect_ratio) || units.layer_width<=0 || units.layer_height<=0 || units.pixel_aspect_ratio<=0)
        return R::failure(starfield::core::ErrorCode::invalid_request,"invalid Transform layer geometry");
    const double scale[]{units.layer_height/units.pixel_aspect_ratio,-units.layer_height,units.layer_height};
    const double centre[]{units.layer_width*.5,units.layer_height*.5,0};Matrix result{};result[15]=1;
    for(unsigned r=0;r<3;++r) {
        for(unsigned c=0;c<3;++c)result[r*4+c]=pixel[r*4+c]*scale[c]/scale[r];
        result[r*4+3]=(pixel[r*4+3]-centre[r])/scale[r];
    }
    for(double v:result)if(!std::isfinite(v) || std::abs(v)>1e6)
        return R::failure(starfield::core::ErrorCode::invalid_request,"inherited Transform exceeds core bounds");
    return R::success(std::move(result));
}

// Insert inside the UUID-selected branch of a main-effect numeric expression.
// The same bounded inverse as the UI capture is evaluated at expression time.
// Only documented toWorld/toWorldVec methods are used; no script-side host writes.
inline std::string matrix_expression(unsigned entry, bool inherited = true) {
    const unsigned row=entry/4,column=entry%4;
    std::string text="// Starfield Transform affine entry "+std::to_string(entry)+"\n";
    // PF_LAYER is constant structure. None is already validated during UI
    // compilation; do not evaluate an empty AE layer property in an expression.
    // Changing the source recompiles these aliases through the supervised edit.
    if (!inherited) {
        text += "var identity = [1,0,0,thisLayer.width*0.5,0,1,0,thisLayer.height*0.5,0,0,1,0];\n";
        return text+"result = identity["+std::to_string(entry)+"];\n";
    }
    // AE expression layer controls resolve to a Layer object. Their scripting
    // DOM numeric .value/index representation is not the expression contract.
    text+=R"(var src = fx.param(1);
if (src == null || typeof src === "number") throw new Error("Starfield: inherited layer unavailable");
function point(v) { return [v[0],v[1],v.length > 2 ? v[2] : 0]; }
function axis(layer,n) {
    var v = [0,0,0]; v[n] = 1; var w = layer.toWorldVec(v,time);
    return [w[0],w[1],w.length > 2 ? w[2] : (n === 2 ? 1 : 0)];
}
var x = axis(thisLayer,0), y = axis(thisLayer,1), z = axis(thisLayer,2);
var rows = [[x[0],y[0],z[0],1,0,0],[x[1],y[1],z[1],0,1,0],[x[2],y[2],z[2],0,0,1]];
var maximum = 0;
for (var r=0;r<3;r++) for (var c=0;c<3;c++) maximum = Math.max(maximum,Math.abs(rows[r][c]));
for (var c=0;c<3;c++) {
    var pivot=c;
    for (var r=c+1;r<3;r++) if (Math.abs(rows[r][c]) > Math.abs(rows[pivot][c])) pivot=r;
    if (!(Math.abs(rows[pivot][c]) > maximum*1e-14)) throw new Error("Starfield: singular effect layer frame");
    var swap=rows[c]; rows[c]=rows[pivot]; rows[pivot]=swap;
    var divisor=rows[c][c]; for (var j=0;j<6;j++) rows[c][j]/=divisor;
    for (var r=0;r<3;r++) if (r!==c) {
        var factor=rows[r][c]; for (var j=0;j<6;j++) rows[r][j]-=factor*rows[c][j];
    }
}
)";
    if(column==3)text+=R"(var target=point(src.toWorld(src.anchorPoint.value,time)), origin=point(thisLayer.toWorld([0,0,0],time));
var v=[target[0]-origin[0],target[1]-origin[1],target[2]-origin[2]];
)";
    else text+="var v=axis(src,"+std::to_string(column)+");\n";
    text+="result = rows["+std::to_string(row)+"][3]*v[0]+rows["+std::to_string(row)+"][4]*v[1]+rows["+std::to_string(row)+"][5]*v[2];\n";
    text+="if (!isFinite(result)) throw new Error(\"Starfield: invalid Null layer transform\");\n";
    return text;
}
}
