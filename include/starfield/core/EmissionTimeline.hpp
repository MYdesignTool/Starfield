#pragma once

#include "starfield/core/Render.hpp"
#include "starfield/core/Settings.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace starfield::core {

enum class RateInterpolation : std::uint8_t { hold, linear };
struct EmissionRateKey {
    double seconds{}, value{};
    RateInterpolation outgoing{RateInterpolation::linear};
};
struct EmissionRateProfile {
    // Set by a metadata reader only when no authored expression is enabled.
    // Empty keys describe a constant; nonlinear/expression curves use sampling.
    double constant{};
    std::vector<EmissionRateKey> keys;
};

// Caller owns the cache and its dependency identity. Reuse across frames is legal
// only while the complete rate dependency state and Hz are unchanged. The class
// has no global state and never infers constancy from equal sampled values.
class EmissionTimeline {
    struct Segment {double begin{},end{},left{},right{};long double prefix{};};
    std::vector<Segment> segments_;
    std::uint32_t hz_{30};
    bool analytic_{};
    double tail_begin_{},tail_rate_{},first_positive_{-1};
    long double accumulated_{};
    static bool valid_rate(double rate) noexcept {
        return std::isfinite(rate) && rate>=0 && rate<=kMaxBirthRate;
    }
    void append(double begin,double end,double left,double right) {
        if(first_positive_<0 && (left>0 || right>0)) first_positive_=begin;
        segments_.push_back({begin,end,left,right,accumulated_});
        accumulated_+=(static_cast<long double>(left)+right)*(end-begin)/2;
    }
public:
    [[nodiscard]] std::uint32_t frequency() const noexcept {return hz_;}
    [[nodiscard]] double first_positive() const noexcept {return first_positive_;}
    [[nodiscard]] std::size_t segment_count() const noexcept {return segments_.size();}
    [[nodiscard]] bool analytic() const noexcept {return analytic_;}
    [[nodiscard]] double initial_rate() const noexcept {
        return segments_.empty()?tail_rate_:segments_.front().left;
    }
    [[nodiscard]] bool initialized() const noexcept {return analytic_ || !segments_.empty();}
    [[nodiscard]] Result<bool> configure(std::uint32_t hz,const EmissionRateProfile* profile=nullptr) {
        using R=Result<bool>;
        if(hz!=30 && hz!=60 && hz!=120) return R::failure(ErrorCode::invalid_request,"time sampling must be 30, 60 or 120 Hz");
        segments_.clear();hz_=hz;analytic_=profile!=nullptr;
        tail_begin_=tail_rate_=0;first_positive_=-1;accumulated_=0;
        if(!profile) return R::success(true);
        if(!valid_rate(profile->constant) || profile->keys.size()>100000)
            return R::failure(ErrorCode::invalid_request,"invalid emission metadata");
        if(profile->keys.empty()) {
            tail_rate_=profile->constant;
            if(tail_rate_>0) first_positive_=0;
            return R::success(true);
        }
        const auto& keys=profile->keys;
        for(std::size_t i=0;i<keys.size();++i) {
            if(!std::isfinite(keys[i].seconds) || !valid_rate(keys[i].value) ||
               (i && keys[i].seconds<=keys[i-1].seconds) ||
               (keys[i].outgoing!=RateInterpolation::linear && keys[i].outgoing!=RateInterpolation::hold))
                return R::failure(ErrorCode::invalid_request,"invalid linear emission keys");
        }
        if(keys.front().seconds>0) append(0,keys.front().seconds,keys.front().value,keys.front().value);
        for(std::size_t i=1;i<keys.size();++i) {
            const auto& a=keys[i-1];const auto& b=keys[i];
            if(b.seconds<=0) continue;
            const double begin=std::max(0.0,a.seconds);
            const double right=a.outgoing==RateInterpolation::hold?a.value:b.value;
            const double fraction=(begin-a.seconds)/(b.seconds-a.seconds);
            append(begin,b.seconds,a.value+(right-a.value)*fraction,right);
        }
        tail_begin_=std::max(0.0,keys.back().seconds);tail_rate_=keys.back().value;
        if(first_positive_<0 && tail_rate_>0) first_positive_=tail_begin_;
        return R::success(true);
    }
    [[nodiscard]] Result<bool> extend(double now,const std::function<Result<double>(double)>& rate,
                                      const Cancellation& cancel,std::uint64_t& work,
                                      std::uint64_t work_limit=20'000'000) {
        using R=Result<bool>;
        if(!std::isfinite(now)) return R::failure(ErrorCode::invalid_time,"invalid emission time");
        if(cancel.is_cancelled()) return R::failure(ErrorCode::cancelled,"emission history cancelled");
        if(analytic_ || now<0) return R::success(true);
        const double needed=std::ceil(now*hz_);
        if(needed>2'000'000) return R::failure(ErrorCode::work_limit_exceeded,"emission timeline exceeds bounded duration");
        // Fixed lattice and stable left-to-right accumulation make old prefixes
        // byte-identical to an uncached evaluation at the same frequency.
        while(segments_.size()<static_cast<std::size_t>(needed)) {
            if(cancel.is_cancelled()) return R::failure(ErrorCode::cancelled,"emission history cancelled");
            if(work>work_limit || work_limit-work<3) return R::failure(ErrorCode::work_limit_exceeded,"emission history work limit");
            work+=3;
            const double t=double(segments_.size())/hz_;
            auto left=rate(t),mid=rate(t+0.5/hz_),right=rate(t+1.0/hz_);
            if(!left.has_value()) return R::failure(left.error());
            if(!mid.has_value()) return R::failure(mid.error());
            if(!right.has_value()) return R::failure(right.error());
            double a=left.value(),b=right.value();
            if(!valid_rate(a) || !valid_rate(b) || !valid_rate(mid.value()))
                return R::failure(ErrorCode::invalid_request,"animated emission rate outside bounds");
            if(a!=b && mid.value()==a) b=a;
            else if(a!=b && mid.value()==b) a=b;
            append(t,t+1.0/hz_,a,b);
            if(accumulated_>9'007'199'254'740'991.0L)
                return R::failure(ErrorCode::invalid_time,"emission ordinal exceeds exact range");
        }
        return R::success(true);
    }
    [[nodiscard]] double integral(double now) const noexcept {
        if(now<=0) return 0;
        if(analytic_ && now>=tail_begin_)
            return double(accumulated_+static_cast<long double>(tail_rate_)*(now-tail_begin_));
        if(segments_.empty()) return 0;
        auto it=std::upper_bound(segments_.begin(),segments_.end(),now,
            [](double t,const Segment& s){return t<s.begin;});
        if(it!=segments_.begin()) --it;
        const double dt=std::clamp(now-it->begin,0.0,it->end-it->begin);
        const double slope=(it->right-it->left)/(it->end-it->begin);
        return double(it->prefix+static_cast<long double>(it->left)*dt+0.5L*slope*dt*dt);
    }
    [[nodiscard]] double birth(std::uint64_t ordinal) const noexcept {
        if(!ordinal) return first_positive_;
        const long double target=static_cast<long double>(ordinal);
        // The evaluator admits thresholds within 1e-10 of an integer. Match
        // that admission when lattice widths round just below the threshold.
        if(analytic_ && target>accumulated_+1e-10L) return tail_rate_>0?
            tail_begin_+double((target-accumulated_)/tail_rate_):-1;
        if(segments_.empty()) return analytic_ && tail_rate_>0?double(target/tail_rate_):-1;
        auto it=std::lower_bound(segments_.begin(),segments_.end(),target,
            [](const Segment& s,long double value){
                return s.prefix+(static_cast<long double>(s.left)+s.right)*(s.end-s.begin)/2<value-1e-10L;
            });
        if(it==segments_.end()) return -1;
        const double amount=std::max(0.0,double(target-it->prefix));
        const double slope=(it->right-it->left)/(it->end-it->begin);
        double dt;
        if(std::abs(slope)<1e-12) dt=it->left>0?amount/it->left:0;
        else {
            const double denominator=it->left+std::sqrt(std::max(0.0,it->left*it->left+2*slope*amount));
            dt=denominator>0?2*amount/denominator:0;
        }
        double fraction=std::clamp(dt/(it->end-it->begin),0.0,1.0);
        if(fraction<1e-10) fraction=0;
        if(fraction>1-1e-10) fraction=1;
        return it->begin+fraction*(it->end-it->begin);
    }
};
} // namespace starfield::core
