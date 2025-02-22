/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.org/
*/

#include "pch.hpp"

#include "creatures/appearance/mounts/mounts.h"
#include "database/databasetasks.h"
#include "game/game.h"
#include "game/scheduling/scheduler.h"
#include "game/scheduling/tasks.h"
#include "lua/creature/events.h"
#include "lua/creature/raids.h"
#include "lua/scripts/lua_environment.hpp"
#include "server/signals.h"


Signals::Signals(asio::io_service& service) :
	set(service)
{
	set.add(SIGINT);
	set.add(SIGTERM);
#ifndef _WIN32
	set.add(SIGUSR1);
	set.add(SIGHUP);
#else
	// This must be a blocking call as Windows calls it in a new thread and terminates
	// the process when the handler returns (or after 5 seconds, whichever is earlier).
	// On Windows it is called in a new thread.
	signal(SIGBREAK, dispatchSignalHandler);
#endif

	asyncWait();
}

void Signals::asyncWait()
{
	set.async_wait([this] (std::error_code err, int signal) {
		if (err) {
			SPDLOG_ERROR("[Signals::asyncWait] - "
                         "Signal handling error: {}", err.message());
			return;
		}
		dispatchSignalHandler(signal);
		asyncWait();
	});
}

// On Windows this function does not need to be signal-safe,
// as it is called in a new thread.
// https://github.com/otland/forgottenserver/pull/2473
void Signals::dispatchSignalHandler(int signal)
{
	switch(signal) {
		case SIGINT: //Shuts the server down
			g_dispatcher().addTask(createTask(sigintHandler));
			break;
		case SIGTERM: //Shuts the server down
			g_dispatcher().addTask(createTask(sigtermHandler));
			break;
#ifndef _WIN32
		case SIGHUP: //Reload config/data
			g_dispatcher().addTask(createTask(sighupHandler));
			break;
		case SIGUSR1: //Saves game state
			g_dispatcher().addTask(createTask(sigusr1Handler));
			break;
#else
		case SIGBREAK: //Shuts the server down
			g_dispatcher().addTask(createTask(sigbreakHandler));
			// hold the thread until other threads end
			g_scheduler().join();
			g_databaseTasks().join();
			g_dispatcher().join();
			break;
#endif
		default:
			break;
	}
}

void Signals::sigbreakHandler()
{
	//Dispatcher thread
	SPDLOG_INFO("SIGBREAK received, shutting game server down...");
	g_game().setGameState(GAME_STATE_SHUTDOWN);
}

void Signals::sigtermHandler()
{
	//Dispatcher thread
	SPDLOG_INFO("SIGTERM received, shutting game server down...");
	g_game().setGameState(GAME_STATE_SHUTDOWN);
}

void Signals::sigusr1Handler()
{
	//Dispatcher thread
	SPDLOG_INFO("SIGUSR1 received, saving the game state...");
	g_game().saveGameState();
}

void Signals::sighupHandler()
{
	//Dispatcher thread
	SPDLOG_INFO("SIGHUP received, reloading config files...");

	g_configManager().reload();
	SPDLOG_INFO("Reloaded config");

	g_game().raids.reload();
	g_game().raids.startup();
	SPDLOG_INFO("Reloaded raids");

	Item::items.reload();
	SPDLOG_INFO("Reloaded items");

	g_game().mounts.reload();
	SPDLOG_INFO("Reloaded mounts");

	g_events().loadFromXml();
	SPDLOG_INFO("Reloaded events");

	g_chat().load();
	SPDLOG_INFO("Reloaded chatchannels");

	g_luaEnvironment.loadFile(g_configManager().getString(CORE_DIRECTORY) + "/core.lua", "core.lua");
	SPDLOG_INFO("Reloaded core.lua");

	lua_gc(g_luaEnvironment.getLuaState(), LUA_GCCOLLECT, 0);
}

void Signals::sigintHandler()
{
	//Dispatcher thread
	SPDLOG_INFO("SIGINT received, shutting game server down...");
	g_game().setGameState(GAME_STATE_SHUTDOWN);
}
