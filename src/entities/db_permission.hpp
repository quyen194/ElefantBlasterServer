/********************************************************************
  Copyright 2026, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2026/01/16 15:28
  filename:  ElefantBlaster/ElefantBlasterServer/entities/db_permission.hpp

  purpose:   Header file for database permission entity
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_ENTITIES_DB_PERMISSION_HPP
#define ELEFANT_BLASTER_SERVER_ENTITIES_DB_PERMISSION_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <cstdint>

#include <entities/permission.hpp>
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------

struct DbPermission : public Permission {
  std::uint64_t id = 0;
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_ENTITIES_DB_PERMISSION_HPP
// -----------------------------------------------------------------------------
