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
	const EventInfo* info;
	std::function<void(Event* event)> func;
	Event(const EventInfo* info, std::function<void(const Event* event)> func = [](const Event* event) {}) : info(info), func(func) {}
	void operator()() { func(this); }
	bool operator<(const Event& other) {
		return static_cast<int>(this->info->priority) < static_cast<int>(other.info->priority);
	}
};
class EventBus {
private:
	std::map<int, std::vector<Event> > events;
	std::map<int, std::deque<Event*> > waiting;
	std::mutex eventsMtx, waitingMtx;
	std::unique_lock<std::mutex> waitingLock = std::unique_lock<std::mutex>(waitingMtx);
	void doOne(Event* _, bool delay = false, int index = 0) {
		EventInfo ccb = *(_->info);
		ccb.CountSub();//nobreak
		switch ((*_).info->mode)
		{
		case Count:
			if (_->info->getCount() <= 0 && !delay)
				if (!delay) {
					yuukaLock(eventsMtx);
					std::map<int, std::vector<Event>> eventMap = events;
					eventMap[_->info->id].erase(eventMap[_->info->id].begin() + index);
					events = (eventMap);
				}
			[[fallthrough]];
		case Repeat:
			(*_)();
			break;
		}
	}
public:
	EventBus() {}
	void addEvent(int id, Event e) {
		events[id].emplace_back(e);
	}
	void removeEvent(int id, Event e) {
		std::vector<Event> nil;
		events[id].swap(nil);
	}
	void triggerEvent(int id) {
		auto _ = events[id][0];
		for (int index = 0; index < events[id].size();_ = events[id][++index]) {
			if (_.info->type == Delay) {
				if (_.info->priority == High)
					waiting[id].push_front(&_);
				else
					waiting[id].push_back(&_);
				break;
			}
			std::async(std::launch::async, [=]() {
				Event ptr = _;
				doOne(&ptr, false, index);});
		}
		std::cout << "event done" << std::endl;
	}
	void delayQueueRun(int id) {
		yuukaLock(waitingMtx);
		while (waiting[id].size() == 0) {
			if (events.count(waiting[id].front()->info->id) > 0) {
				waiting[id].pop_front();
				continue;
			}
			doOne(*(&waiting[id].front()), true);
			waiting[id].pop_front();
		}
	}
};
int main()
{
	EventBus bus;
	EventInfo info(0x1, AtOnce, High, Repeat);
	bus.addEvent(0x1, Event(&info, ([](const Event* event) {
		std::cout << event->info->getCount();
		})));
	bus.triggerEvent(0x1);
	std::cout << "Hello World!\n";
	return 0;
}