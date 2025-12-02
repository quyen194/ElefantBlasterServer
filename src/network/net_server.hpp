/********************************************************************
  Copyright 2014, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/14 21:38
  filename:  ElefantBlaster/ElefantBlasterServer/network/net_server.hpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ARIES_GAMES_BOMBERMANSERVER_SRC_NETWORK_NET_SERVER_H
#define ARIES_GAMES_BOMBERMANSERVER_SRC_NETWORK_NET_SERVER_H
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#include <cstdint>
#include <string>

#include <spdlog/spdlog.h>

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include <aries_base/definitions/macro.hpp>

#include "common/settings_manager.hpp"
#include "network/connection_info.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace common;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
typedef websocketpp::config::asio::message_type::ptr message_ptr;
typedef websocketpp::connection_hdl connection_hdl;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
typedef websocketpp::server<websocketpp::config::asio_tls> websocket_server;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

class NetServer {
 public:
  NetServer();
  virtual ~NetServer();

  bool Start();
  void Stop();

 public:
  context_ptr OnTlsInit(connection_hdl hdl);
  void OnConnected(connection_hdl hdl);
  void OnDisconnected(connection_hdl hdl);
  void OnError(connection_hdl hdl);
  void OnDataRecv(connection_hdl hdl, message_ptr msg);

  bool LoadTlsData();
  std::string ReadFileToString(const std::string &file_path);

 private:
  websocket_server server_;
  int port_;

  std::string cert_file_password_;
  std::string cert_file_data_;
  std::string key_file_data_;
  std::string dh_params_file_data_;
  std::string ciphers_;

 private:
  SettingsManager* settings_;

 private:
  std::shared_ptr<spdlog::logger> logger_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetServer);
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ARIES_GAMES_BOMBERMANSERVER_SRC_NETWORK_NET_SERVER_H
// -----------------------------------------------------------------------------
