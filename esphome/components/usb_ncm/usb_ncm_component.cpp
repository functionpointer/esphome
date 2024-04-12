#include "usb_ncm_component.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"
#include "esphome/core/application.h"

#include <lwip/igmp.h>
#include <lwip/init.h>
#include <tusb.h>

namespace esphome {
namespace usb_ncm {

static const char *const TAG = "usb_ncm";

USBNCMComponent *global_usbncm_component;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

USBNCMComponent::USBNCMComponent() { global_usbncm_component = this; }

void USBNCMComponent::setup() {
  //implementation inspired by https://github.com/OpenLightingProject/rp2040-dmxsun
  ESP_LOGCONFIG(TAG, "Setting up USB NCM...");

  pico_unique_board_id_t id;
  pico_get_unique_board_id(&id);

  const uint8_t tud_network_mac_address[6] = {0x02,0x02,0x84,0x6A,0x96,0x00};
  /* the lwip virtual MAC address must be different from the host's; to ensure this, we toggle the LSbit */
  this->netif_->hwaddr_len = sizeof(tud_network_mac_address);
  memcpy(this->netif_->hwaddr+1, (id.id)+1, sizeof(tud_network_mac_address)-1);
  this->netif_->hwaddr[0] = 0x02;
  this->netif_->hwaddr[5] ^= 0x01;

}

void USBNCMComponent::start_connect_() {
  ip_addr_t ipaddr = 0;
  ip_addr_t netmask = 0;
  ip_addr_t gateway = 0;

  ip_addr_t hostIp = 0;
  ip_addr_t ownIp = 0;

  if (this->manual_ip_.has_value()) {
    ipaddr = static_cast<uint32_t>(this->manual_ip_->static_ip);
    gateway = static_cast<uint32_t>(this->manual_ip_->gateway);
    netmask = static_cast<uint32_t>(this->manual_ip_->subnet);
  }

  this->netif_ = netif_add(this->netif_, &ipaddr, &netmask, &gateway, NULL, USBNCMComponent::netif_init_callback, ip_input);
  netif_set_status_callback(this->netif_, USBNCMComponent::netif_status_callback);
  netif_set_link_callback(this->netif_, USBNCMComponent::netif_link_callback);
  netif_set_default(this->netif_);


  this->netif_->flags |= NETIF_FLAG_IGMP;
  auto igmp_result = igmp_start( this->netif_ );
  LOG("IGMP START: %u", igmp_result);

}

void USBNCMComponent::loop() {

}

err_t USBNCMComponent::netif_init_callback(struct netif *netif) {
  LOG("netif_init_cb");
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
  netif->state = NULL;
  netif->name[0] = 'u'; // for "USB"
  netif->name[1] = '0';
  netif->linkoutput = linkoutput_fn;
  netif->output = output_fn;

  netif->hostname = global_usbncm_component->get_use_address();

  return ERR_OK;
}

void USBNCMComponent::netif_link_callback(struct netif *state_netif) {
  if (netif_is_link_up(state_netif)) {
    LOG("netif_link_callback==UP\n");
  } else {
    LOG("netif_link_callback==DOWN\n");
  }
}

void USBNCMComponent::netif_status_callback(struct netif *nif) {
  LOG("netif_status_callback: %c%c%d is %s\n", nif->name[0], nif->name[1], nif->num, netif_is_up(nif) ? "UP" : "DOWN");
}


void USBNCMComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "USB NCM:");
}

float USBNCMComponent::get_setup_priority() const { return setup_priority::WIFI; }

bool USBNCMComponent::can_proceed() { return this->is_connected() || this->is_failed(); }

network::IPAddress USBNCMComponent::get_ip_address() {
  return network::IPAddress(this->ppp_netif_.ip_addr.addr);
}

void USBNCMComponent::set_use_address(const std::string &use_address) { this->use_address_ = use_address; }

std::string USBNCMComponent::get_use_address() const {
  if (this->use_address_.empty()) {
    return App.get_name() + ".local";
  }
  return this->use_address_;
}

bool USBNCMComponent::is_connected() {
  return this->state_ == USBNCMComponentState::CONNECTED;
}

void USBNCMComponent::set_manual_ip(const ManualIP &manual_ip) { this->manual_ip_ = manual_ip; }


}  // namespace pppos
}  // namespace esphome
