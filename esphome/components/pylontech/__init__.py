import esphome.codegen as cg
from esphome.components import uart

CODEOWNERS = ["@functionpointer"]
DEPENDENCIES = ["uart"]

pylontech_ns = cg.esphome_ns.namespace("pylontech")
PylontechComponent = pylontech_ns.class_(
    "PylontechComponent", cg.PollingComponent, uart.UARTDevice
)
