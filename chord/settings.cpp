#include "settings.h"

#include <cstring>

#include "daisy_seed.h"

// Storage for the global settings struct
Settings gSettings;

const uint32_t signature      = 0xBEDA;
const uint32_t latest_version = 2;

void SaveSettings(daisy::DaisySeed* seed) {
  gSettings.signature = signature;
  gSettings.version   = latest_version;

  seed->qspi_handle.mode = DSY_QSPI_MODE_INDIRECT_POLLING;
  dsy_qspi_init(&seed->qspi_handle);
  uint32_t base      = 0x90000000;
  uint32_t writesize = sizeof(Settings);
  dsy_qspi_erase(base, base + writesize);
  dsy_qspi_write(base, writesize, reinterpret_cast<uint8_t*>(&gSettings));
  dsy_qspi_deinit();
}

void LoadSettings(daisy::DaisySeed* seed) {
  seed->qspi_handle.mode = DSY_QSPI_MODE_DSY_MEMORY_MAPPED;
  dsy_qspi_init(&seed->qspi_handle);
  memcpy(&gSettings, reinterpret_cast<void*>(0x90000000), sizeof(Settings));
  dsy_qspi_deinit();

  if (gSettings.signature != signature || gSettings.version != latest_version) {
    // Invalid settings or version mismatch
    // TODO: upgrade versions
    memset(&gSettings, 0, sizeof(Settings));
  }
}