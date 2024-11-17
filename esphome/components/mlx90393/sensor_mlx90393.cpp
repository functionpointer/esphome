#include "sensor_mlx90393.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mlx90393 {

static const char *const TAG = "mlx90393";

bool MLX90393Cls::transceive(const uint8_t *request, size_t request_size, uint8_t *response, size_t response_size) {
  i2c::ErrorCode e = this->write(request, request_size);
  if (e != i2c::ErrorCode::ERROR_OK) {
    return false;
  }
  e = this->read(response, response_size);
  return e == i2c::ErrorCode::ERROR_OK;
}

bool MLX90393Cls::has_drdy_pin() { return this->drdy_pin_ != nullptr; }

bool MLX90393Cls::read_drdy_pin() {
  if (this->drdy_pin_ == nullptr) {
    return false;
  } else {
    return this->drdy_pin_->digital_read();
  }
}
void MLX90393Cls::sleep_millis(uint32_t millis) { delay(millis); }
void MLX90393Cls::sleep_micros(uint32_t micros) { delayMicroseconds(micros); }
uint32_t MLX90393Cls::millis() { return ::millis(); }

void MLX90393Cls::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MLX90393...");
  // note the two arguments A0 and A1 which are used to construct an i2c address
  // we can hard-code these because we never actually use the constructed address
  // see the transceive function above, which uses the address from I2CComponent
  this->mlx_.begin_with_hal(this, 0, 0);

  this->mlx_.setGainSel(this->gain_);

  this->mlx_.setResolution(this->resolutions_[0], this->resolutions_[1], this->resolutions_[2]);

  this->mlx_.setOverSampling(this->oversampling_);

  this->mlx_.setDigitalFiltering(this->filter_);

  this->mlx_.setTemperatureOverSampling(this->temperature_oversampling_);

  this->mlx_.setTemperatureCompensation(this->temperature_compensation_);

  this->mlx_.setHallConf(this->hallconf_);
}

void MLX90393Cls::dump_config() {
  ESP_LOGCONFIG(TAG, "MLX90393:");
  LOG_I2C_DEVICE(this);

  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with MLX90393 failed!");
    return;
  }
  LOG_UPDATE_INTERVAL(this);

  LOG_SENSOR("  ", "X Axis", this->x_sensor_);
  LOG_SENSOR("  ", "Y Axis", this->y_sensor_);
  LOG_SENSOR("  ", "Z Axis", this->z_sensor_);
  LOG_SENSOR("  ", "Temperature", this->t_sensor_);
}

float MLX90393Cls::get_setup_priority() const { return setup_priority::DATA; }

void MLX90393Cls::update() {
  MLX90393::return_status_t status = this->mlx.checkStatus(
      this->mlx_.startMeasurement(MLX90393::X_FLAG | MLX90393::Y_FLAG | MLX90393::Z_FLAG | MLX90393::T_FLAG));
}

void MLX90393Cls::loop() { if () }

}  // namespace mlx90393
}  // namespace esphome
