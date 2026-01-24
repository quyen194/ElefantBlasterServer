/********************************************************************
  Copyright 2026, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2026/01/18 07:56
  filename:  ElefantBlaster/ElefantBlasterServer/entities/db_group.hpp

  purpose:   Header file for database group entity
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_ENTITIES_DB_GROUP_HPP
#define ELEFANT_BLASTER_SERVER_ENTITIES_DB_GROUP_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <entities/group.hpp>
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------

struct DbGroup : public Group {
  time_t created_at = 0;
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_ENTITIES_DB_GROUP_HPP
// -----------------------------------------------------------------------------
