#pragma once

#include <memory>
#include <mutex>

template<typename T>
class threadsafe_queue_lb_impr
{
private:
	struct node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
	};

	std::mutex mutex_head_;
	std::unique_ptr<node> head_;
	std::mutex mutex_tail_;
	node * tail_;
	std::condition_variable data_cond_;

	node * get_tail()
	{
		std::lock_guard<std::mutex> lk(mutex_tail_);
		return tail_;
	}

	std::unique_ptr<node> pop_head()
	{
		std::unique_ptr<node> old_head = std::move(head_);
		head_ = std::move(old_head->next);
		return old_head;
	}

	std::unique_lock<std::mutex> wait_for_data()
	{
		std::unique_lock<std::mutex> head_lock(mutex_head_);
		data_cond_.wait(head_lock, [&] {return head_.get() != get_tail(); });
		return std::move(head_lock);
	}

	std::unique_ptr<node> wait_pop_head()
	{
		std::unique_lock<std::mutex> head_lock(wait_for_data());
		return pop_head();
	}

	std::unique_ptr<node> wait_pop_head(T& value)
	{
		std::unique_lock<std::mutex> head_lock(wait_for_data());
		value = std::move(*head_->data);
		return pop_head();
	}

	std::unique_ptr<node> try_pop_head()
	{
		std::lock_guard<std::mutex> head_lock(mutex_head_);
		if (head_.get() == get_tail())
		{
			return std::unique_ptr<node>();
		}
		return pop_head();
	}
	std::unique_ptr<node> try_pop_head(T& val)
	{
		std::lock_guard<std::mutex> head_lock(mutex_head_);
		if (head_.get() == get_tail())
		{
			return std::unique_ptr<node>();
		}
		val = std::move(*head_->data);
		return pop_head();
	}
public:
	threadsafe_queue_lb_impr() : head_(std::make_unique<node>(node())), tail_(head_.get()) 
		{}

	threadsafe_queue_lb_impr(const threadsafe_queue_lb_impr&) = delete;
	threadsafe_queue_lb_impr& operator= (const threadsafe_queue_lb_impr&) = delete;

	std::shared_ptr<T> try_pop();
	bool try_pop(T &val);

	std::shared_ptr<T> wait_and_pop();
	void wait_and_pop(T &val);

	void push(T val);
	bool empty();
};


template<typename T>
void threadsafe_queue_lb_impr<T>::push(T val)
{
	std::shared_ptr<T> data(std::make_shared<T>(std::move(val)));
	std::unique_ptr<node> p(std::make_unique<node>(node()));
	{
		std::lock_guard<std::mutex> tail_lock(mutex_tail_);
		tail_->data = data;
		node * const new_tail = p.get();
		tail_->next = std::move(p);
		tail_ = new_tail;
	}
	data_cond_.notify_one();
}


template<typename T>
std::shared_ptr<T> threadsafe_queue_lb_impr<T>::wait_and_pop()
{
	std::unique_ptr<node> const old_node = wait_pop_head();
	return old_node->data;
}


template<typename T>
void threadsafe_queue_lb_impr<T>::wait_and_pop(T &val)
{
	std::unique_ptr<node> const old_node = wait_pop_head(val);
}


template<typename T>
std::shared_ptr<T> threadsafe_queue_lb_impr<T>::try_pop()
{
	std::unique_ptr<node> old_head = try_pop_head();
	return old_head ? old_head->data : std::shared_ptr<T>();
}


template<typename T>
bool threadsafe_queue_lb_impr<T>::try_pop(T& val)
{
	std::unique_ptr<node> const old_head = try_pop_head(val);
	return old_head != nullptr;
}


template<typename T>
bool threadsafe_queue_lb_impr<T>::empty()
{
	std::lock_guard<std::mutex> head_lock(mutex_head_);
	return (head_.get() == get_tail());
}