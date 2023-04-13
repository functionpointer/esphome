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

static const uint8_t NUM_BATTERIES = 16;
static const uint8_t MAX_SENSOR_INDEX = 11;
static const uint8_t NUM_BUFFERS = 20;
static const uint8_t MAX_BINARY_SENSOR_INDEX = 16;

class PylontechComponent : public PollingComponent, public uart::UARTDevice {
 public:
  void set_sensor(sensor::Sensor *sensor, int battery, int index) { this->sensors_[battery-1][index] = sensor; }
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
  void process_line_(std::string &buffer);

  sensor::Sensor *sensors_[NUM_BATTERIES][MAX_SENSOR_INDEX+1] = {{nullptr}};
#ifdef USE_BINARY_SENSOR
  //binary_sensor::BinarySensor *binary_sensors{nullptr};
#endif

  //ring buffer
  std::string buffer_[NUM_BUFFERS];
  int buffer_index_write_ = 0;
  int buffer_index_read_ = 0;
};

class PylontechBinaryComponent : public Component {
 public:
  PylontechBinaryComponent(PylontechComponent *parent) {}
};

}  // namespace hydreon_rgxx
}  // namespace esphome
