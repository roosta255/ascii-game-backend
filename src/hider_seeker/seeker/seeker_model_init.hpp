#pragma once

#include <vector>
#include <string>
#include <ostream>

namespace hider_seeker {

int runSeekerModelInit(const std::vector<std::string>& args, std::ostream& out, std::ostream& err);

} // namespace hider_seeker