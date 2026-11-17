#pragma once
#include <optional>
#include <utility>

namespace starfield::core::evaluation_storage {
// Emplace a factory's prvalue directly into an owned optional. Binding the
// factory call as an emplace argument would materialize a large stack value.
// The conversion proxy lives only for this call; the factory is never retained.
template<class T> class FrameValue {
    std::optional<T> value_;
    template<class Factory> struct FromFactory {
        Factory& make;
        operator T() const {return make();}
    };
public:
    // Avoid value-initializing inactive large payload storage in a heap frame.
    FrameValue() {}
    FrameValue(const FrameValue&)=delete;
    FrameValue& operator=(const FrameValue&)=delete;
    [[nodiscard]] bool has_value() const noexcept {return value_.has_value();}
    void clear() noexcept {value_.reset();}
    template<class Factory> T& replace(Factory&& make) {
        return value_.emplace(FromFactory<Factory>{make});
    }
};
template<class Values> struct FrameScope {
    Values& values;
    explicit FrameScope(Values& owned):values(owned){}
    FrameScope(const FrameScope&)=delete;
    FrameScope& operator=(const FrameScope&)=delete;
    ~FrameScope() noexcept {values.clear();}
};
}
