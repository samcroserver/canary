/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.org/
*/

#include "pch.hpp"

#include "declarations.hpp"
#include "game/game.h"
#include "lua/scripts/luascript.h"
#include "lua/scripts/script_environment.hpp"

ScriptEnvironment::ScriptEnvironment() {
	resetEnv();
}

ScriptEnvironment::~ScriptEnvironment() {
	resetEnv();
}

void ScriptEnvironment::resetEnv() {
	scriptId = 0;
	callbackId = 0;
	timerEvent = false;
	interface = nullptr;
	localMap.clear();
	tempResults.clear();

	auto pair = tempItems.equal_range(this);
	auto it = pair.first;
	while (it != pair.second) {
		Item * item = it -> second;
		if (item -> getParent() == VirtualCylinder::virtualCylinder) {
			g_game().ReleaseItem(item);
		}
		it = tempItems.erase(it);
	}
}

bool ScriptEnvironment::setCallbackId(int32_t newCallbackId, LuaScriptInterface * scriptInterface) {
	if (this -> callbackId != 0) {
		// nested callbacks are not allowed
		if (interface) {
			interface -> reportErrorFunc("Nested callbacks!");
		}
		return false;
	}

	this -> callbackId = newCallbackId;
	interface = scriptInterface;
	return true;
}

void ScriptEnvironment::getEventInfo(int32_t & retScriptId, LuaScriptInterface * & retScriptInterface, int32_t & retCallbackId, bool & retTimerEvent) const {
	retScriptId = this -> scriptId;
	retScriptInterface = interface;
	retCallbackId = this -> callbackId;
	retTimerEvent = this -> timerEvent;
}

uint32_t ScriptEnvironment::addThing(Thing * thing) {
	if (!thing || thing -> isRemoved()) {
		return 0;
	}

	Creature * creature = thing -> getCreature();
	if (creature) {
		return creature -> getID();
	}

	Item * item = thing -> getItem();
	if (item && item -> hasAttribute(ItemAttribute_t::UNIQUEID)) {
		return item->getAttribute<uint32_t>(ItemAttribute_t::UNIQUEID);
	}

	for (const auto & it: localMap) {
		if (it.second == item) {
			return it.first;
		}
	}

	localMap[++lastUID] = item;
	return lastUID;
}

void ScriptEnvironment::insertItem(uint32_t uid, Item * item) {
	auto result = localMap.emplace(uid, item);
	if (!result.second) {
		SPDLOG_ERROR("Thing uid already taken: {}", uid);
	}
}

Thing * ScriptEnvironment::getThingByUID(uint32_t uid) {
	if (uid >= 0x10000000) {
		return g_game().getCreatureByID(uid);
	}

	if (uid <= std::numeric_limits < uint16_t > ::max()) {
		Item * item = g_game().getUniqueItem(static_cast<uint16_t>(uid));
		if (item && !item -> isRemoved()) {
			return item;
		}
		return nullptr;
	}

	auto it = localMap.find(uid);
	if (it != localMap.end()) {
		Item * item = it -> second;
		if (!item -> isRemoved()) {
			return item;
		}
	}
	return nullptr;
}

Item * ScriptEnvironment::getItemByUID(uint32_t uid) {
	Thing * thing = getThingByUID(uid);
	if (!thing) {
		return nullptr;
	}
	return thing -> getItem();
}

Container * ScriptEnvironment::getContainerByUID(uint32_t uid) {
	Item * item = getItemByUID(uid);
	if (!item) {
		return nullptr;
	}
	return item -> getContainer();
}

void ScriptEnvironment::removeItemByUID(uint32_t uid) {
	if (uid <= std::numeric_limits < uint16_t > ::max()) {
		g_game().removeUniqueItem(static_cast<uint16_t>(uid));
		return;
	}

	auto it = localMap.find(uid);
	if (it != localMap.end()) {
		localMap.erase(it);
	}
}

void ScriptEnvironment::addTempItem(Item * item) {
	tempItems.emplace(this, item);
}

void ScriptEnvironment::removeTempItem(Item * item) {
	for (auto it = tempItems.begin(), end = tempItems.end(); it != end; ++it) {
		if (it -> second == item) {
			tempItems.erase(it);
			break;
		}
	}
}

uint32_t ScriptEnvironment::addResult(DBResult_ptr res) {
	tempResults[++lastResultId] = res;
	return lastResultId;
}

bool ScriptEnvironment::removeResult(uint32_t id) {
	auto it = tempResults.find(id);
	if (it == tempResults.end()) {
		return false;
	}

	tempResults.erase(it);
	return true;
}

DBResult_ptr ScriptEnvironment::getResultByID(uint32_t id) {
	auto it = tempResults.find(id);
	if (it == tempResults.end()) {
		return nullptr;
	}
	return it -> second;
}
