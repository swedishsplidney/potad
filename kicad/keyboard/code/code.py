print("Starting")
import board
import busio

from kmk.kmk_keyboard import KMKKeyboard
from kmk.keys import KC
from kmk.scanners import DiodeOrientation
from kmk.modules.encoder import EncoderHandler
from kmk.extensions.media_keys import MediaKeys
from kmk.modules.macros import Macros, Delay, Tap, Press, Release
from board import SCL, SDA
from adafruit_ssd1306 import SSD1306_I2C

# i2c = busio.I2C(SCL, SDA)
# 
# display = SSD1306_I2C(128,32, i2c)
# 
# display.fill(0)
# display.show()
# 
# # Set a pixel in the origin 0,0 position.
# display.pixel(0, 0, 1)
# # Set a pixel in the middle 64, 16 position.
# display.pixel(64, 16, 1)
# # Set a pixel in the opposite 127, 31 position.
# display.pixel(127, 31, 1)
# 
# display.text("hi", 0, 0, 0)
# display.show()

keyboard = KMKKeyboard()
encoder_handler = EncoderHandler()
macros = Macros()
media = MediaKeys()
keyboard.modules.append(macros)
keyboard.modules.append(encoder_handler)
keyboard.extensions.append(media)
# Matrix pins
COL1 = board.D0
COL2 = board.D1
COL3 = board.D2
ROW1 = board.D7
ROW2 = board.D8
ROW3 = board.D10
# rotary encoder pins
ROTA = board.D3
ROTB = board.D9
PUSHBUTTON = None
# Matrix settings
keyboard.row_pins = (COL1, COL2, COL3)
keyboard.col_pins = (ROW1, ROW2, ROW3)
keyboard.diode_orientation = DiodeOrientation.ROW2COL
# Encoder settingsadgbe7890987iadgbe7890987iiii7890987eeeeeeeeeee
encoder_handler.pins = ((ROTA, ROTB, PUSHBUTTON, True),)
encoder_handler.map = [
    ((KC.VOLD, KC.VOLU, KC.MUTE),)
]

# 

PAWD = KC.MACRO(
    "7",
    Delay(300),
    "890987"
)

CTV = KC.MACRO(
    Press(KC.LCTL),
    Tap(KC.C),
    Delay(100),
    Tap(KC.T),
    Delay(100),
    Tap(KC.V),
    Release(KC.LCTL),
)

WK = KC.MACRO(
    Tap(KC.LWIN),
    "WaniKani",
    Tap(KC.ENTER)
)

JLC = KC.MACRO(
    Tap(KC.LCTL(KC.T)),
    "https://jlcpcb.com/parts/",
    Tap(KC.ENTER)
)

JLC_TO_EDA = KC.MACRO(
    Tap(KC.LSHIFT(KC.F)),
    Delay(300),
    Tap(KC.LCTRL(KC.V)),
    Tap(KC.ENTER)
)

ENTER = KC.ENTER

keyboard.keymap = [
    [
        KC.LSHIFT(KC.B)		, KC.N2	, KC.LALT(KC.T)	, JLC_TO_EDA, PAWD		,
        KC.LSHIFT(KC.M)		, KC.N3	, KC.LALT(KC.B)	, JLC		
    ]
]
if __name__ == "__main__":
    keyboard.go()


