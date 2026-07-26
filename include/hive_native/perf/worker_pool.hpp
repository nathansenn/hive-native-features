#pragma once
/**
 * #152 #153 #155 — portable priority worker pool sketch (high/med/low + apply).
 * Not a full hived replacement; validates scheduling math and metrics hooks.
 */
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace hive_native {
namespace perf {

enum class task_priority : uint8_t { apply = 0, high = 1, medium = 2, low = 3 };

class worker_pool {
public:
   explicit worker_pool(size_t n = 0) {
      if(n == 0) n = std::max<size_t>(1, std::thread::hardware_concurrency());
      n_ = n;
      for(size_t i = 0; i < n_; ++i)
         workers_.emplace_back([this]{ loop(); });
   }

   ~worker_pool() {
      {
         std::lock_guard<std::mutex> g(mu_);
         stop_ = true;
      }
      cv_.notify_all();
      for(auto& t : workers_) if(t.joinable()) t.join();
   }

   void submit(task_priority p, std::function<void()> fn) {
      {
         std::lock_guard<std::mutex> g(mu_);
         queues_[size_t(p)].push(std::move(fn));
         ++submitted_;
      }
      cv_.notify_one();
   }

   void wait_idle() {
      for(;;) {
         std::unique_lock<std::mutex> lk(mu_);
         if(inflight_ == 0 && all_empty()) return;
         idle_cv_.wait_for(lk, std::chrono::milliseconds(1));
      }
   }

   uint64_t submitted() const { return submitted_.load(); }
   uint64_t completed() const { return completed_.load(); }
   size_t threads() const { return n_; }

private:
   bool all_empty() const {
      for(auto& q : queues_) if(!q.empty()) return false;
      return true;
   }

   bool pop(std::function<void()>& fn) {
      for(size_t i = 0; i < 4; ++i) {
         if(!queues_[i].empty()) {
            fn = std::move(queues_[i].front());
            queues_[i].pop();
            return true;
         }
      }
      return false;
   }

   void loop() {
      for(;;) {
         std::function<void()> fn;
         {
            std::unique_lock<std::mutex> lk(mu_);
            cv_.wait(lk, [&]{ return stop_ || !all_empty(); });
            if(stop_ && all_empty()) return;
            if(!pop(fn)) continue;
            ++inflight_;
         }
         fn();
         {
            std::lock_guard<std::mutex> g(mu_);
            --inflight_;
            ++completed_;
         }
         idle_cv_.notify_all();
      }
   }

   size_t n_;
   std::vector<std::thread> workers_;
   std::mutex mu_;
   std::condition_variable cv_, idle_cv_;
   std::queue<std::function<void()>> queues_[4];
   bool stop_ = false;
   std::atomic<uint64_t> submitted_{0}, completed_{0};
   size_t inflight_ = 0;
};

} // namespace perf
} // namespace hive_native
