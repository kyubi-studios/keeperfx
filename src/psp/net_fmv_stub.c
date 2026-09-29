/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file net_fmv_stub.c
 *     PSP replacements for the networking (enet/curl/upnp) and FMV (ffmpeg)
 *     backends, which are not built for the PSP. Networking reports itself
 *     unavailable; FMV playback is skipped as if the video had finished.
 */
/******************************************************************************/
#include "pre_inc.h"
#include <stdbool.h>
#include "bflib_enet.h"
#include "bflib_fmvids.h"
#include "net_lan.h"
#include "net_matchmaking.h"
#include "net_portforward.h"
#include "post_inc.h"

/* ---- bflib_enet ---- */
uint16_t enet_port = ENET_DEFAULT_PORT;
uint16_t external_ipv4_port = 0;
char external_ipv4_address[64] = "";
int skip_holepunch = 1;

struct NetSP* InitEnetSP() { return NULL; }
unsigned long GetPing(int id) { return 0; }
unsigned int GetPacketLoss(int id) { return 0; }
unsigned int GetClientDataInTransit() { return 0; }
unsigned int GetClientPacketsLost() { return 0; }
unsigned int GetUploadRateBytesPerSecond() { return 0; }
unsigned int GetDownloadRateBytesPerSecond() { return 0; }
int enet_matchmaking_host_update(void) { return 0; }
uint16_t enet_get_bound_ipv6_port(void) { return 0; }

/* ---- net_lan ---- */
struct TbNetworkSessionNameEntry lan_sessions[LAN_SESSIONS_MAX];
int lan_session_count = 0;

void lan_host_start(const char *name, uint16_t port) {}
void lan_host_update(void) {}
void lan_refresh_sessions(void) { lan_session_count = 0; }
void lan_shutdown(void) {}
void lan_set_lobby_id(const char *id) {}

/* ---- net_matchmaking ---- */
struct TbNetworkSessionNameEntry matchmaking_sessions[MATCHMAKING_SESSIONS_MAX];
TbBool matchmaking_enabled = false;
char matchmaking_ws_url[MATCHMAKING_URL_MAX] = "";
char matchmaking_ip_url[MATCHMAKING_URL_MAX] = "";
int matchmaking_session_count = 0;
char join_lobby_id[MATCHMAKING_ID_MAX] = "";

void matchmaking_set_server(const char* host) {}
void matchmaking_connect_async(void) {}
int matchmaking_connect(void) { return -1; }
int matchmaking_request_list(void) { return -1; }
void matchmaking_disconnect(void) {}
void matchmaking_finish_lobby(enum MatchmakingLobbyResult result, int map_number, const char *map_name) {}
void matchmaking_refresh_sessions(void) { matchmaking_session_count = 0; }
int matchmaking_create(const char *name, const char *udp_ipv4, int udp_ipv4_port, int udp_ipv6_port, int direct_ipv4_port) { return -1; }
int matchmaking_punch(const char *lobby_id, const char *udp_ipv4, int udp_ipv4_port, int udp_ipv6_port, PunchAddresses *output) { return -1; }
int matchmaking_poll_punch(PunchAddresses *output) { return -1; }

/* ---- net_portforward ---- */
int port_forward_add_mapping(uint16_t port) { return 0; }
void port_forward_remove_mapping(void) {}

/* ---- bflib_fmvids ---- */
TbBool play_smk(const char * filename, int flags) { return true; }
short anim_stop(void) { return 0; }
short anim_record(void) { return 0; }
TbBool anim_record_frame(unsigned char * screenbuf, unsigned char * palette) { return false; }
