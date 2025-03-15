// YuukaEvent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <map>
#include <vector>
#include <functional>
#include <future>
#include <queue>
#include <atomic>
typedef int Event;
enum class EventID { Null, OnGroupMsg, OnFriendMsg };
class EventBus {
	std::map<EventID, std::queue<std::function<void(const Event&& event)>>> events;
	std::atomic<bool> lock;
public:
	EventBus() : events() {}
	void addEvent(EventID id, std::function<void(const Event&& event)> func) {
		events[id].emplace(func);
	}
	void removeEvent(EventID id, std::function<void(const Event&& event)> func) {
		std::queue<std::function<void(const Event&& event)>> nil;
		events[id].swap(nil);
	}
	void enqueueEvent(EventID id) {
		std::async(std::launch::async, [=]() {
			for (;events[id].empty();events[id].pop()) {
				events[id].front();
			}
			std::cout << "event done" << std::endl;
			});
	}
};
//auto a() {
//	return Event([](const Event&& event) {
//		std::cout << static_cast<int>(event.id) << std::endl;
//		}, EventID::OnGroupMsg);
//}
int main()
{
	//Event e1 = a();
	//e1();
	Event
		std::cout << "Hello World!\n";
}