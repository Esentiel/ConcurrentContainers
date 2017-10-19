#pragma once

#include <queue>
#include <thread>
#include <condition_variable>
#include <memory>


template<typename T>
class threadsafe_queue_lb
{
private:
	std::mutex mut_;
	std::queue<T> queue_data_;
	std::condition_variable data_cond_;
public:
	threadsafe_queue_lb() {}
	
	void push(T val)
	{
		std::lock_guard<std::mutex> lk(mut_);
		queue_data_.push(val);
		data_cond_.notify_one();
	}

	void wait_and_pop(T &val)
	{
		std::unique_lock<std::mutex> lk(mut_);
		data_cond_.wait(lk, [this] {return !queue_data_.empty(); });
		val = std::move(queue_data_.front());
		queue_data_.pop();
	}

	std::shared_ptr<T> wait_and_pop()
	{
		std::unique_lock<std::mutex> lk(mut_);
		data_cond_.wait(lk, [this] {return !queue_data_.empty()});
		auto res = std::make_shared<T>(std::move(queue_data_.front()));
		queue_data_.pop();

		return res;
	}

	bool try_pop(T &val)
	{
		std::lock_guard<std::mutex> lk(mut_);
		if (queue_data_.empty())
			return false;
		
		val = std::move(queue_data_.front());
		queue_data_.pop();

		return true;
	}

	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mut_);
		if (queue_data_.empty())
			return nullptr;

		auto res = std::make_shared<T>(std::move(queue_data_.front()));
		queue_data_.pop();

		return res;
	}

	bool empty()
	{
		std::lock_guard<std::mutex> lk(mut_);
		return queue_data_.empty();
	}
};