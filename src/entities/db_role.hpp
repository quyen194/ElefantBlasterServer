/********************************************************************
  Copyright 2026, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2026/01/18 06:04
  filename:  ElefantBlaster/ElefantBlasterServer/entities/db_role.hpp

  purpose:   Header file for database role entity
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_ENTITIES_DB_ROLE_HPP
#define ELEFANT_BLASTER_SERVER_ENTITIES_DB_ROLE_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <cstdint>

#include <entities/role.hpp>
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------

struct DbRole : public Role {
  std::uint64_t id;
  time_t created_at;
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_ENTITIES_DB_ROLE_HPP
// -----------------------------------------------------------------------------
