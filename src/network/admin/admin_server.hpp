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
#include <aries_base/process/ipc/mpmc_bounded_queue/ipc_client.hpp>
#include <aries_base/utils/bytes.hpp>

#include <network/shared/admin_protocols/client_protocol.pb.h>
#include <network/shared/admin_protocols/server_protocol.pb.h>

#include "common/settings_manager.hpp"
#include "network/admin/admin_client_info.hpp"
#include "storage/db_manager.hpp"
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
using namespace aries_base;
using namespace aries_base::process::ipc::mpmc_bounded_queue;
// -----------------------------------------------------------------------------
typedef websocketpp::config::asio::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
typedef websocketpp::server<websocketpp::config::asio_tls> websocket_server;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

class AdminServer : public IpcClient {
 public:
  AdminServer(DBManager *db_manager);
  virtual ~AdminServer();

  bool LoadConfig();

  bool Start();
  void Stop();
  void Active();
  void Deactive();

  bool Send(connection_hdl hdl, const protocol::ServerMessage &message);
  bool Send(connection_hdl hdl, const utils::bytes &data);

  void DisconnectAllClients(const std::string &reason);

 public:
  context_ptr OnTlsInit(connection_hdl hdl);
  bool OnValidate(connection_hdl hdl);
  void OnConnected(connection_hdl hdl);
  void OnDisconnected(connection_hdl hdl);
  void OnError(connection_hdl hdl);
  void OnMessage(connection_hdl hdl, message_ptr message);

  bool LoadTlsData();

 private:
  void Worker();

  std::shared_ptr<AdminClientInfo> AddConnection(connection_hdl hdl);
  void RemoveConnection(connection_hdl hdl);
  std::shared_ptr<AdminClientInfo> GetConnectionInfo(connection_hdl hdl);

 private:
  void OnLoginRequest(connection_hdl hdl, const admin_auth::LoginRequest& req);
  void OnShutDownServer(connection_hdl hdl);
  void OnRestartServer(connection_hdl hdl);
  void OnActiveGameServer(connection_hdl hdl);
  void OnDeactiveGameServer(connection_hdl hdl);
  void OnDisconnectAllGameClients(connection_hdl hdl);

 private:
  std::unique_ptr<websocket_server> server_;

  bool accept_client_;
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

  std::unordered_map<std::uint64_t, std::shared_ptr<AdminClientInfo>> map_id_connections_;
  std::map<connection_hdl, std::shared_ptr<AdminClientInfo>, std::owner_less<connection_hdl>> map_hdl_connections_;
  std::atomic<std::uint64_t> next_conn_id_;
  std::mutex connections_mutex_;

  process::Event worker_end_event_;

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
