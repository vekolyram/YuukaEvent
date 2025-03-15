// YuukaEvent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <map>
#include <vector>
#include <functional>
#include <future>
#include <queue>
#include <atomic>
enum class EventID { Null, OnGroupMsg, OnFriendMsg };
//class Event
//{
//public:
//	const EventID id = EventID::Null;
//	std::queue<std::function<void(const Event& event)>> funcs;
//	Event(std::function<void(const Event& event)> func, EventID id) : id(id) {
//		funcs.emplace(func);
//	}
//	void operator()() {
//
//	}
//	~Event() {
//	}
//};
class EventBus {
	std::map<EventID, std::queue<std::function<void(const Event& event)>>> events;
	std::atomic<bool> lock;
public:
	EventBus() : events() {}
	void addEvent(EventID id, std::function<void(const Event& event)> func) {
		events[id].emplace(func);
	}
	void removeEvent(EventID id, std::function<void(const Event& event)> func) {
		std::queue<std::function<void(const Event& event)>> nil;
		events[id].swap(nil);
	}
	void
};
auto a() {
	return Event([](const Event& event) {
		std::cout << static_cast<int>(event.id) << std::endl;
		}, EventID::OnGroupMsg);
}
int main()
{
	Event e1 = a();
	e1();
	std::cout << "Hello World!\n";
}