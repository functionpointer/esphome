#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/defines.h"
#include "esphome/components/network/ip_address.h"

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netif.h"

#include "lwip/igmp.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"

#include <queue>


namespace esphome {
namespace usb_ncm {

struct ManualIP {
  network::IPAddress static_ip;
  network::IPAddress gateway;
  network::IPAddress subnet;
  network::IPAddress dns1;  ///< The first DNS server. 0.0.0.0 for default.
  network::IPAddress dns2;  ///< The second DNS server. 0.0.0.0 for default.
};

enum class USBNCMComponentState {

  STOPPED,
  CONNECTING,
  CONNECTED,
};

class USBNCMComponent : public Component
{
 public:
  USBNCMComponent();
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  bool can_proceed() override;
  bool is_connected();
  void start_connect_();

  void set_manual_ip(const ManualIP &manual_ip);

  network::IPAddress get_ip_address();
  std::string get_use_address() const;
  void set_use_address(const std::string &use_address);

 protected:
  optional<ManualIP> manual_ip_{};
  std::string use_address_;
  struct netif netif_;

  static err_t netif_init_callback(struct netif *netif);
  static void netif_status_callback(ppp_pcb *pcb, int err_code, void *ctx);
  static void netif_link_callback(ppp_pcb *pcb, int err_code, void *ctx);

  USBNCMComponentState state_{USBNCMComponentState::STOPPED};


  bool uart_read_array(uint8_t *data, size_t len);
  int uart_available();
  void uart_write_array(const uint8_t *data, size_t len);
};

}  // namespace pppos
}  // namespace esphome
