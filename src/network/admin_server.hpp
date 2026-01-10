/********************************************************************
  Copyright 2014, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2025/11/14 21:38
  filename:  ElefantBlaster/ElefantBlasterServer/network/admin_server.hpp

  purpose:   Header file for the admin server
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_NETWORK_ADMIN_SERVER_HPP
#define ELEFANT_BLASTER_SERVER_NETWORK_ADMIN_SERVER_HPP
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#include <cstdint>
#include <mutex>
#include <set>
#include <string>

#include <spdlog/spdlog.h>

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include <aries_base/definitions/macro.hpp>
#include <aries_base/process/event/event.hpp>

#include "common/settings_manager.hpp"
#include "network/connection_info.hpp"
#include "storage/db_manager.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace aries_base::process;
// -----------------------------------------------------------------------------
typedef websocketpp::config::asio::message_type::ptr message_ptr;
typedef websocketpp::connection_hdl connection_hdl;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
typedef websocketpp::server<websocketpp::config::asio_tls> websocket_server;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

class AdminServer {
 public:
  AdminServer(DBManager *db_manager);
  virtual ~AdminServer();

  bool LoadConfig();

  bool Start();
  void Stop();
  bool Active();
  void Deactive();

  void DisconnectAllClients(const std::string &reason);

 public:
  context_ptr OnTlsInit(connection_hdl hdl);
  void OnConnected(connection_hdl hdl);
  void OnDisconnected(connection_hdl hdl);
  void OnError(connection_hdl hdl);
  void OnDataRecv(connection_hdl hdl, message_ptr msg);

  bool LoadTlsData();
  std::string ReadFileToString(const std::string &file_path);

 private:
  void Worker();

 private:
  websocket_server server_;
  std::string host_;  // IP to bind
  int port_;

  std::string cert_file_password_;
  std::string cert_file_path_;
  std::string cert_file_data_;
  std::string key_file_path_;
  std::string key_file_data_;
  std::string dh_params_file_path_;
  std::string dh_params_file_data_;
  std::string ciphers_;

  std::set<connection_hdl, std::owner_less<connection_hdl>> connections_;
  std::mutex connections_mutex_;

  Event worker_end_event_;

 private:
  SettingsManager* settings_;
  DBManager* db_manager_;
  std::shared_ptr<spdlog::logger> logger_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AdminServer);
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_NETWORK_ADMIN_SERVER_HPP
// -----------------------------------------------------------------------------
