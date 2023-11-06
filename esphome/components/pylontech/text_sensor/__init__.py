import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from .. import (
    CONF_PYLONTECH_ID,
    PYLONTECH_COMPONENT_SCHEMA,
    PYLONTECH_COMPONENT_FINAL_VALIDATE,
    CONF_BATTERY,
    CONF_BASE_STATE,
    CONF_VOLTAGE_STATE,
    CONF_CURRENT_STATE,
    CONF_TEMPERATURE_STATE,
)

MARKERS: list[str] = [
    CONF_BASE_STATE,
    CONF_VOLTAGE_STATE,
    CONF_CURRENT_STATE,
    CONF_TEMPERATURE_STATE,
]

CONFIG_SCHEMA = PYLONTECH_COMPONENT_SCHEMA.extend(
    {cv.Optional(marker): text_sensor.text_sensor_schema() for marker in MARKERS}
)

FINAL_VALIDATE_SCHEMA = PYLONTECH_COMPONENT_FINAL_VALIDATE


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PYLONTECH_ID])
    bat: int = config[CONF_BATTERY]

    for marker in MARKERS:
        if marker in config:
            conf = config[marker]
            var = await text_sensor.new_text_sensor(conf)
            cg.add(getattr(paren, f"set_{marker}")(var, bat))
