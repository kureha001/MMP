# test_cpython_i2c_pwm.py
# MMP: CPython テスト（PWM→I2C の順に PCA9685 を使ってサーボをスイープ）

import time
from mmpC import MmpClient, CpythonAdapter  # ※環境によっては CpyAdapter 名で提供されている場合あり

# ===== テスト対象の設定 =====
CHANNEL_SERVO = 0       # テストする PCA9685 のチャネル
SERVO_MIN     = 150     # PCA9685 12bitの生値（例: 150）
SERVO_MAX     = 600     # 同上（例: 600）
SERVO_MID     = (SERVO_MIN + SERVO_MAX) // 2
STEPS         = 80
STEP_DELAY_S  = 0.02

# I2C：PCA9685 のアドレス（MMP 搭載分の一般的既定は 0x40）
PCA9685_ADDR  = 0x40

def connect():
    print("<< MMP ライブラリ for CPython >>\n")
    print("接続中...", end="", flush=True)
    mmp = MmpClient(CpythonAdapter())  # 自動ポート＋既定候補で自動接続
    if not mmp.ConnectAutoBaud():
        print("失敗\n")
        raise RuntimeError("MMP への接続に失敗しました。")
    print("成功")
    print(f"自動検出 {mmp.Settings.BaudRate} bps\n")
    return mmp

# ===== 0) 基本情報 =====
def run_info(mmp: MmpClient):
    print("０.システム情報")
    print("　・バージョン  :", mmp.Info.Version())
    print("　・PCA9685 [0] : 0x%04X" % mmp.Info.Dev.Pwm(0))
    print("　・DFPlayer[1] : 0x%04X" % mmp.Info.Dev.Audio(1))
    print("　[終了]\n")

# ===== 1) PWM API でサーボスイープ =====
def run_pwm_sweep(mmp: MmpClient):
    print("１.ＰＷＭ（PCA9685）でサーボスイープ")
    print("　・初期位置 → MID")
    mmp.Pwm.Out(CHANNEL_SERVO, SERVO_MID)
    time.sleep(0.3)

    print("　・正転,加速")
    for i in range(STEPS + 1):
        v = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        mmp.Pwm.Out(CHANNEL_SERVO, v)
        time.sleep(STEP_DELAY_S)

    print("　・逆転,減速")
    for i in range(STEPS, -1, -1):
        v = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        mmp.Pwm.Out(CHANNEL_SERVO, v)
        time.sleep(STEP_DELAY_S)

    print("　・初期位置 → MID")
    mmp.Pwm.Out(CHANNEL_SERVO, SERVO_MID)
    print("　[終了]\n")

# ===== 2) I2C API でサーボスイープ（PCA9685 レジスタ直書き） =====
#   LEDn の ON=0 / OFF=val でデューティ設定
#   LED0_ON_L=0x06 からチャネル毎に 4 レジスタ
#     base = 0x06 + 4*ch
#     [base+0]=ON_L, [base+1]=ON_H, [base+2]=OFF_L, [base+3]=OFF_H
def _pca9685_set_pwm(mmp: MmpClient, ch: int, value_0_4095: int) -> bool:
    base = 0x06 + 4 * ch
    on_l, on_h = 0x00, 0x00
    off = max(0, min(4095, int(value_0_4095)))
    off_l = (off & 0xFF)
    off_h = ((off >> 8) & 0x0F)  # 上位4bitのみ

    ok  = mmp.I2c.Write(PCA9685_ADDR, base + 0, on_l)   # ON_L
    ok &= mmp.I2c.Write(PCA9685_ADDR, base + 1, on_h)   # ON_H
    ok &= mmp.I2c.Write(PCA9685_ADDR, base + 2, off_l)  # OFF_L
    ok &= mmp.I2c.Write(PCA9685_ADDR, base + 3, off_h)  # OFF_H
    return ok

def _pca9685_get_pwm(mmp: MmpClient, ch: int) -> int:
    base = 0x06 + 4 * ch
    off_l = mmp.I2c.Read(PCA9685_ADDR, base + 2)
    off_h = mmp.I2c.Read(PCA9685_ADDR, base + 3) & 0x0F
    if off_l < 0 or off_h < 0:
        return -1
    return (off_h << 8) | off_l

def run_i2c_sweep(mmp: MmpClient):
    print("２.I2C（PCA9685 レジスタ直書き）でサーボスイープ")

    # 初期位置
    print("　・初期位置 → MID")
    if not _pca9685_set_pwm(mmp, CHANNEL_SERVO, SERVO_MID):
        print("　　★I2C書き込み失敗（初期位置）")
        print("　[中断]\n")
        return
    time.sleep(0.3)

    # 正転・加速
    print("　・正転,加速")
    for i in range(STEPS + 1):
        v = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        if not _pca9685_set_pwm(mmp, CHANNEL_SERVO, v):
            print("　　★I2C書き込み失敗（正転）")
            break
        time.sleep(STEP_DELAY_S)

    # 逆転・減速
    print("　・逆転,減速")
    for i in range(STEPS, -1, -1):
        v = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        if not _pca9685_set_pwm(mmp, CHANNEL_SERVO, v):
            print("　　★I2C書き込み失敗（逆転）")
            break
        time.sleep(STEP_DELAY_S)

    # 読み出し確認（最後に設定した値）
    last = _pca9685_get_pwm(mmp, CHANNEL_SERVO)
    print(f"　・現在の OFF 値 = {last}")

    # 初期位置へ
    print("　・初期位置 → MID")
    _pca9685_set_pwm(mmp, CHANNEL_SERVO, SERVO_MID)
    print("　[終了]\n")

def main():
    mmp = connect()

    print("＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n")
    run_info(mmp)         # 情報系
    run_pwm_sweep(mmp)    # PWM API でスイープ
    run_i2c_sweep(mmp)    # I2C API でスイープ（レジスタ直書き）
    print("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝")

    mmp.Close()

if __name__ == "__main__":
    main()
