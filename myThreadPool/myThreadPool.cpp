// myThreadPool.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "myThreadPool.h"
#include <iostream>


template<typename F, typename ...Args>
auto package(F &&f, Args...args) {
	using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>... >;
	auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	if constexpr (std::is_void_v<ReturnType>)
	{
		func();
	}
	else
	{
		ReturnType res = func();

		return res;
	}
}



int main()
{
	int rd = package([](int x, int y) {return x + y; }, 1,2);
	std::promise<int> a_promise;
	std::future<int> b_future = a_promise.get_future();
	a_promise.set_value(1);
	int a = b_future.get();


	tp::ThreadPool pool(4);
	pool.set_exception_handler([](std::size_t idx, std::exception_ptr ep) {
		try {
			if (ep) {
				std::rethrow_exception(ep);
			}
		}
		catch (const std::exception &ex) {
			std::cerr << "[handler] worker " << idx << " exception: " << ex.what() << '\n';
		}
		catch (...) {
			std::cerr << "[handler] worker " << idx << " unknown exception\n";
		}

	});
	auto f1 = pool.submit([] { return 40 + 2; });
	auto f2 = pool.submit([](int x) { return x * 2; }, 21);
	auto fvoid = pool.submit([] {});
	fvoid.wait();

	// Intentional exception: also reported to set_exception_handler and future::get
	auto f3 = pool.submit([]() -> int {
		throw std::runtime_error("task failure");
	});

	std::cout << "sum: " << f1.get() + f2.get() << '\n';

	try {
		f3.get();
	}
	catch (const std::exception& ex) {
		std::cout << "caught from future: " << ex.what() << '\n';
	}
	auto f4 = pool.submit([]() {
		std::string res{ "test string" };
		return res; });
	std::string res = f4.get();
	pool.pause();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	pool.resume();

	pool.shutdown(true);
    std::cout << "Hello World!\n"; 
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
