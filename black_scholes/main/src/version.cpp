#include "ope/version.hpp"

#ifndef OPE_VERSION_STRING
#error "OPE_VERSION_STRING must be defined by the build system"
#endif

using std::string_view;

namespace ope {

string_view version() noexcept { return OPE_VERSION_STRING; }

}  // namespace ope
