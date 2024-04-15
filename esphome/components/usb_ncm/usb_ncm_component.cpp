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

  //pico_unique_board_id_t id;
  //pico_get_unique_board_id(&id);

  const uint8_t tud_network_mac_address[6] = {0x02,0x02,0x84,0x6A,0x96,0x00};
  /* the lwip virtual MAC address must be different from the host's; to ensure this, we toggle the LSbit */
  this->netif_.hwaddr_len = sizeof(tud_network_mac_address);
  //memcpy(this->netif_.hwaddr+1, (id.id)+1, sizeof(tud_network_mac_address)-1);
  memcpy(this->netif_.hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
  this->netif_.hwaddr[0] = 0x02;
  this->netif_.hwaddr[5] ^= 0x01;

}

void USBNCMComponent::start_connect_() {
  ip_addr_t ipaddr = IPADDR4_INIT(0);
  ip_addr_t netmask = IPADDR4_INIT(0);
  ip_addr_t gateway = IPADDR4_INIT(0);

  ip_addr_t hostIp = IPADDR4_INIT(0);
  ip_addr_t ownIp = IPADDR4_INIT(0);

  if (this->manual_ip_.has_value()) {
    ipaddr = IPADDR4_INIT(static_cast<uint32_t>(this->manual_ip_->static_ip));
    gateway = IPADDR4_INIT(static_cast<uint32_t>(this->manual_ip_->gateway));
    netmask = IPADDR4_INIT(static_cast<uint32_t>(this->manual_ip_->subnet));
  }

  netif_add(&this->netif_, &ipaddr, &netmask, &gateway, NULL, USBNCMComponent::netif_init_callback, ip4_input);
  netif_set_status_callback(&this->netif_, USBNCMComponent::netif_status_callback);
  netif_set_link_callback(&this->netif_, USBNCMComponent::netif_link_callback);
  netif_set_default(&this->netif_);


  this->netif_.flags |= NETIF_FLAG_IGMP;
  auto igmp_result = igmp_start( &this->netif_ );
  ESP_LOGD(TAG, "IGMP START: %u", igmp_result);

}


void tud_network_init_cb(void)
{
    /* if the network is re-initializing and we have a leftover packet, we must do a cleanup */
    if (global_usbncm_component->_received_frame)
    {
      pbuf_free(global_usbncm_component->_received_frame);
      _received_frame = NULL;
    }
}

// tinyusb callback when packet is received
// we store it and let loop() process it
bool tud_network_recv_cb(const uint8_t *src, uint16_t size)
{
    /* this shouldn't happen, but if we get another packet before
    parsing the previous, we must signal our inability to accept it */
    if (global_usbncm_component->_received_frame) return false;

    usbTraffic = 1;

    if (size)
    {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

        if (p)
        {
            /* pbuf_alloc() has already initialized struct; all we need to do is copy the data */
            memcpy(p->payload, src, size);

            /* store away the pointer for service_traffic() to later handle */
            global_usbncm_component->_received_frame = p;
        }
    }

    return true;
}

void USBNCMComponent::loop() {

    //take packet stored by tud_network_recv_cb and send it up lwip stack
    if (this->_received_frame)
    {
      ethernet_input(this->_received_frame, &this->netif_);
      pbuf_free(this->_received_frame);
      this->__received_frame = NULL;
      tud_network_recv_renew();
    }
    tud_task();

    //sys_check_timeouts();

}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg)
{
    struct pbuf *p = (struct pbuf *)ref;

    (void)arg; /* unused for this example */

    pbuf_copy_partial(p, dst, p->tot_len, 0);

    return p->tot_len;
}

err_t USBNCMComponent::linkoutput_fn(struct netif *netif, struct pbuf *p)
{
    (void)netif;

    for (;;)
    {
        /* if TinyUSB isn't ready, we must signal back to lwip that there is nothing we can do */
        if (!tud_ready()) {
            return ERR_USE;
        }

        /* if the network driver can accept another packet, we make it happen */
        if (tud_network_can_xmit(p->tot_len))
        {
          tud_network_xmit(p, 0 /* unused for this example */);
          return ERR_OK;
        }

      /* transfer execution to TinyUSB in the hopes that it will finish transmitting the prior packet */
      tud_task();
     }
}

err_t USBNCMComponent::output_fn(struct netif *netif, struct pbuf *p, const ip_addr_t *addr)
{
    return etharp_output(netif, p, addr);
}

err_t USBNCMComponent::netif_init_callback(struct netif *netif) {
  //ESP_LOGD(TAG, "netif_init_cb");
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
  netif->state = NULL;
  netif->name[0] = 'u'; // for "USB"
  netif->name[1] = '0';
  netif->linkoutput = USBNCMComponent::linkoutput_fn;
  netif->output = USBNCMComponent::output_fn;

  netif->hostname = global_usbncm_component->get_use_address().c_str();

  return ERR_OK;
}

void USBNCMComponent::netif_link_callback(struct netif *state_netif) {
  if (netif_is_link_up(state_netif)) {
    ESP_LOGI(TAG, "netif_link_callback==UP\n");
  } else {
    ESP_LOGI(TAG, "netif_link_callback==DOWN\n");
  }
}

void USBNCMComponent::netif_status_callback(struct netif *nif) {
  ESP_LOGD(TAG, "netif_status_callback: %c%c%d is %s\n", nif->name[0], nif->name[1], nif->num, netif_is_up(nif) ? "UP" : "DOWN");
}


void USBNCMComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "USB NCM:");
}

float USBNCMComponent::get_setup_priority() const { return setup_priority::WIFI; }

bool USBNCMComponent::can_proceed() { return this->is_connected() || this->is_failed(); }

network::IPAddress USBNCMComponent::get_ip_address() {
  return network::IPAddress(this->netif_.ip_addr.addr);
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
