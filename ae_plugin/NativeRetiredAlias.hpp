#pragma once
#include <string_view>

namespace starfield::adapter::native_retired_alias {
// The126 ownership prefix deliberately remains recognizable after rollback.
// Disabled, source-free texts; alternate to invalidate a cached dependency even
// when the previous retirement already left this stream disabled and finite.
inline constexpr std::u16string_view a =
    u"var result = -1099511627776; var fx = null; var match = false;\n"
    u"/* retired A */\nvalue;";
inline constexpr std::u16string_view b =
    u"var result = -1099511627776; var fx = null; var match = false;\n"
    u"/* retired B */\nvalue;";
constexpr bool owns(std::u16string_view text) noexcept {return text==a || text==b;}
constexpr std::u16string_view next(std::u16string_view previous) noexcept {return previous==a?b:a;}
static_assert(a!=b && next(a)==b && next(b)==a);
}
