#pragma once

#include <cstdint>


struct ControlCalibration {
  float min, max;
};

struct Settings {
  uint32_t signature;
  uint32_t version;
  ControlCalibration cals[4];
};

extern Settings gSettings;

namespace daisy{ class DaisySeed; }
void SaveSettings(daisy::DaisySeed* seed);
void LoadSettings(daisy::DaisySeed* seed);