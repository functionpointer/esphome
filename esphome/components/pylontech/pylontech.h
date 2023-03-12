#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/components/sensor/sensor.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace pylontech {

#ifndef NUM_BATTERIES
#define NUM_BATTERIES 6
#endif

#ifndef MAX_SENSOR_INDEX
#define MAX_SENSOR_INDEX 11
#endif

#define MIN_BINARY_SENSOR_INDEX 7
#define MAX_BINARY_SENSOR_INDEX 16

class PylontechComponent : public PollingComponent, public uart::UARTDevice {
 public:
  void set_sensor(sensor::Sensor *sensor, int battery, int index) { this->sensors_[battery][index] = sensor; }
#ifdef USE_BINARY_SENSOR
#endif

  /// Schedule data readings.
  void update() override;
  /// Read data once available
  void loop() override;
  /// Setup the sensor and test for a connection.
  void setup() override;
  void dump_config() override;

  float get_setup_priority() const override;

 protected:
  void process_line_();
  void schedule_reboot_();
  bool buffer_starts_with_(const std::string &prefix);
  bool buffer_starts_with_(const char *prefix);
  int num_sensors_missing_();

  sensor::Sensor *sensors_[NUM_BATTERIES][MAX_SENSOR_INDEX] = {{nullptr}};
#ifdef USE_BINARY_SENSOR
  //binary_sensor::BinarySensor *binary_sensors{nullptr};
#endif
};

class PylontechBinaryComponent : public Component {
 public:
  PylontechBinaryComponent(PylontechComponent *parent) {}
};

}  // namespace hydreon_rgxx
}  // namespace esphome
