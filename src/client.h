#ifndef _client_h_
#define _client_h_

#include <stddef.h>

/*
 * C multiplayer client boundary (ADR-001 Stretch / GAME_MAP S9).
 *
 * Owns:
 *   - TCP connect / send / recv queue (online mode only)
 *   - Client → server protocol framing (v1 ASCII lines; see ADR-001)
 *
 * Does NOT own:
 *   - Server → client parse / world apply (main.c parse_buffer)
 *   - Offline place/break/save authority (main.c + db.c)
 *   - Online world authority / persistence (Python server.py)
 *   - Auth HTTPS token fetch (auth.c); this module only sends A,...
 *
 * Wire shapes must match ADR-001 / server.py. Do not invent opcodes here.
 * docs/PROTOCOL.md was not present in-tree at this slice; contract summary
 * lives in docs/ADR-001-modular-craft.md.
 */

#define DEFAULT_PORT 4080
#define CLIENT_PROTOCOL_VERSION 1

void client_enable();
void client_disable();
int get_client_enabled();
void client_connect(char *hostname, int port);
void client_start();
void client_stop();
void client_send(char *data);
char *client_recv();
void client_version(int version);
void client_login(const char *username, const char *identity_token);
void client_position(float x, float y, float z, float rx, float ry);
void client_chunk(int p, int q, int key);
void client_block(int x, int y, int z, int w);
void client_light(int x, int y, int z, int w);
void client_sign(int x, int y, int z, int face, const char *text);
void client_talk(const char *text);

/* Pure client→server framers (no I/O). Return bytes written (excl. NUL), or -1. */
int client_fmt_version(char *buf, size_t size, int version);
int client_fmt_login(char *buf, size_t size,
    const char *username, const char *identity_token);
int client_fmt_position(char *buf, size_t size,
    float x, float y, float z, float rx, float ry);
int client_fmt_chunk(char *buf, size_t size, int p, int q, int key);
int client_fmt_block(char *buf, size_t size, int x, int y, int z, int w);
int client_fmt_light(char *buf, size_t size, int x, int y, int z, int w);
int client_fmt_sign(char *buf, size_t size,
    int x, int y, int z, int face, const char *text);
int client_fmt_talk(char *buf, size_t size, const char *text);

#endif
