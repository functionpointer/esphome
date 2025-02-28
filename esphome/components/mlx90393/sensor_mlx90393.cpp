#include "sensor_mlx90393.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mlx90393 {

static const char *const TAG = "mlx90393";

const char* MLX90393Cls::SETTING_NAMES[] = {
    "gain",
    "resolution",
    "oversampling",
    "digital filtering",
    "temperature oversampling",
    "temperature compensation",
    "hallconf",
    "error",
};

bool MLX90393Cls::transceive(const uint8_t *request, size_t request_size, uint8_t *response, size_t response_size) {
  i2c::ErrorCode e = this->write(request, request_size);
  if (e != i2c::ErrorCode::ERROR_OK) {
    ESP_LOGV(TAG, "i2c failed to write %u", e);
    return false;
  }
  e = this->read(response, response_size);
  if (e != i2c::ErrorCode::ERROR_OK) {
    ESP_LOGV(TAG, "i2c failed to read %u", e);
    return false;
  }
  return true;
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

uint8_t MLX90393Cls::apply_setting_(VerifySettingsStage which) {
  uint8_t ret = -1;
  switch(which) {
    case VERIFY_SETTINGS_GAIN_SEL:
      ret = this->mlx_.setGainSel(this->gain_);
      break;
    case VERIFY_SETTINGS_RESOLUTION:
      ret = this->mlx_.setResolution(this->resolutions_[0], this->resolutions_[1], this->resolutions_[2]);
      break;
    case VERIFY_SETTINGS_OVER_SAMPLING:
      ret = this->mlx_.setOverSampling(this->oversampling_);
        break;
    case VERIFY_SETTINGS_DIGITAL_FILTERING:
      ret = this->mlx_.setDigitalFiltering(this->filter_);
        break;
    case VERIFY_SETTINGS_TEMPERATURE_OVER_SAMPLING:
      ret = this->mlx_.setTemperatureOverSampling(this->temperature_oversampling_);
        break;
    case VERIFY_SETTINGS_TEMPERATURE_COMPENSATION:
      ret = this->mlx_.setTemperatureCompensation(this->temperature_compensation_);
        break;
    case VERIFY_SETTINGS_HALLCONF:
      ret = this->mlx_.setHallConf(this->hallconf_);
        break;
    default:
      break;
  }
  if (ret != MLX90393::STATUS_OK) {
    ESP_LOGE(TAG, "failed to apply %s", SETTING_NAMES[which]);
  }
  return ret;
}

bool MLX90393Cls::apply_all_settings_() {
  // perform dummy read after reset
  // first one always gets NAK even tough everything is fine
  uint8_t ignore = 0;
  this->mlx_.getGainSel(ignore);

  uint8_t result = MLX90393::STATUS_OK;
  for(int i=VERIFY_SETTINGS_GAIN_SEL; i!=VERIFY_SETTINGS_LAST; i++) {
    VerifySettingsStage stage = static_cast<VerifySettingsStage>(i);
    result |= this->apply_setting_(stage);
  }
  return result == MLX90393::STATUS_OK;
}

void MLX90393Cls::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MLX90393...");
  // note the two arguments A0 and A1 which are used to construct an i2c address
  // we can hard-code these because we never actually use the constructed address
  // see the transceive function above, which uses the address from I2CComponent
  this->mlx_.begin_with_hal(this, 0, 0);

  if (!this->apply_all_settings_()) {
    this->mark_failed();
  }
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
  MLX90393::txyz data;

  if (this->mlx_.readData(data) == MLX90393::STATUS_OK) {
    ESP_LOGD(TAG, "received %f %f %f", data.x, data.y, data.z);
    if (this->x_sensor_ != nullptr) {
      this->x_sensor_->publish_state(data.x);
    }
    if (this->y_sensor_ != nullptr) {
      this->y_sensor_->publish_state(data.y);
    }
    if (this->z_sensor_ != nullptr) {
      this->z_sensor_->publish_state(data.z);
    }
    if (this->t_sensor_ != nullptr) {
      this->t_sensor_->publish_state(data.t);
    }
    this->status_clear_warning();
  } else {
    ESP_LOGE(TAG, "failed to read data");
    this->status_set_warning();
  }

  // perform verifications. if a register has an unexpected value, reset chip and set everything again
  if (!this->verify_all_settings_()) {
    if (this->mlx_.checkStatus(this->mlx_.reset()) != MLX90393::STATUS_OK) {
      ESP_LOGE(TAG, "failed to reset device");
      this->status_set_error();
      this->mark_failed();
      return;
    }

    if (!this->apply_all_settings_()) {
      ESP_LOGE(TAG, "failed to re-apply settings");
      this->status_set_error();
      this->mark_failed();
    } else {
      ESP_LOGI(TAG, "reset and re-apply settings completed");
    }
  }
}

bool MLX90393Cls::verify_setting_(VerifySettingsStage which) {
  uint8_t read_value[3] = {0xFF};
  uint8_t *expected_value = nullptr;
  uint8_t num_values = 1;

  uint8_t read_status = -1;

  switch (which) {
    case VERIFY_SETTINGS_GAIN_SEL:
      read_status = this->mlx_.getGainSel(read_value[0]);
      expected_value = &this->gain_;
      break;
    case VERIFY_SETTINGS_RESOLUTION:
      read_status = this->mlx_.getResolution(read_value[0], read_value[1], read_value[2]);
      expected_value = &this->resolutions_[0];
      num_values = 3;
      break;
    case VERIFY_SETTINGS_OVER_SAMPLING:
      read_status = this->mlx_.getOverSampling(read_value[0]);
      expected_value = &this->oversampling_;
      break;
    case VERIFY_SETTINGS_DIGITAL_FILTERING:
      read_status = this->mlx_.getDigitalFiltering(read_value[0]);
      expected_value = &this->filter_;
      break;
    case VERIFY_SETTINGS_TEMPERATURE_OVER_SAMPLING:
      read_status = this->mlx_.getTemperatureOverSampling(read_value[0]);
      expected_value = &this->temperature_oversampling_;
      break;
    case VERIFY_SETTINGS_TEMPERATURE_COMPENSATION:
      read_status = this->mlx_.getTemperatureCompensation(read_value[0]);
      expected_value = (uint8_t*)&this->temperature_compensation_;
      break;
    case VERIFY_SETTINGS_HALLCONF:
      read_status = this->mlx_.getHallConf(read_value[0]);
      expected_value = &this->hallconf_;
      break;
    default:
      return false;
  }
  if(read_status != MLX90393::STATUS_OK) {
    ESP_LOGE(TAG, "verify error: failed to read %s", SETTING_NAMES[which]);
    return false;
  }
  bool is_correct = true;
  for (int i=0;i<num_values;i++) {
    is_correct &= read_value[i] == expected_value[i];
  }
  if (!is_correct) {
    ESP_LOGW(TAG, "verify failed: read back wrong %s: got %u expected %u", SETTING_NAMES[which], read_value[0], expected_value[0]);
    return false;
  }
  ESP_LOGD(TAG, "verify succeeded for %s. got %u", SETTING_NAMES[which], read_value[0]);
  return true;
}

/**
 * Regularly checks that our settings are still applied.
 * Used to catch spurious chip resets.
 *
 * returns true if everything is fine.
 * false if not
 */
bool MLX90393Cls::verify_all_settings_() {
  static enum VerifySettingsStage stage = VERIFY_SETTINGS_GAIN_SEL;
  static uint32_t last_verify = 0;

  // verify at most once every 3s
  if (millis() - last_verify > 3000) {
    last_verify = millis();
    this->verify_setting_(stage);

    // advance to next verify stage
    stage = static_cast<VerifySettingsStage>(static_cast<int>(stage) + 1);
    if (stage == VERIFY_SETTINGS_LAST) {
      stage = static_cast<VerifySettingsStage>(0);
    }
  }
  return true;
}

}  // namespace mlx90393
}  // namespace esphome
