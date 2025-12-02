/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/12/02 07:44
  filename:  ElefantBlasterServer/src/common/asset_manager.cpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#include "common/asset_manager.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
namespace common {
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

AssetManager::AssetManager() {
}
// -----------------------------------------------------------------------------

AssetManager::~AssetManager() {
}
// -----------------------------------------------------------------------------

std::filesystem::path AssetManager::R(std::filesystem::path path) {
#ifdef __EMSCRIPTEN__
    std::string base_path = "/assets";
#elif defined(_WIN32) || defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    std::string base_path = "assets";
#endif

#if !defined(__IOS__)
  return std::filesystem::path(base_path) / path;
#else
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
  char base_path[PATH_MAX];
  if (CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8*)base_path, PATH_MAX)) {
      CFRelease(resourceURL);
      return std::filesystem::path(base_path) / path;
  }
  CFRelease(resourceURL);
  return path; // fallback
#endif
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
}  // namespace common
// -----------------------------------------------------------------------------
