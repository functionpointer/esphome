#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/defines.h"
#include "esphome/components/network/ip_address.h"
#ifdef PPPOS_USE_UART_COMPONENT
#include "esphome/components/uart/uart.h"
#endif

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"

#include <queue>

#ifdef PPPOS_USE_CDC
#ifdef USE_ARDUINO
#ifdef USE_RP2040
#include <HardwareSerial.h>
#include <SerialUSB.h>
#endif  // USE_RP2040
#endif  // USE_ARDUINO
#endif // PPPOS_USE_CDC


namespace esphome {
namespace pppos {

struct ManualIP {
  network::IPAddress static_ip;
  network::IPAddress gateway;
  network::IPAddress subnet;
  network::IPAddress dns1;  ///< The first DNS server. 0.0.0.0 for default.
  network::IPAddress dns2;  ///< The second DNS server. 0.0.0.0 for default.
};

enum class PPPoSComponentState {

  STOPPED,
  CONNECTING,
  CONNECTED,
};

class PPPoSComponent : public Component
#ifdef PPPOS_USE_UART_COMPONENT
                       ,public uart::UARTDevice
#endif
{
 public:
  PPPoSComponent();
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  bool can_proceed() override;
  bool is_connected();

  void set_manual_ip(const ManualIP &manual_ip);

  network::IPAddress get_ip_address();
  std::string get_use_address() const;
  void set_use_address(const std::string &use_address);

 protected:
  static void status_callback(ppp_pcb *pcb, int err_code, void *ctx);
  static uint32_t output_callback(ppp_pcb *pcb, const void *data, uint32_t len, void *ctx);

  optional<ManualIP> manual_ip_{};
  std::string use_address_;

  bool started_{false};
  bool connected_{false};
  PPPoSComponentState state_{PPPoSComponentState::STOPPED};
  uint32_t connect_begin_;
  ppp_pcb *ppp_control_block_ = nullptr;
  struct netif ppp_netif_;

  bool uart_read_array(uint8_t *data, size_t len);
  int uart_available();
  void uart_write_array(const uint8_t *data, size_t len);
#ifdef PPPOS_USE_CDC
  uint32_t baud_rate_;
 public:
  void set_baud_rate(uint32_t baud_rate) {this->baud_rate_ = baud_rate;}
  uint32_t get_baud_rate() const { return baud_rate_; }
 protected:
#ifdef USE_ARDUINO
  Stream *hw_serial_{nullptr};
#endif // USE_ARDUINO
#endif // PPPOS_USE_CDC
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern PPPoSComponent *global_pppos_component;

}  // namespace pppos
}  // namespace esphome
