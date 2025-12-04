#include "usb_ncm_component.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"

#include <lwip/dns.h>
#include <cinttypes>

namespace esphome {
namespace usb_ncm {

static const char *const TAG = "usb_ncm";

USBNCMComponent *global_usb_ncm_component;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

USBNCMComponent::USBNCMComponent() { global_usb_ncm_component = this; }

void USBNCMComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up USB NCM...");

  if (this->manual_ip_.has_value()) {
    IPAddress localIP = IPAddress(this->manual_ip_->static_ip);
    IPAddress gateway = IPAddress(this->manual_ip_->gateway);
    IPAddress netmask = IPAddress(this->manual_ip_->subnet);
    IPAddress dns1 = IPAddress(this->manual_ip_->dns1);
    IPAddress dns2 = IPAddress(this->manual_ip_->dns2);

    this->eth.config(localIP, gateway, netmask, dns1, dns2);
  }

  ESP_LOGI(TAG, "Connecting via USB NCM...");
  bool ok = this->eth.begin();
  if (!ok) {
    this->mark_failed();
  }
  this->state_ = USBNCMComponentState::CONNECTING;
  connect_begin = millis();
}

void USBNCMComponent::loop() {
  const uint32_t now = millis();

  if (this->state_ == USBNCMComponentState::STOPPED) {
  } else if (this->state_ == USBNCMComponentState::CONNECTING) {
    if (now - connect_begin > 5000) {
      connect_begin = now;
      ESP_LOGI(TAG, "Still connecting via USB NCM...");
    }
    if (this->eth.connected()) {
      ESP_LOGI(TAG, "Connected via USB NCM!");

      this->dump_config();
      this->status_clear_warning();
      this->state_ = USBNCMComponentState::CONNECTED;
    }
  } else if (this->state_ == USBNCMComponentState::CONNECTED) {
    if (!this->eth.connected()) {
      ESP_LOGW(TAG, "Connection via USB NCM lost! reconnecting...");
      this->status_set_warning("connection lost");
      connect_begin = now;
      this->state_ = USBNCMComponentState::CONNECTING;
    }
  }
}

void USBNCMComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "USB NCM:");
  ESP_LOGCONFIG(TAG, "  IP Address: %s", network::IPAddress(this->eth.localIP()).str().c_str());
  ESP_LOGCONFIG(TAG, "  Hostname: '%s'", App.get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Subnet: %s", network::IPAddress(this->eth.subnetMask()).str().c_str());
  ESP_LOGCONFIG(TAG, "  Gateway: %s", network::IPAddress(this->eth.gatewayIP()).str().c_str());

  const ip_addr_t *dns_ip1 = dns_getserver(0);
  const ip_addr_t *dns_ip2 = dns_getserver(1);

  ESP_LOGCONFIG(TAG, "  DNS1: %s", network::IPAddress(dns_ip1).str().c_str());
  ESP_LOGCONFIG(TAG, "  DNS2: %s", network::IPAddress(dns_ip2).str().c_str());

  ESP_LOGCONFIG(TAG, "  MAC Address: %s", this->get_usb_ncm_mac_address_pretty().c_str());
}

float USBNCMComponent::get_setup_priority() const { return setup_priority::WIFI; }

bool USBNCMComponent::can_proceed() { return this->is_connected(); }

network::IPAddresses USBNCMComponent::get_ip_addresses() {
  network::IPAddresses addresses;
  addresses[0] = network::IPAddress(this->eth.localIP());
  return addresses;
}

network::IPAddress USBNCMComponent::get_dns_address(uint8_t num) {
  const ip_addr_t *dns_ip = dns_getserver(num);
  return dns_ip;
}

bool USBNCMComponent::is_connected() { return this->state_ == USBNCMComponentState::CONNECTED; }

void USBNCMComponent::set_manual_ip(const ManualIP &manual_ip) { this->manual_ip_ = manual_ip; }

const char *USBNCMComponent::get_use_address() const { return this->use_address_; }

void USBNCMComponent::set_use_address(const char *use_address) { this->use_address_ = use_address; }

void USBNCMComponent::get_usb_ncm_mac_address_raw(uint8_t *mac) { this->eth.macAddress(mac); }

std::string USBNCMComponent::get_usb_ncm_mac_address_pretty() {
  uint8_t mac[6];
  get_mac_address_raw(mac);
  return str_snprintf("%02X:%02X:%02X:%02X:%02X:%02X", 17, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

}  // namespace usb_ncm
}  // namespace esphome
