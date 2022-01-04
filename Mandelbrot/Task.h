#pragma once
#include <optional>
#include <future>
#include <utility>
#include <type_traits>

template <class Result>
struct Task {

	template <class Fn, class... Args> requires std::is_same_v<Result, std::invoke_result_t<Fn, Args...>>
	Task(Fn&& fn, Args&&... args) :
		f(std::async(std::launch::async, std::forward<Fn>(fn), std::forward<Args>(args)...)),
		complete(false)
	{
	}

	// Returns true if the task completed and stores the result in res. Otherwise returns false
	bool PollCompletion(Result& res) {
		if (complete) {
			return false;
		} else if (f.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			res = std::move(f.get());
			complete = true;
			return true;
		} else {
			return false;
		}
	}

	// Blocks until task is complete and returns the result
	Result GetResult() {
		return f.get();
	}

private:
	std::future<Result> f;
	bool complete;
};
