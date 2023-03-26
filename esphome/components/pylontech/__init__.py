import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@functionpointer"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

CONF_PYLONTECH_ID = "pylontech_id"
CONF_PYLONTECH_BATTERY = "battery"
CONF_COULOMB = "coulomb"
CONF_TEMPERATURE_LOW = "temperature_low"
CONF_TEMPERATURE_HIGH = "temperature_high"
CONF_VOLTAGE_LOW = "voltage_low"
CONF_VOLTAGE_HIGH = "voltage_high"

pylontech_ns = cg.esphome_ns.namespace("pylontech")
PylontechComponent = pylontech_ns.class_(
    "PylontechComponent", cg.PollingComponent, uart.UARTDevice
)

PYLONTECH_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PYLONTECH_ID): cv.use_id(PylontechComponent),
        cv.Required(CONF_PYLONTECH_BATTERY): cv.int_range(1, 6),
    }
)

CONFIG_SCHEMA = cv.All(
    cv.Schema({cv.GenerateID(): cv.declare_id(PylontechComponent)})
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    # todo find and define NUM_BATTERIES
