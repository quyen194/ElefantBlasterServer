/********************************************************************
  Copyright 2026, Cong Quyen Knight. All rights reserved

  project:   Aries Games: Elefant Blaster
  author:    quyen19492
  email:     quyen19492@gmail.com

  created:   2026/01/15 09:43
  filename:  ElefantBlaster/ElefantBlasterServer/common/events.hpp

  purpose:
*********************************************************************/


// -----------------------------------------------------------------------------
#ifndef ELEFANT_BLASTER_SERVER_COMMON_EVENTS_HPP
#define ELEFANT_BLASTER_SERVER_COMMON_EVENTS_HPP
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#include <cstdint>
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
namespace _EventId {
  enum T {
    kUnknown,
    kAdminServer_ShutdownServer,
    kAdminServer_RestartServer,
    kAdminServer_ActiveGameServer,
    kAdminServer_DeactiveGameServer,
    kAdminServer_DisconnectAllGameClients,
  };
}
typedef _EventId::T EventId;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

struct EventBase {
};
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
#endif  // ELEFANT_BLASTER_SERVER_COMMON_EVENTS_HPP
// -----------------------------------------------------------------------------
