#include "StdAfx.h"
#include "Server.h"

#define __API_IMPL__
#include "API_Server.h"

uint Timer_GameTick() {
	return Timer::GameTick();
}
uint Timer_FastTick() {
	return Timer::FastTick();
}

ServerGameOptions* Server_GameOptions() {
    return &GameOpt;
}
