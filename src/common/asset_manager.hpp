/********************************************************************
  Copyright 2025, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/12/02 07:43
  filename:  ElefantBlasterServer/src/common/asset_manager.hpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ElefantBlasterServer_src_common_asset_manager_hpp
#define ElefantBlasterServer_src_common_asset_manager_hpp
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <filesystem>
#include <string>

#include <aries_base/definitions/macro.hpp>
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
namespace common {
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

class AssetManager {
 public:
  AssetManager();
  ~AssetManager();

  static std::filesystem::path R(std::filesystem::path path);

 private:
  DISALLOW_COPY_AND_ASSIGN(AssetManager);
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
}  // namespace common
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ElefantBlasterServer_src_common_asset_manager_hpp
// -----------------------------------------------------------------------------
