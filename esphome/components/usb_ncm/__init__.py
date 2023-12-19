import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_DOMAIN,
    CONF_ID,
    CONF_MANUAL_IP,
    CONF_STATIC_IP,
    CONF_TYPE,
    CONF_USE_ADDRESS,
    CONF_GATEWAY,
    CONF_SUBNET,
    CONF_DNS1,
    CONF_DNS2,
    CONF_UART_ID,
    CONF_BAUD_RATE,
)
from esphome.core import CORE, coroutine_with_priority
from esphome.components import uart

AUTO_LOAD = ["network"]
CONFLICTS_WITH = ["wifi", "ethernet"]

usb_ncm_ns = cg.esphome_ns.namespace("usb_ncm")
USBNCMComponent = usb_ncm_ns.class_("USBNCMComponent", cg.Component)


MANUAL_IP_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_STATIC_IP): cv.ipv4,
        cv.Required(CONF_GATEWAY): cv.ipv4,
        cv.Required(CONF_SUBNET): cv.ipv4,
        cv.Optional(CONF_DNS1, default="0.0.0.0"): cv.ipv4,
        cv.Optional(CONF_DNS2, default="0.0.0.0"): cv.ipv4,
    }
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(USBNCMComponent),
            cv.Optional(CONF_MANUAL_IP): MANUAL_IP_SCHEMA,
            cv.Optional(CONF_DOMAIN, default=".local"): cv.domain_name,
            cv.Optional(CONF_USE_ADDRESS): cv.string_strict,
        }
    ).extend(cv.COMPONENT_SCHEMA),
)


@coroutine_with_priority(60.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add_define("USE_USB_NCM")
    # cg.add_build_flag(
    #    "-DPPP_SUPPORT=1"
    # )  # can't use add_define, as this needs to be evaluated before lwipopts.h
    # cg.add_build_flag("-DPPPOS_SUPPORT=1")
    # cg.add_build_flag("-Dsys_jiffies=millis")  # ppp uses this for randomness

    # cg.add_library("lwip-ppp-esphome", "0.0.2")
