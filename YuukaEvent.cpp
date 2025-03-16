#include <iostream>
#include <map>
#include <functional>
#include <future>
#include <queue>
#include <mutex>
#include <shared_mutex>
#define yuukaLock(x) std::unique_lock<std::shared_mutex> _lock = std::unique_lock<std::shared_mutex>(x)
enum EventRunType { Immediate, Delay };
struct EventInfo {
private:
	int count = 10;
public:
	const int id = -1;
	const EventRunType type = Immediate;
	void countSub() { count--; }
	int getCount() const { return count; }
	EventInfo(int id, EventRunType type, int count = 1) :id(id), type(type), count(count) {}
	EventInfo() {}
};
class EventRedirector {
public:
	const virtual void doDelayTasksDeque(Event* event, int delayID, int immeID);
	std::deque < std::shared_ptr<Event> > delayDeque;
	const virtual void doTasksDeque(Event* event, int index);
	std::deque<Event> immediateDeque;
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
		  EventRedirector& redirector;
		  std::deque<Event>& tasksDeque;
		  std::deque<std::shared_ptr<Event> >& delayTasksDeque = redirector.delayDeque;
		  std::atomic<bool> newTaskFlag, exitFlag, delay = false;
		  std::thread thread = std::thread([&](auto& tasks) {
			  while (!exitFlag.load()) {
				  newTaskFlag.wait(false);
				  if (exitFlag.load()) break;
				  for (int i = 0; i < tasks.size(); i++) {
					  auto& e = tasks[i];
					  if (e.info->mode == Once || e.info->getCount() <= 0)
						  tasks.erase(tasks.begin() + i--);
					  if (e.info->mode == Count)
						  e.info->countSub();
					  if (e.info->type == Delay)
						  delayTasksDeque.emplace_front(std::make_shared<Event>(e));
					  if (e.info->type == Immediate) {
						  e();
						  break;
					  }
				  }
			  }
			  newTaskFlag.store(false);
			  }, tasksDeque);
		  void operator()() {
			  newTaskFlag.store(true);
			  newTaskFlag.notify_all();
		  }
		  ~EventGroup() {
			  exitFlag.store(false);
			  thread.join();
		  }
	};
	std::map<int, EventGroup > eventGroups;
	EventRedirector redirector;
public:
	EventBus() {}
	int addEvent(int id, Event e) {
		yuukaLock(eventGroups[id].mtx);
		eventGroups[id].tasksDeque.emplace_front(e);
		return eventGroups[id].tasksDeque.size() - 1;
	}
	void removeAllEvent(int id) {
		std::deque<Event> nil;
		yuukaLock(eventGroups[id].mtx);
		eventGroups[id].tasksDeque.swap(nil);
	}
	void removeEvent(int id, int index) {
		yuukaLock(eventGroups[id].mtx);
		eventGroups[id].tasksDeque.erase(eventGroups[id].tasksDeque.begin() + index);
	}
	void triggerEvent(int id) {
		yuukaLock(eventGroups[id].mtx1);
		if (eventGroups.find(id) == eventGroups.end()) return;
		eventGroups[id]();
	}
	void delayQueueRun(int id) {
		yuukaLock(eventGroups[id].mtx1);
		while (eventGroups[id].delayTasksDeque.size() > 0) {
			if (eventGroups[id].delayTasksDeque.front()->info->getCount() <= 0) {
				eventGroups[id].delayTasksDeque.pop_front();
				continue;
			}
			eventGroups[id].doDelayTasksDeque();
		}
	}
};
int main()
{
	EventBus bus;
	EventInfo info(0x1, Immediate, Repeat);
	bus.addEvent(0x1, Event(std::make_shared<EventInfo>(info), ([](const Event* event) mutable {
		std::cout << event->info->getCount();
		})));
	bus.triggerEvent(0x1);
	std::cout << "Hello World!\n";
	return 0;
}