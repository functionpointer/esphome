#include "pylontech.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pylontech {

static const char *const TAG = "pylontech.sensor";
static const int MAX_DATA_LENGTH_BYTES = 256;
static const uint8_t ASCII_LF = 0x0A;


void PylontechComponent::dump_config() {
  this->check_uart_settings(115200, 1, esphome::uart::UART_CONFIG_PARITY_NONE, 8);
  ESP_LOGCONFIG(TAG, "pylontech:");
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Connection with pylontech failed!");
  }

  for(int batid=0;batid<NUM_BATTERIES;batid++) {
    bool any = false;
    for(int i=0;i<=MAX_SENSOR_INDEX;i++) {
      if(this->sensors_[batid][i]!=nullptr) {
        if(!any) {
          ESP_LOGCONFIG(TAG, " Battery %d", batid+1);
        }
        any = true;
        LOG_SENSOR("  ", "", this->sensors_[batid][i]);
      }
    }
  }

  LOG_UPDATE_INTERVAL(this);
}

void PylontechComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up pylontech...");
  while (this->available() != 0) {
    this->read();
  }
}

void PylontechComponent::update() {
  this->write_str("pwr\n");
}

void PylontechComponent::loop() {
  uint8_t data;
  while (this->available() > 0) {
    if (this->read_byte(&data)) {
      buffer_[buffer_index_write_] += (char) data;
      if (buffer_[buffer_index_write_].back() == static_cast<char>(ASCII_LF) || buffer_[buffer_index_write_].length() >= MAX_DATA_LENGTH_BYTES) {
        // complete line received
        buffer_index_write_ =(buffer_index_write_ +1)%NUM_BUFFERS;
      }
    }
  }
  if(buffer_index_read_ != buffer_index_write_) {
    this->process_line_(buffer_[buffer_index_read_]);
    buffer_[buffer_index_read_].clear();
    buffer_index_read_ =(buffer_index_read_ +1)%NUM_BUFFERS;
  }
}

void PylontechComponent::process_line_(std::string &buffer) {
  ESP_LOGV(TAG, "Read from serial: %s", buffer.substr(0, buffer.size() - 2).c_str());

  if(!isdigit(buffer[0])) {
    return;
  }
  auto end_batnum = buffer.find_first_of(" \t");
  if(end_batnum >2) {
    return;
  }
  int const bat_num = parse_number<int>(buffer.substr(0, end_batnum)).value_or(-1);
  if(bat_num<=0 || bat_num>NUM_BATTERIES) {
    return;
  }

  auto current_index = end_batnum;
  for(int i=0;i<=MAX_SENSOR_INDEX;i++) {
    //find next non-whitespace
    while(current_index < buffer.size() && std::isspace(buffer[current_index])) {
      current_index++;
    }
    if(current_index==buffer.size()){
      return;
    }

    auto end = buffer.find_first_of(" \t%", current_index);
    if(end==std::string::npos) {
      return;
    }
    auto subs = buffer.substr(current_index, end-current_index);
    ESP_LOGV(TAG, "sensor %d value %s", i, subs.c_str());
    if(this->sensors_[bat_num-1][i]!= nullptr) {
      int const parsed_value = parse_number<int>(subs).value_or(-1);
      if(parsed_value==-1) {
        ESP_LOGW(TAG, "failed to parse number \"%s\"", subs.c_str());
        continue;
      }
      float val = parsed_value;
      if(buffer[end]!='%') {
        val/=1000;
      }
      this->sensors_[bat_num-1][i]->publish_state(val);
    }
    current_index = end;
    if(buffer[current_index]=='%') {
      current_index++;
    }
  }
}

float PylontechComponent::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace hydreon_rgxx
}  // namespace esphome
