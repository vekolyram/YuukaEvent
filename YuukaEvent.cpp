// YuukaEvent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <map>
#include <functional>
#include <future>
#include <queue>
#include <atomic>
enum class EventID { Null, OnGroupMsg, OnFriendMsg };
enum class EventQueue { AtOnce, Delay };
enum class EventPriority { Low, Normal, High };
enum class EventMode { Count, Repeat };
class Event {
public:
	const EventID id;
	std::function<void(const Event& event)> func;
	Event(EventID id) : id(id) {}
	void operator()(const Event& event) {
		func(*this);
	}
};
class EventBus {
	std::map<EventID, std::queue<Event>> events;
	std::map<EventID, bool> locks;
	std::atomic<bool> bigLock;
public:
	EventBus() : bigLock(false) {}
	void addEvent(EventID id, std::function<void(const Event& event)> func) {
		events[id].emplace(func);
	}
	void removeEvent(EventID id, std::function<void(const Event& event)> func) {
		std::queue<Event> nil;
		events[id].swap(nil);
	}
	void enqueueEvent(EventID id) {
		std::async(std::launch::async, [=]() {
			while (locks[id]);
			locks[id] = true;
			for (;events[id].empty();events[id].pop()) {
				events[id].front()(Event(id));
			}
			locks[id] = false;
			std::cout << "event done" << std::endl;
			});
	}
};
int main()
{
	//Event e1 = a();
	//e1();
	EventBus bus;
	bus.addEvent(EventID::OnGroupMsg, [](const Event& event) {});
	bus.enqueueEvent(EventID::OnGroupMsg);
	std::cout << "Hello World!\n";
}