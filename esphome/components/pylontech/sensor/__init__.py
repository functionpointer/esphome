import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
import voluptuous as vol
from esphome.const import (
    CONF_VOLTAGE,
    CONF_CURRENT,
    CONF_TEMPERATURE,
    UNIT_VOLT,
    UNIT_AMPERE,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_BATTERY,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

from .. import (
    CONF_PYLONTECH_ID,
    PYLONTECH_COMPONENT_SCHEMA,
    CONF_COULOMB,
    CONF_PYLONTECH_BATTERY,
    CONF_TEMPERATURE_LOW,
    CONF_TEMPERATURE_HIGH,
    CONF_VOLTAGE_HIGH,
    CONF_VOLTAGE_LOW,
)

TYPES: dict[tuple[int, vol.Marker], cv.Schema] = {
    (0, cv.Optional(CONF_VOLTAGE)): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
    ),
    (1, cv.Optional(CONF_CURRENT)): sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_CURRENT,
    ),
    (2, cv.Optional(CONF_TEMPERATURE)): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    (3, cv.Optional(CONF_TEMPERATURE_LOW)): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    (4, cv.Optional(CONF_TEMPERATURE_HIGH)): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    (5, cv.Optional(CONF_VOLTAGE_LOW)): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    (6, cv.Optional(CONF_VOLTAGE_HIGH)): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    (11, cv.Optional(CONF_COULOMB)): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
    ),
}

CONFIG_SCHEMA = PYLONTECH_COMPONENT_SCHEMA.extend(
    {marker: schema for (_, marker), schema in TYPES.items()}
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PYLONTECH_ID])
    bat = config[CONF_PYLONTECH_BATTERY]

    for index, conf in TYPES.keys():
        if conf in config:
            sens = await sensor.new_sensor(config[conf])
            cg.add(paren.set_sensor(sens, bat, index))
