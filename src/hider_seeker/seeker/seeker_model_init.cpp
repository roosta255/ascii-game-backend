#include "seeker_model_init.hpp"
#include "model_store.hpp"
#include "seeker_model.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace hider_seeker {

int runSeekerModelInit(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) {
    std::string root;
    std::string version;
    int64_t seed = 0;
    bool activate = false;

    // Parse arguments
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--root" && i + 1 < args.size()) {
            root = args[++i];
        } else if (args[i] == "--version" && i + 1 < args.size()) {
            version = args[++i];
        } else if (args[i] == "--seed" && i + 1 < args.size()) {
            try {
                seed = std::stoll(args[++i]);
                if (seed < 0) {
                    return 2;
                }
            } catch (...) {
                return 2;
            }
        } else if (args[i] == "--activate") {
            activate = true;
        }
    }

    // Check required arguments
    if (root.empty() || version.empty()) {
        return 2;
    }

    // Create the model
    SeekerModel model = makeRandomSeekerModel(seed);
    
    // Write the model
    std::string error;
    if (!writeSeekerModel(root, version, model, seed, activate, error)) {
        err << error << "\n";
        return 1;
    }

    // On success, print one line to out with version, root, and activation status
    out << version << " " << root << " " << (activate ? "active" : "inactive") << "\n";
    return 0;
}

} // namespace hider_seeker