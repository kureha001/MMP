# XIAO Expansion Board + M5Stack CardKB (I2C) demo
# - OLED: SSD1306 128x64 @ 0x3C (I2C1: SDA=GPIO6(D4), SCL=GPIO7(D5))
# - CardKB: I2C keyboard @ 0x5F
# - Buzzer: PWM on GPIO29 (A3/D3)

from machine import Pin, I2C, PWM
import time
import ssd1306

# ==== Pins / I2C ====
i2c = I2C(1, sda=Pin(6), scl=Pin(7), freq=400_000)  # D4/D5 = GPIO6/7

# ==== OLED ====
oled = ssd1306.SSD1306_I2C(128, 64, i2c, addr=0x3C)

# ==== Buzzer (passive) ====
bz = PWM(Pin(29))          # A3/D3 = GPIO29
bz.duty_u16(0)

def beep(freq=2000, ms=50):
    bz.freq(freq)
    bz.duty_u16(32768)     # 50%
    time.sleep_ms(ms)
    bz.duty_u16(0)

def beep_ok():   beep(2000, 40)
def beep_bs():   beep(1500, 60)
def beep_enter(): 
    beep(1600, 60); time.sleep_ms(40); beep(2200, 80)

# ==== CardKB ====
CARDKB_ADDR = 0x5F

def cardkb_read():
    """1バイト読む。0なら未入力。例外時はNone。"""
    try:
        b = i2c.readfrom(CARDKB_ADDR, 1)
        if not b: 
            return None
        val = b[0]
        return val if val != 0 else None
    except OSError:
        return None

# ==== Text buffer (16桁×最大8行表示) ====
COLS, ROWS = 16, 8
lines = [""]  # 末尾が現在行

def render():
    oled.fill(0)
    # 下8行分を描画（足りない分は上を空行）
    last = lines[-ROWS:]
    pad = [""] * (ROWS - len(last)) + last
    y = 0
    for s in pad:
        oled.text(s[:COLS], 0, y)
        y += 8
    oled.show()

def put_char(ch):
    global lines
    cur = lines[-1]
    if ch == "\n":
        lines.append("")
        if len(lines) > 64:   # 無限肥大化しない程度の履歴
            lines = lines[-64:]
        beep_enter()
        render()
        return
    if len(cur) < COLS:
        lines[-1] = cur + ch
    else:
        lines.append(ch)
    beep_ok()
    render()

def backspace():
    global lines
    if not lines:
        return
    cur = lines[-1]
    if cur:
        lines[-1] = cur[:-1]
    else:
        if len(lines) > 1:
            lines.pop()
            lines[-1] = lines[-1][:-1]
    beep_bs()
    render()

# ==== 初期画面 ====
oled.fill(0)
oled.text("CardKB + OLED", 0, 0)
oled.text("Type keys...", 0, 16)
oled.text("(Enter, BS OK)", 0, 24)
oled.text("Addr: 3C/5F", 0, 40)
oled.show()
time.sleep_ms(5000)
render()

# ==== メインループ ====
last_val = None
last_t   = 0

while True:
    v = cardkb_read()
    if v is None:
        time.sleep_ms(5)
        continue

    now = time.ticks_ms()
    # 同じコードの連続読み出しを超短周期では無視（簡易リピート抑制）
    if v == last_val and time.ticks_diff(now, last_t) < 30:
        continue
    last_val, last_t = v, now

    # 特殊キー処理（Backspace/Enter）
    if v in (8, 127):        # BS or DEL をBackspace扱い
        backspace()
        continue
    if v in (10, 13):        # LF/CR を改行扱い
        put_char("\n")
        continue

    # 表示可能ASCIIのみ
    if 32 <= v <= 126:
        put_char(chr(v))
    else:
        # 非表示コードは小さくビープだけ
        beep(1200, 20)
