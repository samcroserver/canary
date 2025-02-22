/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.org/
*/

#include "pch.hpp"

#include "game/scheduling/scheduler.h"

void Scheduler::threadMain()
{
	std::unique_lock<std::mutex> eventLockUnique(eventLock, std::defer_lock);
	while (getState() != THREAD_STATE_TERMINATED) {
		std::cv_status ret = std::cv_status::no_timeout;

		eventLockUnique.lock();
		if (eventList.empty()) {
			eventSignal.wait(eventLockUnique, [this] {
				return eventList.empty();
			});
		} else {
			ret = eventSignal.wait_until(eventLockUnique, eventList.top()->getCycle());
		}

		// the mutex is locked again now...
		if (ret == std::cv_status::timeout && !eventList.empty()) {
			// ok we had a timeout, so there has to be an event we have to execute...
			SchedulerTask* task = eventList.top();
			eventList.pop();

			// check if the event was stopped
			auto it = eventIds.find(task->getEventId());
			if (it == eventIds.end()) {
				eventLockUnique.unlock();
				delete task;
				continue;
			}
			eventIds.erase(it);
			eventLockUnique.unlock();

			task->setDontExpire();
			g_dispatcher().addTask(task, true);
		} else {
			eventLockUnique.unlock();
		}
	}
}

uint32_t Scheduler::addEvent(SchedulerTask* task)
{
	bool do_signal;
	eventLock.lock();

	if (getState() == THREAD_STATE_RUNNING) {
		// check if the event has a valid id
		if (task->getEventId() == 0) {
			// if not generate one
			if (++lastEventId == 0) {
				lastEventId = 1;
			}

			task->setEventId(lastEventId);
		}

		// insert the event id in the list of active events
		eventIds.insert(task->getEventId());

		// add the event to the queue
		eventList.push(task);

		// if the list was empty or this event is the top in the list
		// we have to signal it
		do_signal = (task == eventList.top());
	} else {
		eventLock.unlock();
		delete task;
		return 0;
	}

	eventLock.unlock();

	if (do_signal) {
		eventSignal.notify_one();
	}

	return task->getEventId();
}

bool Scheduler::stopEvent(uint32_t eventid)
{
	if (eventid == 0) {
		return false;
	}

	std::lock_guard<std::mutex> lockClass(eventLock);

	// search the event id..
	auto it = eventIds.find(eventid);
	if (it == eventIds.end()) {
		return false;
	}

	eventIds.erase(it);
	return true;
}

void Scheduler::shutdown()
{
	setState(THREAD_STATE_TERMINATED);
	eventLock.lock();

	//this list should already be empty
	while (!eventList.empty()) {
		delete eventList.top();
		eventList.pop();
	}

	eventIds.clear();
	eventLock.unlock();
	eventSignal.notify_one();
}

SchedulerTask* createSchedulerTask(uint32_t delay, std::function<void (void)> f)
{
	return new SchedulerTask(delay, std::move(f));
}
