#pragma once
#include <chrono>

using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;

template<typename Duration = std::chrono::milliseconds>
class Timer {
public:
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  static TimePoint Now() { return Clock::now(); }

  void Start();

  void Pause();

  Duration TimeElapsed() const;

  bool IsRunning() const;

  void Clear();

  void Reset();

private:
  bool isRunning_ = false;
  TimePoint start_;
  Duration timeElapsed_ = Duration::zero();
};

template<typename Duration>
void Timer<Duration>::Start() {
  if (isRunning_)
    return;

  isRunning_ = true;
  start_ = Now();
}

template<typename Duration>
void Timer<Duration>::Pause() {
  if (!isRunning_)
    return;

  isRunning_ = false;
  timeElapsed_ += std::chrono::duration_cast<Duration>(Now() - start_);
}

template<typename Duration>
Duration Timer<Duration>::TimeElapsed() const {
  return timeElapsed_ +
         (isRunning_ ? std::chrono::duration_cast<Duration>(Now() - start_) : Duration::zero());
}

template<typename Duration>
bool Timer<Duration>::IsRunning() const {
  return isRunning_;
}

template<typename Duration>
void Timer<Duration>::Clear() {
  timeElapsed_ = Duration::zero();
}

template<typename Duration>
void Timer<Duration>::Reset() {
  isRunning_ = false;
  timeElapsed_ = Duration::zero();
}

template<typename Rep, typename Period>
double ToSeconds(std::chrono::duration<Rep, Period> duration) {
  return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(duration).count();
}