#pragma once

#include <string_view>

namespace ope {

// Library version, taken from the project() call in the top-level CMakeLists.txt.
[[nodiscard]] std::string_view version() noexcept;

}  // namespace ope
