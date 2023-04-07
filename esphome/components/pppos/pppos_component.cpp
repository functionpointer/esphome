#include "pppos_component.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"
#include "esphome/core/application.h"

#include <pico/cyw43_arch.h>

namespace esphome {
namespace pppos {

static const char *const TAG = "pppos";

PPPoSComponent *global_pppos_component;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

PPPoSComponent::PPPoSComponent() { global_pppos_component = this; }

void PPPoSComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PPPoS...");

#ifdef USE_RP2040
  /*
  if (!async_context_poll_init_with_defaults(&this->async_context_poll_)) {
    ESP_LOGE(TAG, "async_context_init_with_defaults has failed");
    this->mark_failed();
    return;
  }
  this->async_context = &this->async_context_poll_.core;
  if(!this->async_context) {
    ESP_LOGE(TAG, "no async context!");
    this->mark_failed();
    return;
  }
  lwip_nosys_init(this->async_context);*/

  cyw43_arch_init();
#endif

  this->ppp_control_block = pppos_create(&this->ppp_netif_, PPPoSComponent::output_callback, PPPoSComponent::status_callback, nullptr);
  ESP_LOGCONFIG(TAG, "pppos_create");
  //we are the PPP client
  ppp_set_default(this->ppp_control_block);
  ppp_set_usepeerdns(this->ppp_control_block, 1);
  ppp_connect(this->ppp_control_block, 0);
  this->state_ = PPPoSComponentState::CONNECTING;
}

void PPPoSComponent::status_callback(ppp_pcb *pcb, int err_code, void *ctx) {
  struct netif *pppif = &global_pppos_component->ppp_netif_;

  switch(err_code) {
    case PPPERR_NONE:
        global_pppos_component->state_ = PPPoSComponentState::CONNECTED;
        ESP_LOGI(TAG, "connected");
        ESP_LOGI(TAG, " our_ip %s", ipaddr_ntoa(&pppif->ip_addr));
        ESP_LOGI(TAG, " his_ip %s", ipaddr_ntoa(&pppif->gw));
        ESP_LOGI(TAG, " netmask %s", ipaddr_ntoa(&pppif->netmask));
        break;
    case PPPERR_PARAM:
        ESP_LOGE(TAG, "invalid parameter");
        global_pppos_component->state_ = PPPoSComponentState::STOPPED;
        global_pppos_component->mark_failed();
        break;
    case PPPERR_OPEN:
        ESP_LOGE(TAG, "unable to open PPP session");
        global_pppos_component->state_ = PPPoSComponentState::STOPPED;
        global_pppos_component->mark_failed();
        break;
    case PPPERR_DEVICE:
        ESP_LOGE(TAG, "Invalid I/O device for PPP");
        global_pppos_component->state_ = PPPoSComponentState::STOPPED;
        global_pppos_component->mark_failed();
        break;
    case PPPERR_ALLOC:
        ESP_LOGE(TAG, "unable to allocate resources");
        global_pppos_component->state_ = PPPoSComponentState::STOPPED;
        global_pppos_component->mark_failed();
        break;
    case PPPERR_USER:
        ESP_LOGE(TAG, "user interrupt");
        global_pppos_component->state_ = PPPoSComponentState::STOPPED;
        global_pppos_component->mark_failed();
        break;
    case PPPERR_CONNECT:
        ESP_LOGE(TAG, "connection lost");
        global_pppos_component->state_ = PPPoSComponentState::CONNECTING;
        break;
    case PPPERR_AUTHFAIL:
        ESP_LOGE(TAG, "Failed authentication challenge");
        global_pppos_component->state_ = PPPoSComponentState::STOPPED;
        global_pppos_component->mark_failed();
        break;
    case PPPERR_PROTOCOL:
        ESP_LOGE(TAG, "Failed to meet protocol");
        global_pppos_component->state_ = PPPoSComponentState::STOPPED;
        global_pppos_component->mark_failed();
        break;
    case PPPERR_PEERDEAD:
        ESP_LOGE(TAG, "Connection timeout");
        global_pppos_component->state_ = PPPoSComponentState::CONNECTING;
        break;
    case PPPERR_IDLETIMEOUT:
        ESP_LOGE(TAG, "Idle Timeout");
        global_pppos_component->state_ = PPPoSComponentState::CONNECTING;
        break;
    case PPPERR_CONNECTTIME:
        ESP_LOGE(TAG, "Max connect time reached");
        global_pppos_component->state_ = PPPoSComponentState::CONNECTING;
        break;
    case PPPERR_LOOPBACK:
        ESP_LOGE(TAG, "Loopback detected");
        global_pppos_component->state_ = PPPoSComponentState::CONNECTING;
        break;
    default:
        ESP_LOGE(TAG, "unknown error code: %d", err_code);
        global_pppos_component->state_ = PPPoSComponentState::STOPPED;
        global_pppos_component->mark_failed();
        break;
  }

  if(err_code == PPPERR_NONE) {
    return;
  }

  /* ppp_close() was previously called, don't reconnect */
  if (err_code == PPPERR_USER) {
    /* ppp_free(); -- can be called here */
    return;
  }

  // try to reconnect in 30s
  if(!global_pppos_component->is_failed()) {
    ppp_connect(pcb, 30);
  }
}

uint32_t PPPoSComponent::output_callback(ppp_pcb *pcb, const void *data, uint32_t len, void *ctx) {
  global_pppos_component->write_array((const uint8_t*)data, len);
  return len;
}

void PPPoSComponent::loop() {

}

void PPPoSComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PPPoS:");
  //this->dump_connect_params_();
}

float PPPoSComponent::get_setup_priority() const { return setup_priority::WIFI; }

bool PPPoSComponent::can_proceed() { return this->is_connected(); }

network::IPAddress PPPoSComponent::get_ip_address() {
  return network::IPAddress(this->ppp_netif_.ip_addr.addr);
}


bool PPPoSComponent::is_connected() {
  return this->state_ == PPPoSComponentState::CONNECTED;
}

void PPPoSComponent::set_manual_ip(const ManualIP &manual_ip) { this->manual_ip_ = manual_ip; }

}  // namespace pppos
}  // namespace esphome
