#include <cstdarg>
#include <cstdio>
#include <string>

#include "calibration.h"
#include "daisy_patch.h"
#include "daisysp.h"
#include "notes.h"
#include "settings.h"
#include "supersaw.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch gPatch;

class ChordApp {
 public:
  void        Init(DaisyPatch *_patch);
  static void StaticAudioCallback(float **in, float **out, size_t size);
  void        AudioCallback(float **in, float **out, size_t size);
  void        UpdateControls();
  void        UpdateOled();
  void        Run();

  float ReadControlInV(int i) { return ReadCalibratedCV(patch, i); }

  const int   base_note      = 24;  // C1
  const float voltage_offset = semitone_to_e1vo(base_note);

  DaisyPatch *patch;

  int   fund_st;  // Fundamental frequency in semitones
  float detune;
  float mix;

  enum ActiveParam {
    PARAM_FIRST,
    PARAM_FUND = PARAM_FIRST,
    PARAM_MIX,
    PARAM_LAST = PARAM_MIX
  };
  ActiveParam active_param_;

  float    osc_st[3];  // Semitone freq for derived oscillators
  SuperSaw osc[3];

  // OLED helpers.
  // TODO: break these out from this class
  void Printf(int line, bool inverted, const char *fmt, ...);
};

ChordApp app;

void ChordApp::Init(DaisyPatch *_patch) {
  patch = _patch;

  detune  = 1.0;
  mix     = 1.0;
  fund_st = 48;  // C3

  active_param_ = PARAM_MIX;

  auto samplerate = patch->AudioSampleRate();
  for (int i = 0; i < 3; i++) {
    osc[i].Init(samplerate);
    osc[i].SetAmp(.7);
  }
}

void ChordApp::Run() {
  patch->StartAudio(StaticAudioCallback);

  while (1) {
    UpdateOled();
  }
}
void ChordApp::StaticAudioCallback(float **in, float **out, size_t size) {
  app.AudioCallback(in, out, size);
}

void ChordApp::AudioCallback(float **in, float **out, size_t size) {
  UpdateControls();

  for (size_t i = 0; i < size; i++) {
    float mix = 0;
    // Process and output the three oscillators
    for (size_t chn = 0; chn < 3; chn++) {
      float sig = osc[chn].Process();
      mix += sig * .33f;
      out[chn][i] = sig;
    }

    // output the mixed oscillators
    out[3][i] = mix;
  }
}

void ChordApp::Printf(int line, bool inverted, const char *fmt, ...) {
  // Display is 64x128. We can fit 6 lines of 10px each using 6x8 font. Center
  // all 6 lines by adding 2 to the top. Center characters each line by adding
  // another.
  int y_start = 3 + line * 10;
  if (inverted) {
    patch->display.DrawRect(0, y_start - 1, 128, y_start + 9, true, true);
  }

  char    line_buf[25];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line_buf, 25, fmt, args);
  va_end(args);

  patch->display.SetCursor(0, y_start);
  patch->display.WriteString(line_buf, Font_6x8, !inverted);
}

void ChordApp::UpdateOled() {
  patch->display.Fill(false);

  Printf(0, false, "0xBEDA SuperSaw");
  Printf(1, active_param_ == PARAM_FUND, "Base: %0.0f Hz %s",
         semitone_to_hz(fund_st), semitone_name(fund_st).c_str());
  Printf(2, false, "Detune: %0.2f (%0.2f)", detune, osc[0].DetuneScale());
  Printf(3, active_param_ == PARAM_MIX, "Mix: %0.2f", mix);
  Printf(5, false, "%4s %4s %4s", semitone_name(osc_st[0]).c_str(),
         semitone_name(osc_st[1]).c_str(), semitone_name(osc_st[2]).c_str());

  patch->display.Update();
}

void ChordApp::UpdateControls() {
  patch->DebounceControls();
  patch->UpdateAnalogControls();

  if (patch->encoder.Pressed()) {
    active_param_ =
        static_cast<ActiveParam>(active_param_ + patch->encoder.Increment());
    if (active_param_ < 0) active_param_ = PARAM_LAST;
    if (active_param_ > PARAM_LAST) active_param_ = PARAM_FIRST;
  } else {
    switch (active_param_) {
      case PARAM_FUND:
        fund_st += patch->encoder.Increment();
        if (fund_st < st_min) fund_st = st_min;
        if (fund_st > st_max) fund_st = st_max;
        break;
      case PARAM_MIX:
        mix += patch->encoder.Increment() * 0.05;
        if (mix > 1.0) mix = 1.0;
        if (mix < 0.0) mix = 0.0;
        break;
    }
  }

  // Read detune value
  detune = ReadControlInV(3) / 5.0f;

  // Tune each osc
  for (int i = 0; i < 3; i++) {
    float osc_e1vo = ReadControlInV(i);
    osc_st[i]      = e1vo_to_semitone(osc_e1vo) + fund_st;
    float osc_hz   = semitone_to_hz(osc_st[i]);
    osc[i].SetDetuneAndFreq(detune, osc_hz);
    osc[i].SetMix(mix);
  }
}

int main(void) {
  gPatch.Init();  // Initialize hardware (daisy seed, and patch)
  gPatch.StartAdc();

  RunCalibration(&gPatch);

  LoadSettings(&gPatch.seed);

  app.Init(&gPatch);
  app.Run();
}