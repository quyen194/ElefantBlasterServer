/********************************************************************
  Copyright 2026, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2026/01/12 07:04
  filename:  ElefantBlaster/ElefantBlasterServer/entities/user.hpp

  purpose:   Header file for user entity
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_ENTITIES_USER_HPP
#define ELEFANT_BLASTER_SERVER_ENTITIES_USER_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include <time.h>
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
namespace _UserType {
  enum T {
    kUnknown,
    kAdmin,
    kPlayer,
  };
}
typedef _UserType::T UserType;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

struct User {
  std::uint64_t id;
  UserType type;
  std::string username;
  std::string password_hash;
  std::string display_name;
  std::string api_token;
  bool is_banned;
  std::string ban_reason;
  time_t banned_until;
  time_t created_at;
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_ENTITIES_USER_HPP
// -----------------------------------------------------------------------------
