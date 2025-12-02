/********************************************************************
  Copyright 2014, Cong Quyen Knight. All rights reserved

  project:   Aries Games Project
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/15 19:28
  filename:  AriesGames\BomberManServer\src\network\connection_info.hpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ARIES_GAMES_BOMBERMANSERVER_SRC_NETWORK_CONNECTION_INFO_HPP
#define ARIES_GAMES_BOMBERMANSERVER_SRC_NETWORK_CONNECTION_INFO_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <cstdint>
#include <string>

#include <websocketpp/config/asio.hpp>
#include <websocketpp/connection.hpp>
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
typedef websocketpp::connection<websocketpp::config::asio_tls> connection;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

struct ConnectionInfo {
  int index;
  connection* connection_ptr;
  std::string player_name;
  std::string player_id;
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ARIES_GAMES_BOMBERMANSERVER_SRC_NETWORK_CONNECTION_INFO_HPP
// -----------------------------------------------------------------------------
