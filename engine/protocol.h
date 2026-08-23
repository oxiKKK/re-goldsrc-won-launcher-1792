// protocol.h -- communications protocols
#ifndef PROTOCOL_H
#define PROTOCOL_H
#ifdef _WIN32
#pragma once
#endif

// This is used, unless overridden in the registry
#define VALVE_MASTER_ADDRESS "half-life.east.won.net:27010"

#define MASTER_PARSE_FILE "woncomm.lst"

// Built-in WON master host the launcher falls back to when woncomm.lst supplies
// no master-list entry (CFavorites::GetAddrA/GetAddrB).
#define WON_MASTER_DEFAULT_HOST	"half-life.west.won.net"

// Default ports for the woncomm.lst comm-server block types, used when an entry
// omits ":port" (ParseServers).  PORT_WON_AUTH is the same WON directory server
// as PORT_DIR below.
#define PORT_WON_TITAN		6001	// Titan account / session server
#define PORT_WON_AUTH		6002	// Auth (directory) server
#define PORT_WON_MASTER		27010	// master server-list
#define PORT_WON_MODSERVER	27011	// mod / announce server

// Default WON chat-client service ports (CChatClient: IRC / chat / directory),
// used unless a woncomm.lst entry overrides them.
#define PORT_IRC	6667	// IRC chat server
#define PORT_CHAT	2667	// WON chat relay
#define PORT_DIR	6002	// WON directory server

//
// Out-of-band query opcodes.  Valve kept the WON-era set intact all the way into
// the Source SDK, so these are its own names from common/proto_oob.h; only the
// ones this launcher sends or receives are listed.
//
#define A2A_ACK					'j'	// general acknowledgement without info
#define A2M_GETMASTERSERVERS	'v'	// + byte type of request
#define A2M_GET_SERVERS_BATCH	'e'	// + int32 uniqueID (the resume token)
#define A2M_GET_SERVERS_BATCH2	'1'	// new-style query: + int32 uniqueID + filter
#define M2A_SERVER_BATCH		'f'	// + int32 next uniqueID, then 6-byte IP/port entries
#define M2A_MASTERSERVERS		'w'	// + byte type + 6-byte IP/port list

//
// server to client
//
#define	svc_nop					1		// Padding / keepalive

//
// client to server
//
#define	clc_stringcmd			3		// [string] message

// How many data slots to use when in multiplayer (must be power of 2)
#define MULTIPLAYER_BACKUP		(1<<6)
// Same for single player
#define SINGLEPLAYER_BACKUP		(1<<3)

#endif // PROTOCOL_H