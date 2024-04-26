#pragma once

class Timer {
public:
  static Timer &getInstance() {
    static Timer instance;
    return instance;
  }

private:
  Timer();
  Timer(Timer &&) = delete;
  Timer(const Timer &) = delete;
};
