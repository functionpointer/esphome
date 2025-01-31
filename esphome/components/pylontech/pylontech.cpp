#include "pylontech.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace pylontech {

static const char *const TAG = "pylontech";
static const int MAX_DATA_LENGTH_BYTES = 256;
static const uint8_t ASCII_LF = 0x0A;

PylontechComponent::PylontechComponent() {}

void PylontechComponent::dump_config() {
  this->check_uart_settings(115200, 1, esphome::uart::UART_CONFIG_PARITY_NONE, 8);
  ESP_LOGCONFIG(TAG, "pylontech:");
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Connection with pylontech failed!");
  }

  for (PylontechListener *listener : this->listeners_) {
    listener->dump_config();
  }

  LOG_UPDATE_INTERVAL(this);
}

void PylontechComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up pylontech...");
  while (this->available() != 0) {
    this->read();
  }
}

void PylontechComponent::update() { this->write_str("pwr\n"); }

void PylontechComponent::loop() {
  if (this->available() > 0) {
    // pylontech sends a lot of data very suddenly
    // we need to quickly put it all into our own buffer, otherwise the uart's buffer will overflow
    uint8_t data;
    int recv = 0;
    while (this->available() > 0) {
      if (this->read_byte(&data)) {
        buffer_[buffer_index_write_] += (char) data;
        recv++;
        if (buffer_[buffer_index_write_].back() == static_cast<char>(ASCII_LF) ||
            buffer_[buffer_index_write_].length() >= MAX_DATA_LENGTH_BYTES) {
          // complete line received
          buffer_index_write_ = (buffer_index_write_ + 1) % NUM_BUFFERS;
        }
      }
    }
    ESP_LOGV(TAG, "received %d bytes", recv);
  } else {
    // only process one line per call of loop() to not block esphome for too long
    if (buffer_index_read_ != buffer_index_write_) {
      this->process_line_(buffer_[buffer_index_read_]);
      buffer_[buffer_index_read_].clear();
      buffer_index_read_ = (buffer_index_read_ + 1) % NUM_BUFFERS;
    }
  }
}

void PylontechComponent::process_line_(std::string &buffer) {
  ESP_LOGV(TAG, "Read from serial: %s", buffer.substr(0, buffer.size() - 2).c_str());
  // clang-format off
  // example line to parse:
  // Power Volt  Curr Tempr Tlow  Thigh  Vlow Vhigh Base.St Volt.St Curr.St Temp.St Coulomb Time                B.V.St B.T.St MosTempr M.T.St
  // 1    50548  8910 25000 24200 25000  3368 3371  Charge  Normal  Normal  Normal  97%     2021-06-30 20:49:45 Normal Normal 22700    Normal
  // clang-format on

  PylontechListener::LineContents l{};
  char mostempr_s[6];
  const int parsed = sscanf(                                                                                   // NOLINT
      buffer.c_str(), "%d %d %d %d %d %d %d %d %7s %7s %7s %7s %d%% %*d-%*d-%*d %*d:%*d:%*d %*s %*s %5s %*s",  // NOLINT
      &l.bat_num, &l.volt, &l.curr, &l.tempr, &l.tlow, &l.thigh, &l.vlow, &l.vhigh, l.base_st, l.volt_st,      // NOLINT
      l.curr_st, l.temp_st, &l.coulomb, mostempr_s);                                                           // NOLINT

  if (l.bat_num <= 0) {
    ESP_LOGD(TAG, "invalid bat_num in line %s", buffer.substr(0, buffer.size() - 2).c_str());
    return;
  }
  if (parsed != 14) {
    ESP_LOGW(TAG, "invalid line: found only %d items in %s", parsed, buffer.substr(0, buffer.size() - 2).c_str());
    return;
  }
  auto mostempr_parsed = parse_number<int>(mostempr_s);
  if (mostempr_parsed.has_value()) {
    l.mostempr = mostempr_parsed.value();
  } else {
    l.mostempr = -300;
    ESP_LOGW(TAG, "bat_num %d: received no mostempr", l.bat_num);
  }

  for (PylontechListener *listener : this->listeners_) {
    listener->on_line_read(&l);
  }
}

float PylontechComponent::get_setup_priority() const { return setup_priority::DATA; }

/*
 * possible future commands:
 *
 * set time to 2022-04-26T20:53:00
 * > time 22 04 26 20 53 00
 *
 * possibly unlock for some batteries:
 * > login debug
 *
 * set communication to 115200 (send via 1200baud 8n1):
 * > \x7e\x32\x30\x30\x31\x34\x36\x38\x32\x43\x30\x30\x34\x38\x35\x32\x30\x46\x43\x43\x33\x0d
 * then switch to 115200
 * > \x0D\x0A
 * pylontech> prompt should appear
 *
 * read state of health:
 * > soh
 *
 * read statistics:
 * > stat1
 * > stat2
 * ...
 *
 * other commands:
 *
 * > pwrsys
 * > getpwr1
 * > getpwr2
 * ...
 * > logout
 * > help
 */

}  // namespace pylontech
}  // namespace esphome
