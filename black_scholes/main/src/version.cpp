#include "ope/version.hpp"

#ifndef OPE_VERSION_STRING
#error "OPE_VERSION_STRING must be defined by the build system"
#endif

namespace ope {

std::string_view version() noexcept { return OPE_VERSION_STRING; }

}  // namespace ope
