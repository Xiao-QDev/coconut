#pragma once
#include "value.h"

void net_init();
Value net_http_get(int argc, Value *argv);
Value net_listen(int argc, Value *argv);
