#include "NativeTemporalCache.hpp"
#include "SPBasic.h"
#include <array>
#include <cstring>
#include <cstdio>

using namespace starfield::adapter;
using namespace starfield::core;
namespace {
int checks{},failures{},references{},samples{};bool unavailable{},compare_error{};
std::array<unsigned,1024> epochs{};
PF_ParamUtilsSuite3 utils{};
void check(bool v,const char* m) {++checks;if(!v){++failures;std::printf("FAILED: %s\n",m);}}
SPErr acquire(const char* name,int32,const void** out) {
    if(unavailable || std::strcmp(name,kPFParamUtilsSuite))return 1;
    ++references;*out=&utils;return 0;
}
SPErr release(const char*,int32){--references;return 0;}
PF_Err state(PF_ProgPtr ref,PF_ParamIndex index,const A_Time* start,const A_Time* duration,PF_State* out) {
    check(!start && !duration,"state covers all time");
    *out={};unsigned identity=unsigned(reinterpret_cast<std::uintptr_t>(ref));
    std::memcpy(out,&identity,sizeof(identity));std::memcpy(reinterpret_cast<char*>(out)+4,&epochs[index],4);return 0;
}
PF_Err same(PF_ProgPtr,const PF_State* a,const PF_State* b,A_Boolean* result) {
    if(compare_error)return 1;
    *result=std::memcmp(a,b,sizeof(*a))==0;return 0;
}
NodeId id(unsigned n) {NodeId x{};x.value.bytes[15]=static_cast<unsigned char>(n);return x;}
Result<double> rate(double t){++samples;return Result<double>::success(10+t*t);}
}
int main() {
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    utils.PF_GetCurrentState=state;utils.PF_AreStatesIdentical=same;
    PF_InData data{};data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.pica_basicP=&basic;
    NeverCancelled never;PF_State stamp{};state(data.effect_ref,91,nullptr,nullptr,&stamp);
    NativeControlProof proof{id(1),91,stamp,true,EmissionRateProfile{10000,{}},2};
    remember_native_control_proofs(&data,{proof});
    auto valid=validated_native_control_proofs(&data);
    check(valid.size()==1 && valid[0].constant && valid[0].rate->constant==10000,"matching authored proof validates");
    ++epochs[91];check(validated_native_control_proofs(&data).empty(),"value/key/expression dependency change rejects proof");
    --epochs[91];compare_error=true;check(validated_native_control_proofs(&data).empty(),"comparison error cannot certify constant");compare_error=false;
    for(unsigned hz:{30u,60u,120u}) {
        auto timeline=native_emission_timeline(&data,id(hz),92,hz);
        check(bool(timeline),"prefix lease available");
        check(timeline->configure(hz).has_value(),"configure cached prefix");
        std::uint64_t work=0;samples=0;
        check(timeline->extend(2,rate,never,work).has_value(),"first frame builds prefix");
        check(samples==int(hz*6),"first frame queries fixed lattice only");
        auto held=timeline.get();
        check(!native_emission_timeline(&data,id(hz),92,hz),"contention chooses uncached fallback without waiting");
        const auto old_integral=timeline->integral(2);timeline.reset();
        auto reused=native_emission_timeline(&data,id(hz),92,hz);
        check(reused.get()==held,"unchanged dependency reuses completed prefix");
        samples=0;work=0;check(reused->extend(2.0+1.0/hz,rate,never,work).has_value(),"next frame extends one interval");
        check(samples==3,"next frame does not resample previous history");
        samples=0;check(reused->extend(1,rate,never,work).has_value() && samples==0,"reverse seek reuses earlier prefix");
        check(reused->integral(2)==old_integral,"prefix is byte-stable across extension");
        EmissionTimeline fresh;check(fresh.configure(hz).has_value(),"uncached comparison configured");work=0;
        check(fresh.extend(2.0+1.0/hz,rate,never,work).has_value(),"uncached comparison extended");
        check(fresh.integral(2.0+1.0/hz)==reused->integral(2.0+1.0/hz),"cached and uncached integral identical");
        for(unsigned k=0;k<20;++k)check(fresh.birth(k)==reused->birth(k),"cached and uncached birth identity identical");
        ++epochs[92];auto changed=native_emission_timeline(&data,id(hz),92,hz);
        check(changed && changed.get()!=reused.get() && !changed->initialized(),"edited curve cannot use old prefix");
        --epochs[92];
    }
    auto oversized=native_emission_timeline(&data,id(4),93,30);
    check(oversized && oversized->configure(30).has_value(),"bounded cache fixture ready");
    std::uint64_t work=0;check(oversized->extend(5000,[](double){return Result<double>::success(10);},never,work).has_value(),"long timeline can compute uncached tail");
    oversized.reset();auto bounded=native_emission_timeline(&data,id(4),93,30);
    check(bounded && !bounded->initialized(),"oversized retained prefix is evicted at release");bounded.reset();
    unavailable=true;check(!native_emission_timeline(&data,id(8),94,30),"missing suite keeps uncached behavior");
    check(validated_native_control_proofs(&data).empty(),"missing suite cannot certify stale metadata");unavailable=false;
    compare_error=true;check(!validated_native_control_proofs(&data).size(),"readback comparison failure declines optimization");compare_error=false;
    check(references==0,"all ParamUtils suite acquisitions released");
    std::printf("Emission cache: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
