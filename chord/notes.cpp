#include "notes.h"

#include <string>

#include "daisy.h"
#include "daisysp.h"

float e1vo_to_hz(float e1vo) { return powf(2.f, e1vo) * 55.; }

int e1vo_to_semitone(float e1vo) {
  return static_cast<int>(std::round(e1vo * 12.0));
}

float semitone_to_e1vo(int semitone) {
  return static_cast<float>(semitone)/12.0f;
}

float quantize_e1vo(float e1vo) {
  return semitone_to_e1vo(e1vo_to_semitone(e1vo));
}

const char* semitones[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
};

std::string semitone_name(int semitone) {
  // C0 is 12
  // B8 is 119
  if (semitone < 10 || semitone > 119) {
    return "???";
  }

  semitone -= 12;
  int octave = semitone / 12;
  int note   = semitone % 12;

  std::string out(semitones[note]);
  out.push_back('0'+octave);
  return out;
}

float semitone_to_hz(int semitone) {
  return daisysp::mtof(semitone);
}