#pragma once
#include "../services/log_bus.h"

class LogPanel {
public:
  void draw(LogBus& bus);
private:
  int filter_ = 0;
};
