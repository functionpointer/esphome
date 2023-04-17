#include "pppos_component.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"
#include "esphome/core/application.h"

#include <pico/cyw43_arch.h>
//#include "lwip/init.h"


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
  //lwip_init();
  cyw43_arch_init();
#endif

  this->ppp_control_block_ = pppos_create(&this->ppp_netif_, PPPoSComponent::output_callback, PPPoSComponent::status_callback, nullptr);
  if(this->ppp_control_block_ == nullptr) {
    ESP_LOGE(TAG, "failed to create ppp control block");
    this->mark_failed();
    return;
  }
  //we are the PPP client
  ppp_set_default(this->ppp_control_block_);
  ppp_set_usepeerdns(this->ppp_control_block_, 1);

  if(ppp_connect(this->ppp_control_block_, 10) != ERR_OK) {
    ESP_LOGE(TAG, "pppos couldnt start connecting");
    this->mark_failed();
    return;
  }
  this->state_ = PPPoSComponentState::CONNECTING;
  ESP_LOGI(TAG, "pppos is connecting");
}

void PPPoSComponent::status_callback(ppp_pcb *pcb, int err_code, void *ctx) {
  struct netif *pppif = ppp_netif(pcb);

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
  ESP_LOGV(TAG, "cb: sending %d bytes", len);
  /*for(int i=0;i<len;i++) {
    global_pppos_component->tx_queue_.push(((const uint8_t* )data)[len]);
  }*/
  global_pppos_component->write_array((const uint8_t*)data, len);
  return len;
}

void PPPoSComponent::loop() {
  sys_check_timeouts();

  static uint32_t nextmsg = 0;
  if(nextmsg<millis()) {
    ESP_LOGI(TAG, "state is %d", this->state_);
    nextmsg = millis()+2000;
  }

  if(!this->tx_queue_.empty()) {
    ESP_LOGV(TAG, "sending %d bytes", this->tx_queue_.size());
    while(!this->tx_queue_.empty()) {
        this->write(this->tx_queue_.front());
        this->tx_queue_.pop();
    }
  }

  if(this->available() > 0) {
    size_t size = this->available();
    uint8_t data[size];
    if(!this->read_array(data, size)) {
        ESP_LOGE(TAG, "error read_array");
    }
    ESP_LOGV(TAG, "receiving %d bytes", size);
    pppos_input(this->ppp_control_block_, data, size);
  }

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
