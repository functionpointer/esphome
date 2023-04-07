#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/network/ip_address.h"
#include "esphome/components/uart/uart.h"

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"


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

class PPPoSComponent : public Component, public uart::UARTDevice {
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

 protected:
  static void status_callback(ppp_pcb *pcb, int err_code, void *ctx);
  static uint32_t output_callback(ppp_pcb *pcb, const void *data, uint32_t len, void *ctx);

  optional<ManualIP> manual_ip_{};

  bool started_{false};
  bool connected_{false};
  PPPoSComponentState state_{PPPoSComponentState::STOPPED};
  uint32_t connect_begin_;
  ppp_pcb *ppp_control_block;
  struct netif ppp_netif_;
/*#ifdef USE_RP2040
  static async_context_t *async_context;
  static async_context_poll_t async_context_poll_;
#endif*/
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern PPPoSComponent *global_pppos_component;

}  // namespace pppos
}  // namespace esphome
