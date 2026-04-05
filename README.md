# x1nes

ObaraEmmanuel氏が作成されたNESをX1turboZに移植したものです。
https://github.com/ObaraEmmanuel/NES

# 実装状況

* 入力対応していません。
* 無音です。
* mapper0のみ対応しています。

# 注意
* 物凄く物凄く物凄く遅いです。  
  X1エミュレータのフルスピード以上を推奨します。

# ビルド方法

## 必要な物

* SCDD
* AILZ80ASM
* HuDisk
* SWXCV110.d88

## 手順

1. SCDD、AILZ80ASM、HuDiskにパスを通しておきます。
1. SWXCV110.d88をsource\x1nes\resに入れます。  
   無くても大丈夫ですが、あると起動できるd88イメージを作成します。  
   便利です。
1. source\setup\make.batを実行します。
1. source\x1nesp\make.batを実行します。
1. source\x1nesp\x1nes.d88をバイナリエディタで開いてAUTOEXEC.BATの属性をアスキーにします。  
   （AUTOEXECの前の01を04に）  

# 動かし方

1. ドライブ0にx1nes.d88、ドライブ1にデータを入れてリセットします。

# データについて

NESのヘッダー付きでDATA0.BINというファイル名でディスクイメージを作成してください。  
サイズは24KiBぐらいまでなら大丈夫だと思います。  
それ以上だと複数ファイル(DATA1.BIN～)に分割してください。
