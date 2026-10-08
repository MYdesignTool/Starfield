#include "TransformBinding.hpp"
#include <cstdio>
#include <fstream>
#include <limits>

namespace {
using namespace starfield::adapter::transform_binding;
int checks{},failures{};
void check(bool ok,const char* why) { ++checks;if(!ok){++failures;std::printf("FAILED: %s\n",why);} }
void near(double a,double b) { if(std::abs(a-b)>=1e-10)std::printf("  actual %.17g expected %.17g\n",a,b);check(std::abs(a-b)<1e-10,"affine coefficient matches independent expected value"); }
Matrix identity() { return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}; }
}
int main() {
    const auto units=starfield::core::LayerUnits{1920,1080,1};
    auto source=identity(),owner=identity();source[3]=910;source[7]=490;
    auto pixel=relative_anchor_affine(source,owner,{50,50,0});check(pixel.has_value(),"centred Null captures successfully");
    auto canonical=canonical_matrix(pixel.value(),units);check(canonical.has_value(),"centred Null converts successfully");
    for(unsigned i=0;i<16;++i)near(canonical.value()[i],identity()[i]);
    source={-2,.3,0,100,0,3,.2,200,.4,0,1,300,0,0,0,1};
    owner={2,0,0,20,0,-4,0,40,0,0,.5,60,0,0,0,1};
    pixel=relative_anchor_affine(source,owner,{50,50,10});check(pixel.has_value(),"reflected and sheared parent motion is retained");
    const PixelAffine expected{-1,.15,0,-2.5,0,-.75,-.05,-78,.8,0,2,540};
    for(unsigned i=0;i<12;++i)near(pixel.value()[i],expected[i]);
    canonical=canonical_matrix(pixel.value(),{1920,1080,2});check(canonical.has_value(),"PAR conversion succeeds");
    near(canonical.value()[1],-.3);near(canonical.value()[4],0);near(canonical.value()[9],0);
    near(canonical.value()[3],(-2.5-960)/540);near(canonical.value()[7],(-78-540)/-1080.0);
    // Rank-deficient source motion is valid; only the owner's inverse is required.
    source=identity();source[0]=0;source[5]=0;source[10]=0;
    check(relative_anchor_affine(source,owner,{}).has_value(),"zero source scales do not require an inverse");
    owner[0]=0;check(!relative_anchor_affine(source,owner,{}).has_value(),"singular owner frame rejects");
    owner=identity();owner[0]=1e-16;check(!relative_anchor_affine(source,owner,{}).has_value(),"ill-conditioned owner frame rejects");
    owner=identity();owner[15]=0;check(!relative_anchor_affine(source,owner,{}).has_value(),"nonaffine frame rejects");
    owner=identity();source[7]=std::numeric_limits<double>::quiet_NaN();
    check(!relative_anchor_affine(source,owner,{}).has_value(),"nonfinite source frame rejects");
    source=identity();check(!relative_anchor_affine(source,owner,{0,0,std::numeric_limits<double>::infinity()}).has_value(),"nonfinite anchor rejects");
    for(const auto bad: {starfield::core::LayerUnits{0,1080,1},{1920,-1,1},{1920,1080,0}})
        check(!canonical_matrix(expected,bad).has_value(),"invalid geometry rejects");
    auto excessive=expected;excessive[0]=1e7;check(!canonical_matrix(excessive,units).has_value(),"matrix bound rejects");
    std::ofstream output("artifacts/transform-matrix-expressions.json");output<<"{";
    for(bool inherited:{false,true}) {
        if(inherited)output<<",";
        output<<"\""<<(inherited?"selected":"none")<<"\":[";
        for(unsigned i=0;i<12;++i) {
            if(i)output<<",";output<<"\"";
            const auto expression=matrix_expression(i,inherited);
            if(!inherited)check(expression.find("fx.param(1)")==std::string::npos,
                "None expression never dereferences the PF layer property");
            for(char c:expression) {
                if(c=='\n')output<<"\\n";else if(c=='\r')output<<"\\r";
                else {if(c=='\\' || c=='\"')output<<'\\';output<<c;}
            }
            output<<"\"";
        }
        output<<"]";
    }
    output<<"}\n";check(bool(output),"actual expression bodies emitted for JS execution checks");
    std::printf("Transform binding: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
