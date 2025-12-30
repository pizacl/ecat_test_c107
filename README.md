# Linux + Arduinoで始める EtherCAT 入門

### C107 RMFwind ロボット関係技術合同誌 サンプルコード
### Author：@pizac__ (https://x.com/pizac__)

---

## 概要

本リポジトリは，**C107 RMFwind ロボット関係技術合同誌
「Linux + Arduinoで始める EtherCAT 入門」** 向けに用意した
**EtherCAT 学習・検証用サンプルコード**です．

Linux 上で動作する EtherCAT マスターと，Arduino（Teensy 等）上で動作する
EtherCAT スレーブの最小構成で組み，効率的な学習を目指します．

コードは **実製品向けではなく，学習・実験用途**を主目的としています．

## サンプル構成

- **teensy41_led_blink**  
  Teensy 用 PlatformIO 環境確認（LED点滅）サンプル

- **teensy41_ecat_test**  
  Teensy 用 EtherCAT スレーブ実装サンプル

- **linux/**  
  Linux（EtherCAT マスター）用 ループバックテストコード

## 依存関係

### SOES（Simple Open Source EtherCAT Slave）

* **SOES v3.1.0 ベース**
* 以下リポジトリを元にしています：

```
ecat_servo/examples/SOES_CIA402_AX58100
```

* 本書の内容に合わせて，

  * 必要な機能のみを抽出
  * 構成を簡略化
  * Arduino / Teensy 環境向けに調整
    しています．

参考リポジトリ：
[https://github.com/kubabuda/ecat_servo](https://github.com/kubabuda/ecat_servo)
[Simple Open Source EtherCAT Slave](https://github.com/OpenEtherCATsociety/SOES)

### SOEM（Simple Open EtherCAT Master Library）

* **SOEM v2.0.0 依存**
* Linux 上で EtherCAT マスターとして使用します
* DC（Distributed Clocks）, Sync0, Sync1 は使用しない最小構成を前提としています

## 注意事項

* 本サンプルコードは **学習・評価目的**のものです
* リアルタイム性・安全性・耐障害性が要求される用途には適していません
* 実機接続時は **専用NICの使用**や **物理配線**に十分注意してください

## EtherCAT 商標について

EtherCAT® は **Beckhoff Automation GmbH** の登録商標です．

本リポジトリおよび本書は，
EtherCAT Technology Group（ETG）とは公式な関係はなく，
技術解説・学習目的で EtherCAT 技術を利用しています．

## ライセンス

各依存ライブラリ（SOES / SOEM）のライセンス条件に従って利用してください．
本リポジトリ内の追加コードについては、**LICENSE ファイル**を参照してください。