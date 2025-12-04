#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include "esphome/components/network/ip_address.h"

#ifdef USE_RP2040
#include <NCMEthernetlwIP.h>
#endif

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

class USBNCMComponent : public Component {
 public:
  USBNCMComponent();
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  bool can_proceed() override;
  bool is_connected();

  void set_manual_ip(const ManualIP &manual_ip);

  network::IPAddresses get_ip_addresses();
  network::IPAddress get_dns_address(uint8_t num);
  const char *get_use_address() const;
  void set_use_address(const char *use_address);
  void get_usb_ncm_mac_address_raw(uint8_t *mac);
  std::string get_usb_ncm_mac_address_pretty();

 protected:
  NCMEthernetlwIP eth;

  optional<ManualIP> manual_ip_{};

  USBNCMComponentState state_{USBNCMComponentState::STOPPED};
  uint32_t connect_begin = 0;

 private:
  // Stores a pointer to a string literal (static storage duration).
  // ONLY set from Python-generated code with string literals - never dynamic strings.
  const char *use_address_{""};
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern USBNCMComponent *global_usb_ncm_component;

}  // namespace usb_ncm
}  // namespace esphome
