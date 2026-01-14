/********************************************************************
  Copyright 2014, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/15 19:28
  filename:  ElefantBlaster/ElefantBlasterServer/network/admin/admin_client_info.hpp

  purpose:   Header file for admin client information
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_NETWORK_ADMIN_ADMIN_CLIENT_INFO_HPP
#define ELEFANT_BLASTER_SERVER_NETWORK_ADMIN_ADMIN_CLIENT_INFO_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <cstdint>
#include <string>

#include "entities/user.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
typedef websocketpp::connection_hdl connection_hdl;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

struct AdminClientInfo {
  std::uint64_t index;
  connection_hdl hdl;

  User user;
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_NETWORK_ADMIN_ADMIN_CLIENT_INFO_HPP
// -----------------------------------------------------------------------------
