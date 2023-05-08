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

USB_CDC = "USB_CDC"
UART_COMPONENT = "UART_COMPONENT"
UART_TYPE = "uart_type"

pppos_ns = cg.esphome_ns.namespace("pppos")
PPPoSComponent = pppos_ns.class_("PPPoSComponent", cg.Component)


MANUAL_IP_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_STATIC_IP): cv.ipv4,
        cv.Required(CONF_GATEWAY): cv.ipv4,
        cv.Required(CONF_SUBNET): cv.ipv4,
        cv.Optional(CONF_DNS1, default="0.0.0.0"): cv.ipv4,
        cv.Optional(CONF_DNS2, default="0.0.0.0"): cv.ipv4,
    }
)


def _validate(config):
    if config[UART_TYPE] == USB_CDC:
        if not CORE.is_rp2040 or not CORE.using_arduino:
            raise cv.Invalid("USB_CDC is only supported on RP2040 using arduino")
    return config


BASE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PPPoSComponent),
        cv.Optional(CONF_USE_ADDRESS): cv.string_strict,
        cv.Optional(CONF_MANUAL_IP): MANUAL_IP_SCHEMA,
    }
)

CONFIG_SCHEMA = cv.All(
    cv.typed_schema(
        {
            UART_COMPONENT: BASE_SCHEMA.extend(uart.UART_DEVICE_SCHEMA),
            USB_CDC: BASE_SCHEMA.extend(
                cv.Schema(
                    {
                        cv.Optional(CONF_BAUD_RATE, default=115200): cv.positive_int,
                    }
                )
            ),
        },
        key=UART_TYPE,
        default_type="UART_COMPONENT",
        upper=True,
        space="_",
    ),
    _validate,
)


@coroutine_with_priority(60.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if config[UART_TYPE] == UART_COMPONENT:
        await uart.register_uart_device(var, config)
    else:
        cg.add_define("PPPOS_USE_CDC")
        cg.add(var.set_baud_rate(config[CONF_BAUD_RATE]))

    cg.add_define("USE_PPPOS")
    cg.add_build_flag(
        "-DPPP_SUPPORT=1"
    )  # can't use add_define, as this needs to be evaluated before lwipopts.h
    cg.add_build_flag("-DPPPOS_SUPPORT=1")
    cg.add_build_flag("-Dsys_jiffies=millis")  # ppp uses this for randomness

    cg.add_library("lwip-ppp-esphome", "0.0.2")
