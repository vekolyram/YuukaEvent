#include <iostream>
#include <map>
#include <functional>
#include <future>
#include <queue>
#include <mutex>
#define yuukaLock(x) std::unique_lock<std::mutex> _lock = std::unique_lock<std::mutex>(x)
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
	public: std::mutex mtx; std::vector<Event> events;
	};
	std::map<int, EventGroup > eventGroups;
	std::map<int, std::deque<std::shared_ptr<EventGroup> > > waitingGroups;
	std::mutex eventsMtx, waitingMtx;
	std::unique_lock<std::mutex> waitingLock = std::unique_lock<std::mutex>(waitingMtx);
	void processEvent(std::shared_ptr<Event> fn, bool delay = false, int index = 0) {
		EventInfo eventInfo = *(fn->info);
		switch ((*fn).info->mode)
		{
		case Count:
			if (fn->info->getCount() - 1 >= 0 && !delay) {//delay to another fn
				eventInfo.CountSub();//nobreak
			}
			[[fallthrough]];
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
			yuukaLock(eventsMtx);//
			if (eventGroups.find(id) == eventGroups.end()) return;
			for (auto& e : eventGroups[id].events)
				eventsToProcess.emplace_back(std::make_shared<Event>(e));
		}
		std::async(std::launch::async, [this, eventsToProcess, id]() {
			for (auto& event : eventsToProcess) {
				if (event->info->type == Delay) {
					yuukaLock(waitingGroups[id]->mtx);//waiting group gg
					if (event->info->priority == High)
						waiting[event->info->id].emplace_front(event);
					else
						waiting[event->info->id].emplace_back(event);
				}
				else {
					processEvent(event, false);
				}
			}
			std::cout << "event done" << std::endl;
			});
	}
	void delayQueueRun(int id) {
		yuukaLock(waitingMtx);
		while (waiting[id].size() > 0) {
			if (events.count(waiting[id].front()->info->id) == 0 || waiting[id].front()->info->getCount() <= 0) {
				waiting[id].pop_front();
				continue;
			}
			processEvent(*(&waiting[id].front()), true);
			waiting[id].pop_front();
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