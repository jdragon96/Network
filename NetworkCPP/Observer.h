#pragma once

#include <vector>
#include <functional>

template<typename T>
class Observer
{
public:
	Observer()
	{

	}

	void Subscribe(std::function<void(T)> cb)
	{
		callback.push_back(cb);
	}

	void Broadcast(T item)
	{
		for (auto& cb : callback)
		{
			cb(item);
		}
	}

private:
	std::vector<std::function<void(T)>> callback;
};