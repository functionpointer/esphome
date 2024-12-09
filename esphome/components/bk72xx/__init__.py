# This file was auto-generated by libretiny/generate_components.py
# Do not modify its contents.
# For custom pin validators, put validate_pin() or validate_usage()
# in gpio.py file in this directory.
# For changing schema/pin schema, put COMPONENT_SCHEMA or COMPONENT_PIN_SCHEMA
# in schema.py file in this directory.

from esphome import pins
from esphome.components import libretiny
from esphome.components.libretiny.const import (
    COMPONENT_BK72XX,
    KEY_COMPONENT_DATA,
    KEY_LIBRETINY,
    LibreTinyComponent,
)
from esphome.core import CORE

from .boards import BK72XX_BOARD_PINS, BK72XX_BOARDS

CODEOWNERS = ["@kuba2k2"]
AUTO_LOAD = ["libretiny"]

COMPONENT_DATA = LibreTinyComponent(
    name=COMPONENT_BK72XX,
    boards=BK72XX_BOARDS,
    board_pins=BK72XX_BOARD_PINS,
    pin_validation=None,
    usage_validation=None,
)


def _set_core_data(config):
    CORE.data[KEY_LIBRETINY] = {}
    CORE.data[KEY_LIBRETINY][KEY_COMPONENT_DATA] = COMPONENT_DATA
    return config


CONFIG_SCHEMA = libretiny.BASE_SCHEMA

PIN_SCHEMA = libretiny.gpio.BASE_PIN_SCHEMA

CONFIG_SCHEMA.prepend_extra(_set_core_data)


async def to_code(config):
    return await libretiny.component_to_code(config)


@pins.PIN_SCHEMA_REGISTRY.register("bk72xx", PIN_SCHEMA)
async def pin_to_code(config):
    return await libretiny.gpio.component_pin_to_code(config)
