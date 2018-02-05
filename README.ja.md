# WatercoolingController

Arduino製本格水冷PC用ファン・ポンプコントローラ

*[English](README.md), [日本語](README.ja.md)*

## 概要

- 水温と室温の温度差、または水温によるポンプ・ファンの回転数制御
- PC用クライアントソフトウェア [WatercoolingMonitor](https://github.com/xiphen-jp/WatercoolingMonitor) との連携
  - シリアル通信でデータを送受信
  - PCに回転数・Duty比・温度データを送信しモニタリング
  - PCからCPU・GPU使用率を受信し回転数制御に利用

## 回路図

![circuit diagram](https://raw.githubusercontent.com/wiki/xiphen-jp/WatercoolingController/images/watercooling_controller_circuit.png)
![breadboard](https://raw.githubusercontent.com/wiki/xiphen-jp/WatercoolingController/images/watercooling_controller_breadboard.png)

- ポンプ・ファンは増設可能
- ポンプ・ファン合計14個まで回転数を検出可能
  - 個々にローパスフィルタとプルアップ抵抗が必要

## 依存関係

- 温度センサー用ライブラリ
  - [milesburton/Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library)

### ハードウェア

- Arduino Nano v3.0または互換品
- DS18B20温度センサー
- 基板・ピンヘッダー/ソケット・コネクター・抵抗・コンデンサ等
- PWM制御に対応したファン・ポンプ
- USBケーブル

## 作者

[@_kure_](https://twitter.com/_kure_)
