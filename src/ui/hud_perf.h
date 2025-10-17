#pragma once
#include "../services/metrics_detail.h"

struct PerCallHud {
  CallHistogram upd{512};
  CallHistogram ren{512};
  void draw_small();
};
