/********************************************************************
  Copyright 2026, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2026/01/12 07:04
  filename:  ElefantBlaster/ElefantBlasterServer/entities/db_user.hpp

  purpose:   Header file for database user entity
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_ENTITIES_DB_USER_HPP
#define ELEFANT_BLASTER_SERVER_ENTITIES_DB_USER_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <entities/user.hpp>
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------

struct DbUser : public User {
  std::uint64_t id;
  time_t created_at;
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_ENTITIES_DB_USER_HPP
// -----------------------------------------------------------------------------
