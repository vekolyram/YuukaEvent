#include <iostream>
#include <map>
#include <functional>
#include <future>
#include <queue>
#include <mutex>
#include <shared_mutex>
#define yuukaLock(x) std::unique_lock<std::shared_mutex> _lock = std::unique_lock<std::shared_mutex>(x)
enum EventRunType { AtOnce, Delay };
enum EventPriority { Low, High };
enum EventMode { Count, Repeat };
struct EventInfo {
private:
	int count = 1;
public:
	const int id = -1;
	const EventRunType type = AtOnce;
	const EventPriority priority = Low;
	const EventMode mode = Repeat;
	void CountSub() { count--; }
	int getCount() const { return count; }
	EventInfo(int id, EventRunType type, EventPriority priority, EventMode mode, int count = 1) :id(id), type(type), priority(priority), mode(mode), count(count) {}
	EventInfo() {}
};
class Event {
public:
	const std::shared_ptr<EventInfo> info;
	std::function<void(Event* event)> func;
	Event(const std::shared_ptr<EventInfo> info, std::function<void(const Event* event)> func = [](const Event* event) {}) : info(info), func(func) {}
	void operator()() { func(this); }	//bool operator<(const Event& other) {return static_cast<int>(this->info->priority) < static_cast<int>(other.info->priority);}
};
class EventBus {
private:
	struct EventGroup {
	public: std::shared_mutex mtx, mtx1;
		  std::vector<Event> events;
		  std::deque<std::shared_ptr<Event> > waitings;
		  std::atomic<bool> newTaskFlag, exitFlag = false;
		  std::thread thread = std::thread([=](id) {while (!exitFlag.load()) {}});
		  ~EventGroup() { thread.join(); }
	};
	std::map<int, EventGroup > eventGroups;
	void processEvent(std::shared_ptr<Event> fn, bool delay = false, int index = 0) {
		EventInfo eventInfo = *(fn->info);
		switch ((*fn).info->mode)
		{
		case Count:
			if (fn->info->getCount() - 1 >= 0 && !delay) {//delay to another fn
				eventInfo.CountSub();//nobreak
			} [[fallthrough]];
		case Repeat:
			(*fn)();
			break;
		}
	}
public:
	EventBus() {}
	void addEvent(int id, Event e) {
		yuukaLock(eventGroups[id].mtx);
		eventGroups[id].events.emplace_back(e);
	}
	void removeEvent(int id, Event e) {
		std::vector<Event> nil;
		yuukaLock(eventGroups[id].mtx);
		eventGroups[id].events.swap(nil);
	}
	void triggerEvent(int id) {
		std::vector<std::shared_ptr<Event>> eventsToProcess;
		{
			yuukaLock(eventGroups[id].mtx1);
			if (eventGroups.find(id) == eventGroups.end()) return;
			for (auto& e : eventGroups[id].events)
				eventsToProcess.emplace_back(std::make_shared<Event>(e));
		}
		std::async(std::launch::async, [this, eventsToProcess, id]() {
			for (auto& event : eventsToProcess) {
				if (event->info->type == Delay) {
					yuukaLock(eventGroups[id].mtx1);
					if (event->info->priority == High)
						eventGroups[id].waitings.emplace_front(event);
					else
						eventGroups[id].waitings.emplace_back(event);
				}
				else {
					processEvent(event, false);
				}
			}
			std::cout << "event done" << std::endl;
			});
	}
	void delayQueueRun(int id) {
		yuukaLock(eventGroups[id].mtx1);
		while (eventGroups[id].waitings.size() > 0) {
			if (eventGroups[id].waitings.front()->info->getCount() <= 0) {
				eventGroups[id].waitings.pop_front();
				continue;
			}
			processEvent(*(&eventGroups[id].waitings.front()), true);
			eventGroups[id].waitings.pop_front();
		}
	}
};
int main()
{
	EventBus bus;
	EventInfo info(0x1, AtOnce, High, Repeat);
	bus.addEvent(0x1, Event(std::make_shared<EventInfo>(info), ([](const Event* event) mutable {
		std::cout << event->info->getCount();
		})));
	bus.triggerEvent(0x1);
	std::cout << "Hello World!\n";
	return 0;
}