#include "calibration.h"

#include <cstring>
#include <string>

#include "daisy_patch.h"
#include "daisysp.h"
#include "settings.h"

using namespace daisy;
using namespace daisysp;

class Calibrator {
 public:
  Calibrator(DaisyPatch *_patch) : patch(_patch) {}
  void AudioCallback(float **in, float **out, size_t size);
  void UpdateControls();
  void UpdateOled();
  void Run();

  DaisyPatch *patch;
  float       ctrl_values[4];
  int         cur_cv      = 0;
  int         cur_voltage = 0;
  bool        exit        = false;
  Oscillator  osc;
};

Calibrator *cal;

void RunCalibration(daisy::DaisyPatch *patch) {
  // The amount of time each loop should take
  uint32_t update_span_ms =
      static_cast<uint32_t>((1.0f / patch->AudioSampleRate()) * 1000);

  // Wait for things to settle
  for (int settle = 0; settle < 100; settle++) {
    uint32_t start = dsy_system_getnow();

    patch->DebounceControls();
    patch->UpdateAnalogControls();

    uint32_t end       = dsy_system_getnow();
    uint32_t span_used = end - start;
    if (span_used <= update_span_ms) {
      dsy_system_delay(update_span_ms - span_used);
    }
  }

  if (!patch->encoder.Pressed()) {
    return;
  }

  cal = new Calibrator(patch);
  cal->Run();
  delete cal;
}

static void CalibrationAudioCallback(float **in, float **out, size_t size) {
  cal->AudioCallback(in, out, size);
}

static void SilenceAudioCallback(float **in, float **out, size_t size) {
  for (int i = 0; i < 4; i++) {
    memset(out[i], 0, sizeof(float) * size);
  }
}

void Calibrator::Run() {
  // initialize calibration
  for (int i = 0; i < 4; i++) {
    ctrl_values[0] = 0.5;
    for (int v = 0; v <= 5; v++) {
      gSettings.cals[i].map[v] = v * 0.2;
    }
  }

  osc.Init(patch->AudioSampleRate());
  osc.SetAmp(0.75);
  osc.SetFreq(131);  // Approx C3
  osc.SetWaveform(Oscillator::WAVE_SIN);

  patch->StartAudio(CalibrationAudioCallback);

  while (!exit) {
    UpdateOled();
  }
  // Write the calibration settings out
  SaveSettings(&patch->seed);

  // Stop the audio so that the main loop can start it up again
  // Note: Not currently supported.
  // patch->StopAudio();
  patch->ChangeAudioCallback(SilenceAudioCallback);
}

void Calibrator::AudioCallback(float **in, float **out, size_t size) {
  UpdateControls();
  if (patch->encoder.RisingEdge()) {
    exit = true;
  }

  for (int i=0; i < size; i++) {
    auto val = osc.Process();
    for (int chn = 0; chn < 4; chn++) {
      out[chn][i] = val;
    }
  }
}

void Calibrator::UpdateControls() {
  patch->DebounceControls();
  patch->UpdateAnalogControls();
  if (patch->encoder.Increment() > 0) {
    cur_voltage++;
    if (cur_voltage > 5) {
      cur_voltage = 0;
      cur_cv++;
    }
    if (cur_cv > 3) {
      cur_cv = 0;
    }
  }

  if (patch->encoder.Increment() < 0) {
    cur_voltage--;
    if (cur_voltage < 0) {
      cur_voltage = 5;
      cur_cv--;
    }
    if (cur_cv < 0) {
      cur_cv = 3;
    }
  }
  // Read all of the values
  for (int i = 0; i < 4; i++) {
    ctrl_values[i] = patch->GetCtrlValue((DaisyPatch::Ctrl)i);
  }

  // Grab/set the value for the current voltage/cv being calibrated
  gSettings.cals[cur_cv].map[cur_voltage] = ctrl_values[cur_cv];
}
void Calibrator::UpdateOled() {
  char line[100];
  patch->display.Fill(false);
  patch->display.SetCursor(0, 0);
  snprintf(line, 100, "cv%d", cur_cv + 1);
  patch->display.WriteString(line, Font_6x8, true);

  for (int v = 0; v <= 5; v++) {
    patch->display.SetCursor(24, v * 10);
    snprintf(line, 100, "%c %dV %0.3f", v == cur_voltage ? '*' : ' ', v,
             gSettings.cals[cur_cv].map[v]);
    patch->display.WriteString(line, Font_6x8, true);
  }

  patch->display.Update();
}

static float lerp(float in, float in_min, float in_max, float out_min,
                  float out_max) {
  return ((in - in_min) / (in_max - in_min)) * (out_max - out_min) + out_min;
}

// Read the CV input while applying calibration map
float ReadCalibratedCV(daisy::DaisyPatch *patch, int cv) {
  float in  = patch->GetCtrlValue((DaisyPatch::Ctrl)cv);
  auto  map = gSettings.cals[cv].map;

  // Below our lowest calibration point, return 0V
  if (in <= map[0]) {
    return 0.0f;
  }
  for (int i = 0; i < 5; i++) {
    if (in < map[i + 1]) {
      return lerp(in, map[i], map[i + 1], i, i + 1);
    }
  }

  // Maxed out, return 5V
  return 5;
}