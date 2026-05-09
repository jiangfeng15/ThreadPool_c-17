#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>


namespace tp {
	enum class WorkerState : std::uint8_t {
		Idle,
		Running,
		Stopped
	};

	enum class PoolState : std::uint8_t {
		Running,
		Paused,
		Stopping,
		Stopped
	};

	namespace detail {
		inline std::size_t & thread_local_worker_index() noexcept {
			thread_local std::size_t idx = 0;
			return idx;
		}

		template <typename R, typename Bound, std::enable_if_t<!std::is_void_v<R>,int> = 0>
		void invoke_and_set_value(const std::shared_ptr<std::promise<R>> & promise, Bound & bound) {
			promise->set_value(bound());
		}

		template <typename Bound>
		void invoke_and_set_value(const std::shared_ptr<std::promise<void>> & promise, Bound & bound){
			bound();
			promise->set_value();
		}
	}

	class ThreadPool {
	public:
		using ExceptionHandler = std::function<void(std::size_t worker_index, std::exception_ptr ep)>;
		explicit ThreadPool(std::size_t thread_count = 0) {
			const std::size_t tc = std::thread::hardware_concurrency();
			std::size_t n = thread_count == 0 ? (tc == 0 ? 4u : static_cast<std::size_t>(tc)) : thread_count;
			if (n == 0)
				n = 2;
			workers_.reserve(n);
			worker_states_.reset(new std::atomic<WorkerState>[n]);
			for (std::size_t i = 0; i < n; i++) {
				worker_states_[i].store(WorkerState::Idle, std::memory_order_relaxed);
				workers_.emplace_back([this,i]() {
					worker_loop(i);
				});
			}
		}
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		~ThreadPool() {
			shutdown(true);
		}
		template<typename F, typename ...Args>
		auto submit(F &&f, Args&&...args)
		->std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>{
			using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>... >;
			if (!accepting_tasks()) {
				throw std::runtime_error("ThreadPool: pool is not accepting new tasks");
			}

			auto promise = std::make_shared<std::promise<ReturnType>>();
			std::future<ReturnType> result = promise->get_future();

			auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
			{
				std::unique_lock<std::mutex> lock(queque_mutex_);
				if (!accepting_tasks()) {
					throw std::runtime_error("ThreadPool: pool stopped before task was queued");
				}
				tasks_.emplace([this, promise, bound = std::move(bound)]() mutable {
					const std::size_t wid = detail::thread_local_worker_index();
					try {
						detail::invoke_and_set_value(promise, bound);
					}
					catch (...) {
						std::exception_ptr ep = std::current_exception();
						invoke_exception_handler(wid, ep);
						promise->set_exception(ep);
					}
				});
			}
			condition_.notify_all();
			return result;
		}
		void pause() {
			pool_state_.store(PoolState::Paused, std::memory_order_release);
			condition_.notify_all();
		}
		void resume() {
			PoolState expected = PoolState::Paused;
			if (pool_state_.compare_exchange_strong(expected, PoolState::Running, std::memory_order_acq_rel)) {
				condition_.notify_all();
			}
		}
		void shutdown(bool wait_pending = true) {
			if (pool_state_.load(std::memory_order_acquire) == PoolState::Stopped)
				return;
			{
				std::unique_lock<std::mutex> lock(queque_mutex_);
				pool_state_.store(PoolState::Stopping, std::memory_order_release);
				discard_remaining_tasks_on_stop_ = !wait_pending;
				if (!wait_pending) {
					clear_pending_tasks_unlocked();
				}
			}
			condition_.notify_all();
			for (std::thread &w : workers_) {
				if (w.joinable()) {
					w.join();
				}
			}
			pool_state_.store(PoolState::Stopped, std::memory_order_release);
		}
		std::size_t thread_count() const noexcept{ return workers_.size(); };

		tp::PoolState state() const noexcept { return pool_state_.load(std::memory_order_acquire); }
		std::vector<WorkerState> worker_states_snapshot() const {
			const std::size_t n = workers_.size();
			std::vector<WorkerState> out(n);
			for (std::size_t i = 0; i < n; i++) {
				out[i] = worker_states_[i].load(std::memory_order_acquire);
			}
			return out;
		}
		void set_exception_handler(ExceptionHandler handler) {
			std::unique_lock<std::mutex> lock(exception_handler_mutex_);
			exception_handler_ = std::move(handler);
		}
		bool accepting_tasks() const noexcept {
			const PoolState s = pool_state_.load(std::memory_order_acquire);
			return s == PoolState::Running || s == PoolState::Paused;
		}
		void clear_pending_tasks_unlocked(){
			while (!tasks_.empty()) {
				tasks_.pop();
			}
		}
		void worker_loop(std::size_t index) {
			for (;;) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(queque_mutex_);
					// Stopping 时必须谓词为 true 才会唤醒；若队列已空，旧逻辑返回 false 会导致线程永远睡在 wait() 上，join() 死锁。
					condition_.wait(lock, [this]() {
						const PoolState ps = pool_state_.load(std::memory_order_acquire);
						if (ps == PoolState::Stopping) {
							return true;
						}
						if (!tasks_.empty() && ps == PoolState::Running)
							return true;
						return false;
					});

					const PoolState ps = pool_state_.load(std::memory_order_acquire);
					if (ps == PoolState::Stopping) {
						if (tasks_.empty()) {
							worker_states_[index].store(WorkerState::Stopped, std::memory_order_release);
							return;
						}
						if (discard_remaining_tasks_on_stop_) {
							worker_states_[index].store(WorkerState::Stopped, std::memory_order_release);
							return;
						}
					}
					if (tasks_.empty()) {
						continue;
					}
					task = std::move(tasks_.front());
					tasks_.pop();
				}
				detail::thread_local_worker_index() = index;
				worker_states_[index].store(WorkerState::Running, std::memory_order_release);
				try {
					task();
				}
				catch (...) {
					invoke_exception_handler(index, std::current_exception());
				}
				worker_states_[index].store(WorkerState::Idle, std::memory_order_release);
			}
		}
		void invoke_exception_handler(std::size_t index, std::exception_ptr ep) {
			ExceptionHandler h;
			{
				std::unique_lock<std::mutex> lock(exception_handler_mutex_);
				h = exception_handler_;
			}
			if (h) {
				try {
					h(index, ep);
				}
				catch (...) {
					// Swallow exceptions from user handler
				}
			}
		}
	private:
		std::vector<std::thread> workers_;

		std::unique_ptr<std::atomic<WorkerState>[]> worker_states_;

		std::queue<std::function<void()>> tasks_;
		std::mutex queque_mutex_;
		std::condition_variable condition_;

		std::atomic<PoolState> pool_state_{ PoolState::Running };
		bool discard_remaining_tasks_on_stop_{ false };
		std::mutex exception_handler_mutex_;
		ExceptionHandler exception_handler_;
	};
}