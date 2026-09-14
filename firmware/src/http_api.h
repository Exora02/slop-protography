#pragma once

#include <WebServer.h>

bool http_api_begin();
void http_api_loop();
void http_set_live_state(bool on);
