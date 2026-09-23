//#pragma once
//#include "../Common/Netlib.h"
//
//#include <vector>
//#include <queue>
//#include <mutex>
//#include <condition_variable>
//#include <thread>
//#include <future>
//#include <functional>
//#include <stdexcept>
//#include <memory>
//#include <utility>
//
//class cThreadPool
//{
//public:
//    // 스레드 numThreads개를 갖는 풀을 생성
//    explicit cThreadPool(size_t numThreads = 10)
//        : stop(false)
//    {
//        // 워커 스레드를 numThreads개 생성
//        for (size_t i = 0; i < numThreads; ++i)
//        {
//            workers.emplace_back([this] {
//                while (true)
//                {
//                    std::function<void()> task;
//                    {
//                        // 큐에서 작업이 들어오거나, stop 상태가 될 때까지 대기
//                        std::unique_lock<std::mutex> lock(this->queueMutex);
//                        this->condition.wait(lock, [this] {
//                            return this->stop || !this->tasks.empty();
//                            });
//
//                        // stop이며 더 이상 할 일이 없다면 종료
//                        if (this->stop && this->tasks.empty())
//                            return;
//
//                        // 작업 하나 꺼내서 실행
//                        task = std::move(this->tasks.front());
//                        this->tasks.pop();
//                    }
//                    // 잠금 해제 후 실제 작업 실행
//                    task();
//                }
//                });
//        }
//    }
//
//    // 스레드 풀 소멸자
//    ~cThreadPool()
//    {
//        {
//            // 더 이상 작업을 받지 않도록 설정
//            std::unique_lock<std::mutex> lock(queueMutex);
//            stop = true;
//        }
//        // 모든 대기 중인 스레드를 깨워 종료하도록 함
//        condition.notify_all();
//
//        // 스레드 join
//        for (auto& th : workers)
//        {
//            if (th.joinable())
//                th.join();
//        }
//    }
//
//    // -----------------------------------------------------
//    //  이 부분이 핵심: 람다(또는 함수) + 추가 인자들을 받아 큐에 등록
//    // -----------------------------------------------------
//    template <class F, class... Args>
//    auto enqueue(F&& f, Args&&... args)
//        -> std::future<typename std::invoke_result_t<F, Args...>::type>
//    {
//        using return_type = typename std::invoke_result_t<F, Args...>::type;
//
//        // (return_type())를 호출하는 packaged_task 생성
//        // std::bind로 f와 args를 모두 묶어 "인자 없는 콜러블"로 만듦
//        auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(
//            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
//        );
//
//        // future로 결과 받기
//        std::future<return_type> res = taskPtr->get_future();
//        {
//            std::unique_lock<std::mutex> lock(queueMutex);
//
//            if (stop)
//                throw std::runtime_error("enqueue on stopped ThreadPool");
//
//            // 작업 큐에 등록: 나중에 워커 스레드가 이 람다를 꺼내 호출
//            tasks.emplace([taskPtr] {
//                (*taskPtr)();  // 실제 실행
//                });
//        }
//        // 새로운 작업이 들어왔다고 조건 변수에 신호
//        condition.notify_one();
//        return res;
//    }
//
//private:
//    // 실제 작업을 처리할 스레드들
//    std::vector<std::thread> workers;
//    // 작업(함수)들을 담아둘 큐
//    std::queue<std::function<void()>> tasks;
//
//    // 동기화 도구들
//    std::mutex queueMutex;
//    std::condition_variable condition;
//
//    // 풀 중지 여부 (true가 되면 더 이상 작업을 받지 않고, 기존 스레드는 종료)
//    bool stop;
//};
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace cThreadPooler {
    class cThreadPool {
    public:
        cThreadPool(size_t num_threads = 5);
        ~cThreadPool();

        // job 을 추가한다.
        void EnqueueJob(std::function<void()> job);

    private:
        // 총 Worker 쓰레드의 개수.
        size_t num_threads_;
        // Worker 쓰레드를 보관하는 벡터.
        std::vector<std::thread> worker_threads_;
        // 할일들을 보관하는 job 큐.
        std::queue<std::function<void()>> jobs_;
        // 위의 job 큐를 위한 cv 와 m.
        std::condition_variable cv_job_q_;
        std::mutex m_job_q_;

        // 모든 쓰레드 종료
        bool stop_all;

        // Worker 쓰레드
        void WorkerThread();
    };

    //cThreadPool::cThreadPool(size_t num_threads)
    //    : num_threads_(num_threads), stop_all(false) {
    //    worker_threads_.reserve(num_threads_);
    //    for (size_t i = 0; i < num_threads_; ++i) {
    //        worker_threads_.emplace_back([this]() { this->WorkerThread(); });
    //    }
    //}

    //void cThreadPool::WorkerThread() {
    //    while (true) {
    //        std::unique_lock<std::mutex> lock(m_job_q_);
    //        cv_job_q_.wait(lock, [this]() { return !this->jobs_.empty() || stop_all; });
    //        if (stop_all && this->jobs_.empty()) {
    //            return;
    //        }

    //        // 맨 앞의 job 을 뺀다.
    //        std::function<void()> job = std::move(jobs_.front());
    //        jobs_.pop();
    //        lock.unlock();

    //        // 해당 job 을 수행한다 :)
    //        job();
    //    }
    //}

    //cThreadPool::~cThreadPool() {
    //    stop_all = true;
    //    cv_job_q_.notify_all();

    //    for (auto& t : worker_threads_) {
    //        t.join();
    //    }
    //}

    //void cThreadPool::EnqueueJob(std::function<void()> job) {
    //    if (stop_all) {
    //        throw std::runtime_error("cThreadPool 사용 중지됨");
    //    }
    //    {
    //        std::lock_guard<std::mutex> lock(m_job_q_);
    //        jobs_.push(std::move(job));
    //    }
    //    cv_job_q_.notify_one();
    //}
}