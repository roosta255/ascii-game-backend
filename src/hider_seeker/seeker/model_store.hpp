#pragma once

#include <string>
#include "seeker_model.hpp"

namespace hider_seeker {

// Forward declarations
struct SeekerModel;
std::string seekerModelBytes(const SeekerModel& model);
bool seekerModelFromBytes(const std::string& bytes, SeekerModel& model);

// Write a seeker model to disk in the specified version directory.
// If activate is true, also update the ACTIVE file to point to this version.
// Returns false if the version directory already exists or on any error.
bool writeSeekerModel(const std::string& root, const std::string& version,
  const SeekerModel& model, uint64_t seed, bool activate,
  std::string& error);

// Load the active seeker model from disk.
// Returns false if the ACTIVE file is missing/empty or any other error occurs.
bool loadActiveSeekerModel(const std::string& root,
  SeekerModel& model, std::string& version,
  std::string& error);

} // namespace hider_seeker