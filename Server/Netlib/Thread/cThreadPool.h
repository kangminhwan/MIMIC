#pragma once
#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <functional>
#include <stdexcept>
#include <memory>
#include <utility>

// C++14 이하에서는 invoke_result 대신 result_of를 사용합니다.

class cThreadPool
{
public:
    // 스레드 numThreads개를 갖는 풀을 생성
    explicit cThreadPool(size_t numThreads = 10);

    // 스레드 풀 소멸자
    ~cThreadPool();

    // 작업(람다, 함수포인터 등)을 풀에 등록(enqueue)하고, 결과를 std::future로 받음
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        // 함수 F(Args...) 의 반환타입
        using return_type = typename std::result_of<F(Args...)>::type;

        // (return_type())를 호출하는 packaged_task 생성
        auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = taskPtr->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            // 이미 풀을 멈춘 상태라면 예외
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            // 작업 큐에 등록
            tasks.emplace([taskPtr]() {
                (*taskPtr)();
                });
        }
        // 새로운 작업이 들어왔다고 하나의 대기 스레드를 깨움
        condition.notify_one();
        return res;
    }

private:
    // 실제 작업을 처리할 스레드들
    std::vector<std::thread> workers;
    // 작업(함수)들을 담아둘 큐
    std::queue<std::function<void()>> tasks;

    // 동기화 도구들
    std::mutex queueMutex;
    std::condition_variable condition;
    // 풀 중지 여부 (true가 되면 더 이상 작업을 받지 않고, 기존 스레드는 종료)
    bool stop;
};
