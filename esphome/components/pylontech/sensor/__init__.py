from collections import namedtuple
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
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
    CONF_BATTERY,
    CONF_TEMPERATURE_LOW,
    CONF_TEMPERATURE_HIGH,
    CONF_VOLTAGE_HIGH,
    CONF_VOLTAGE_LOW,
)

PylontechSensor = namedtuple("PylontechSensor", ["marker", "protocol_index", "schema"])

SENSOR_TYPES: list[PylontechSensor] = [
    PylontechSensor(
        cv.Optional(CONF_VOLTAGE),
        0,
        sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
        ),
    ),
    PylontechSensor(
        cv.Optional(CONF_CURRENT),
        1,
        sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_CURRENT,
        ),
    ),
    PylontechSensor(
        cv.Optional(CONF_TEMPERATURE),
        2,
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
        ),
    ),
    PylontechSensor(
        cv.Optional(CONF_TEMPERATURE_LOW),
        3,
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
        ),
    ),
    PylontechSensor(
        cv.Optional(CONF_TEMPERATURE_HIGH),
        4,
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
        ),
    ),
    PylontechSensor(
        cv.Optional(CONF_VOLTAGE_LOW),
        5,
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
        ),
    ),
    PylontechSensor(
        cv.Optional(CONF_VOLTAGE_HIGH),
        6,
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
        ),
    ),
    PylontechSensor(
        cv.Optional(CONF_COULOMB),
        11,
        sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_BATTERY,
        ),
    ),
]

CONFIG_SCHEMA = PYLONTECH_COMPONENT_SCHEMA.extend(
    {s.marker: s.schema for s in SENSOR_TYPES}
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PYLONTECH_ID])
    bat = config[CONF_BATTERY]

    q = cv.Optional(CONF_CURRENT)

    for s in SENSOR_TYPES:
        if s.marker in config:
            name = str(q.schema)
            sens = await sensor.new_sensor(config[s.marker])
            cg.add(paren.set_sensor(sens, bat, s.protocol_index))
