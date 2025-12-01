import esphome.codegen as cg
from esphome.components.network import IPAddress
import esphome.config_validation as cv
from esphome.const import (
    CONF_DNS1,
    CONF_DNS2,
    CONF_DOMAIN,
    CONF_GATEWAY,
    CONF_ID,
    CONF_MANUAL_IP,
    CONF_STATIC_IP,
    CONF_SUBNET,
    CONF_USE_ADDRESS,
)
from esphome.core import CORE
from esphome.coroutine import coroutine_with_priority

CONFLICTS_WITH = ["wifi", "ethernet"]
AUTO_LOAD = ["network"]

usb_ncm_ns = cg.esphome_ns.namespace("usb_ncm")
ManualIP = usb_ncm_ns.struct("ManualIP")

MANUAL_IP_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_STATIC_IP): cv.ipv4address,
        cv.Required(CONF_GATEWAY): cv.ipv4address,
        cv.Required(CONF_SUBNET): cv.ipv4address,
        cv.Optional(CONF_DNS1, default="0.0.0.0"): cv.ipv4address,
        cv.Optional(CONF_DNS2, default="0.0.0.0"): cv.ipv4address,
    }
)

USBNCMComponent = usb_ncm_ns.class_("USBNCMComponent", cg.Component)


def _validate(config):
    if CONF_USE_ADDRESS not in config:
        if CONF_MANUAL_IP in config:
            use_address = str(config[CONF_MANUAL_IP][CONF_STATIC_IP])
        else:
            use_address = CORE.name + config[CONF_DOMAIN]
        config[CONF_USE_ADDRESS] = use_address
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(USBNCMComponent),
            cv.Optional(CONF_MANUAL_IP): MANUAL_IP_SCHEMA,
            cv.Optional(CONF_DOMAIN, default=".local"): cv.domain_name,
            cv.Optional(CONF_USE_ADDRESS): cv.string_strict,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate,
)


def manual_ip(config):
    return cg.StructInitializer(
        ManualIP,
        ("static_ip", IPAddress(*config[CONF_STATIC_IP].packed)),
        ("gateway", IPAddress(*config[CONF_GATEWAY].packed)),
        ("subnet", IPAddress(*config[CONF_SUBNET].packed)),
        ("dns1", IPAddress(*config[CONF_DNS1].packed)),
        ("dns2", IPAddress(*config[CONF_DNS2].packed)),
    )


@coroutine_with_priority(60.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_use_address(config[CONF_USE_ADDRESS]))

    if CONF_MANUAL_IP in config:
        cg.add(var.set_manual_ip(manual_ip(config[CONF_MANUAL_IP])))

    cg.add_define("USE_USB_NCM")
