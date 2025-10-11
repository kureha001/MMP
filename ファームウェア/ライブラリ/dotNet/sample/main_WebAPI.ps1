# -*- coding: utf-8 -*-
# ============================================================
# MMP WebAPI Test (PowerShell 5.1 safe, ASCII-only messages)
# ============================================================

param(
  [string]$ApiHost = "192.168.2.124",
  [int]$ApiPort = 8080,
  [switch]$ShowDetail = $false
)

# Unicode 出力（Windows PowerShell 5.1 対策）
try {
  $OutputEncoding = [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()
} catch {}

# -----------------------------
# Helpers
# -----------------------------
function Write-Title([string]$s) {
  Write-Host ""
  Write-Host $s
}

function Write-Json($obj) {
  $obj | ConvertTo-Json -Depth 5
}

# クエリはここで生成（& 直書き禁止）
function Build-Path([string]$Path, [hashtable]$Query = $null) {
  if (-not $Query -or $Query.Keys.Count -eq 0) { return $Path }
  $pairs = @()
  foreach ($k in $Query.Keys) {
    $v = $Query[$k]
    $vk = [uri]::EscapeDataString([string]$k)
    $vv = [uri]::EscapeDataString([string]$v)
    $pairs += "$vk=$vv"
  }
  return "$Path?$(($pairs -join '&'))"
}

# -----------------------------
# HTTP GET
# -----------------------------
function Invoke-ApiGet([string]$Path, [hashtable]$Query = $null) {
  $p = Build-Path $Path $Query
  $url = "http://${ApiHost}:$ApiPort$p"

  try {
    $res   = Invoke-WebRequest -UseBasicParsing -Uri $url -Method GET -TimeoutSec 15
    $text  = $res.Content
    $json  = $null
    try { $json = $text | ConvertFrom-Json } catch {}

    if ($ShowDetail) {
      Write-Host ("GET {0}" -f $url)
      Write-Host ("HTTP {0}" -f $res.StatusCode)
      if ($json) {
        Write-Json $json
      } else {
        if ($text -and $text.Length -gt 0) { Write-Host $text } else { Write-Host "(empty)" }
      }
      Write-Host ""
    }

    return [pscustomobject]@{
      ok     = $true
      status = $res.StatusCode
      text   = $text
      json   = $json
      error  = $null
    }
  }
  catch {
    if ($ShowDetail) {
      Write-Host ("GET {0}" -f $url)
      Write-Host ("ERROR: {0}" -f $_.Exception.Message)
      Write-Host ""
    }
    return [pscustomobject]@{
      ok     = $false
      status = 0
      text   = ""
      json   = $null
      error  = $_.Exception.Message
    }
  }
}

function TF($b) { if ($b) { "True" } else { "False" } }

# -----------------------------
# 1) Analog
# -----------------------------
function Run-Analog {
  Write-Title "1. Analog Input (HC4067: JoyPad1,2)"

  $r = Invoke-ApiGet '/analog/configure' @{ start = 2; count = 4; chTtl = 2; devTtl = 4 }
  $ok = ($r.json -and $r.json.ok)
  Write-Host ("  - Range: [2,4] : {0}" -f (TF $ok))
  if (-not $ok) { Write-Host "  [Interrupted]"; return }

  $r = Invoke-ApiGet '/analog/update'
  $ok = ($r.json -and $r.json.ok)
  Write-Host ("  - Update buffer : {0}" -f (TF $ok))
  if (-not $ok) { Write-Host "  [Interrupted]"; return }

  Write-Host "  - Read buffer"
  for ($x=0; $x -le 1; $x++) {
    Write-Host ("    JoyPad[{0}]" -f ($x+1))
    for ($y=0; $y -le 3; $y++) {
      $r = Invoke-ApiGet '/analog/read' @{ ch = $x; dev = $y }
      $v = if ($r.json -and $r.json.PSObject.Properties.Name -contains 'value') { $r.json.value } else { 'NaN' }
      Write-Host ("      <- [{0}] = {1}" -f $y, $v)
    }
  }
  Write-Host "  [Done]"
}

# -----------------------------
# 2) Digital
# -----------------------------
function Run-Digital {
  Write-Title "2. Digital I/O (GPIO)"
  Write-Host "  - Input"
  foreach ($pin in 2,6,7) {
    $r = Invoke-ApiGet '/digital/in' @{ id = $pin }
    $val = if ($r.json -and $r.json.PSObject.Properties.Name -contains 'value') { $r.json.value } else { 1 }
    $state = if ($val -eq 0) { "ON" } else { "OFF" }
    Write-Host ("    <- [{0}] = {1}" -f $pin, $state)
  }

  Write-Host "  - Output [3]"
  for ($i=0; $i -lt 6; $i++) {
    $r1 = Invoke-ApiGet '/digital/out' @{ id = 3; val = 1 }
    Write-Host ("    -> HIGH : {0}" -f (TF ($r1.json -and $r1.json.ok)))
    Start-Sleep -Milliseconds 500

    $r0 = Invoke-ApiGet '/digital/out' @{ id = 3; val = 0 }
    Write-Host ("    -> LOW  : {0}" -f (TF ($r0.json -and $r0.json.ok)))
    Start-Sleep -Milliseconds 500
  }
  Write-Host "  [Done]"
}

# -----------------------------
# 3) MP3 Playlist (DFPlayer)
# -----------------------------
function Run-Mp3Playlist {
  Write-Title "3. MP3 Playlist (DFPlayer)"

  $r = Invoke-ApiGet '/audio/volume' @{ dev = 1; value = 20 }
  Write-Host ("  - Volume -> 20 : {0}" -f (TF ($r.json -and $r.json.ok)))

  $r = Invoke-ApiGet '/audio/play/setLoop' @{ dev = 1; mode = 0 }
  Write-Host ("  - Loop -> OFF  : {0}" -f (TF ($r.json -and $r.json.ok)))

  Write-Host "  - Play folder 1, track 1..3"
  for ($track=1; $track -le 3; $track++) {
    $r1 = Invoke-ApiGet '/audio/play/start' @{ dev = 1; dir = 1; file = $track }
    $r2 = Invoke-ApiGet '/audio/read/state' @{ dev = 1 }
    $st = if ($r2.json) { $r2.json.state } else { 'NaN' }
    Write-Host ("    -> F=1,T={0} : {1} : state={2}" -f $track, (TF ($r1.json -and $r1.json.ok)), $st)
    Start-Sleep -Seconds 3
  }

  $r  = Invoke-ApiGet '/audio/play/stop'  @{ dev = 1 }
  $r3 = Invoke-ApiGet '/audio/read/state' @{ dev = 1 }
  $st = if ($r3.json) { $r3.json.state } else { 'NaN' }
  Write-Host ("  - Stop : {0} : state={1}" -f (TF ($r.json -and $r.json.ok)), $st)

  $r = Invoke-ApiGet '/audio/play/start' @{ dev = 1; dir = 2; file = 102 }
  Write-Host ("  - Play F=2,T=102 : {0}" -f (TF ($r.json -and $r.json.ok)))

  $r  = Invoke-ApiGet '/audio/play/setLoop' @{ dev = 1; mode = 1 }
  $r3 = Invoke-ApiGet '/audio/read/state'   @{ dev = 1 }
  $st = if ($r3.json) { $r3.json.state } else { 'NaN' }
  Write-Host ("  - Loop -> ON : {0} : state={1}" -f (TF ($r.json -and $r.json.ok)), $st)
  Start-Sleep -Seconds 10

  $r  = Invoke-ApiGet '/audio/play/stop'  @{ dev = 1 }
  $r3 = Invoke-ApiGet '/audio/read/state' @{ dev = 1 }
  $st = if ($r3.json) { $r3.json.state } else { 'NaN' }
  Write-Host ("  - Stop : {0} : state={1}" -f (TF ($r.json -and $r.json.ok)), $st)
  Write-Host "  [Done]"
}

# -----------------------------
# 4) MP3 Control (DFPlayer)
# -----------------------------
function Run-Mp3Control {
  Write-Title "4. MP3 Control (DFPlayer)"

  $r = Invoke-ApiGet '/audio/volume' @{ dev = 1; value = 20 }
  Write-Host ("  - Volume -> 20 : {0}" -f (TF ($r.json -and $r.json.ok)))

  $r = Invoke-ApiGet '/audio/play/start' @{ dev = 1; dir = 4; file = 1 }
  Write-Host ("  - Play F=4,T=1 : {0}" -f (TF ($r.json -and $r.json.ok)))

  $r = Invoke-ApiGet '/audio/play/setLoop' @{ dev = 1; mode = 0 }
  Write-Host ("  - Loop -> OFF : {0}" -f (TF ($r.json -and $r.json.ok)))

  Write-Host "  - Read props"
  $st = Invoke-ApiGet '/audio/read/state'       @{ dev = 1 }
  $vv = Invoke-ApiGet '/audio/read/volume'      @{ dev = 1 }
  $eq = Invoke-ApiGet '/audio/read/eq'          @{ dev = 1 }
  $fc = Invoke-ApiGet '/audio/read/fileCounts'  @{ dev = 1 }
  $fn = Invoke-ApiGet '/audio/read/fileNumber'  @{ dev = 1 }
  Write-Host ("    state       = {0}" -f ($(if ($st.json) { $st.json.state } else { 'NaN' })))
  Write-Host ("    volume      = {0}" -f ($(if ($vv.json) { $vv.json.volume } else { 'NaN' })))
  Write-Host ("    eq          = {0}" -f ($(if ($eq.json) { $eq.json.eq } else { 'NaN' })))
  Write-Host ("    fileCounts  = {0}" -f ($(if ($fc.json) { $fc.json.fileCounts } else { 'NaN' })))
  Write-Host ("    fileNumber  = {0}" -f ($(if ($fn.json) { $fn.json.fileNumber } else { 'NaN' })))

  $r  = Invoke-ApiGet '/audio/play/pause'  @{ dev = 1 }
  $st = Invoke-ApiGet '/audio/read/state'  @{ dev = 1 }
  Write-Host ("  - Pause : {0} : state={1}" -f (TF ($r.json -and $r.json.ok)), ($(if ($st.json) { $st.json.state } else { 'NaN' })))
  Start-Sleep -Seconds 2

  $r  = Invoke-ApiGet '/audio/play/resume' @{ dev = 1 }
  $st = Invoke-ApiGet '/audio/read/state'  @{ dev = 1 }
  Write-Host ("  - Resume: {0} : state={1}" -f (TF ($r.json -and $r.json.ok)), ($(if ($st.json) { $st.json.state } else { 'NaN' })))

  Write-Host "  - Equalizer"
  for ($m=0; $m -le 5; $m++) {
    $r = Invoke-ApiGet '/audio/setEq' @{ dev = 1; mode = $m }
    Write-Host ("    -> {0} : {1}" -f $m, (TF ($r.json -and $r.json.ok)))
    Start-Sleep -Seconds 3
  }

  Write-Host "  - Volume sweep"
  for ($v=0; $v -le 30; $v+=5) {
    $r = Invoke-ApiGet '/audio/volume' @{ dev = 1; value = $v }
    Write-Host ("    -> {0} : {1}" -f $v, (TF ($r.json -and $r.json.ok)))
    Start-Sleep -Seconds 1
  }

  $r  = Invoke-ApiGet '/audio/play/stop' @{ dev = 1 }
  $st = Invoke-ApiGet '/audio/read/state' @{ dev = 1 }
  Write-Host ("  - Stop : {0} : state={1}" -f (TF ($r.json -and $r.json.ok)), ($(if ($st.json) { $st.json.state } else { 'NaN' })))
  Write-Host "  [Done]"
}

# -----------------------------
# 5) PWM / 6) I2C (PCA9685)
# -----------------------------
function Run-I2C([int]$addr, [int]$reg, [int]$val) {
  Invoke-ApiGet '/i2c/write' @{ addr = $addr; reg = $reg; val = $val } | Out-Null
}

function Run-Pwm([bool]$UsePwm = $true) {
  $title = if ($UsePwm) { "5. PWM" } else { "6. I2C" }
  Write-Title "$title (PCA9685: servo 180deg & continuous)"

  $SERVO_MIN = 150
  $SERVO_MAX = 600
  $SERVO_MID = [int](($SERVO_MIN + $SERVO_MAX) / 2)
  $OffsetMax360 = 60
  $STEPS = 80
  $STEP = 8
  $CH_180 = 0
  $CH_360 = 15
  $PCA_ADDR_DEC = 64  # 0x40

  function Set-ServoTicks([int]$ch, [int]$ticks) {
    $base = 0x06 + (4 * $ch)
    Run-I2C -addr $PCA_ADDR_DEC -reg ($base + 2) -val ($ticks -band 0xFF)
    Run-I2C -addr $PCA_ADDR_DEC -reg ($base + 3) -val (($ticks -shr 8) -band 0x0F)
  }

  Write-Host "  - Init position"
  if ($UsePwm) {
    Invoke-ApiGet '/pwm/out' @{ ch = $CH_180; val = $SERVO_MID } | Out-Null
    Invoke-ApiGet '/pwm/out' @{ ch = $CH_360; val = $SERVO_MID } | Out-Null
  } else {
    Set-ServoTicks -ch $CH_180 -ticks $SERVO_MID
    Set-ServoTicks -ch $CH_360 -ticks $SERVO_MID
  }
  Start-Sleep -Milliseconds 300

  Write-Host "  - Forward/Accel"
  for ($i=0; $i -le $STEPS; $i += $STEP) {
    $pwm180 = $SERVO_MIN + [int](($SERVO_MAX - $SERVO_MIN) * $i / $STEPS)
    $pwm360 = $SERVO_MID + [int]($OffsetMax360 * $i / $STEPS)
    if ($UsePwm) {
      Invoke-ApiGet '/pwm/out' @{ ch = $CH_180; val = $pwm180 } | Out-Null
      Invoke-ApiGet '/pwm/out' @{ ch = $CH_360; val = $pwm360 } | Out-Null
    } else {
      Set-ServoTicks -ch $CH_180 -ticks $pwm180
      Set-ServoTicks -ch $CH_360 -ticks $pwm360
    }
  }

  Write-Host "  - Reverse/Decel"
  for ($i=$STEPS; $i -ge 0; $i -= $STEP) {
    $pwm180 = $SERVO_MIN + [int](($SERVO_MAX - $SERVO_MIN) * $i / $STEPS)
    $pwm360 = $SERVO_MID + [int]($OffsetMax360 * $i / $STEPS)
    if ($UsePwm) {
      Invoke-ApiGet '/pwm/out' @{ ch = $CH_180; val = $pwm180 } | Out-Null
      Invoke-ApiGet '/pwm/out' @{ ch = $CH_360; val = $pwm360 } | Out-Null
    } else {
      Set-ServoTicks -ch $CH_180 -ticks $pwm180
      Set-ServoTicks -ch $CH_360 -ticks $pwm360
    }
  }

  Write-Host "  - Back to center"
  if ($UsePwm) {
    Invoke-ApiGet '/pwm/out' @{ ch = $CH_180; val = $SERVO_MID } | Out-Null
    Invoke-ApiGet '/pwm/out' @{ ch = $CH_360; val = $SERVO_MID } | Out-Null
  } else {
    Set-ServoTicks -ch $CH_180 -ticks $SERVO_MID
    Set-ServoTicks -ch $CH_360 -ticks $SERVO_MID
  }
  Write-Host "  [Done]"
}

# -----------------------------
# main
# -----------------------------
Write-Host ""
Write-Host "=== MMP API Test start ==="
# Run-Analog
Run-Digital
# Run-Mp3Playlist
# Run-Mp3Control
# Run-Pwm $true       # PWM
# Run-Pwm $false    # I2C
Write-Host "=== MMP API Test end ==="
