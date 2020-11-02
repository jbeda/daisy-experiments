#include "supersaw.h"

void SuperSaw::Init(float sample_rate) {
  for (int i = 0; i < 7; i++) {
    osc[i].Init(sample_rate);
    osc[i].SetWaveform(daisysp::Oscillator::WAVE_SAW);
  }
  freq_   = 130.81f;  // C3
  amp_    = 0.75f;
  detune_ = 1.0f;
  mix_    = 1.0f;

  UpdateFreqs();
  UpdateAmps();
}

void SuperSaw::SetFreq(float f) {
  if (f != freq_) {
    freq_ = f;
    UpdateFreqs();
  }
}

void SuperSaw::SetAmp(float amp) {
  if (amp != amp_) {
    amp_ = amp;
    UpdateAmps();
  }
}

void SuperSaw::SetDetune(float detune) {
  if (detune != detune_) {
    detune_ = detune;
    UpdateFreqs();
  }
}

void SuperSaw::SetDetuneAndFreq(float detune, float freq) {
  if (detune != detune_ || freq != freq_) {
    detune_ = detune;
    freq_   = freq;
    UpdateFreqs();
  }
}

void SuperSaw::SetMix(float mix) {
  if (mix != mix_) {
    mix_ = mix;
    UpdateAmps();
  }
}

float SuperSaw::Process() {
  float s = 0;
  for (int i = 0; i < 7; i++) {
    s += osc[i].Process();
  }
  return s;
}

void SuperSaw::Reset() {
  for (int i = 0; i < 7; i++) {
    osc[i].Reset();
  }
}

// The max detune for each of the side oscillators.
static const float detune_factors[] = {-0.11002313, -0.06288439, -0.01952356,
                                       0.01991221,  0.06216538,  0.10745242};

void SuperSaw::UpdateFreqs() {
  // We approximate the curve in the paper by the following formula
  // (y=detune_scale, x=detune_):
  //    y = 1.5*x^6 - 0.8*x^5 + 0.3*x^3 - 0.3*x^2 + 0.3*x
  // You can play with that curve here:
  //   https://www.desmos.com/calculator/rwzsa4gsou
  detune_scale_ = 1.5 * powf(detune_, 6) - 0.8 * powf(detune_, 5) +
                  0.3 * powf(detune_, 3) - 0.3 * powf(detune_, 2) +
                  0.3 * detune_;
  osc[0].SetFreq(freq_);
  for (int i = 0; i < 6; i++) {
    osc[i + 1].SetFreq(freq_ + freq_*detune_factors[i] * detune_scale_);
  }
}

void SuperSaw::UpdateAmps() {
  // Formulas are taken from the paper referened above.
  float center_amp = -0.55366 * mix_ + 0.99785;
  osc[0].SetAmp(amp_ * center_amp);

  float side_amp = -0.73764 * mix_ * mix_ + 1.2841 * mix_ + 0.044372;
  for (int i = 1; i < 7; i++) {
    osc[i].SetAmp(amp_ * side_amp);
  }
}