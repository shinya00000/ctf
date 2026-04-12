先ほどまで取り組まれていた `server.py` のCrypto問題について，ご提示いただいた `writeup.md` と全く同じ構成でWriteupを作成しました．

---

# server.py Writeup
<https://alpacahack.com/daily/challenges/bloom?month=2026-03>

## 1. 概要
本問題は，提供されたPythonスクリプト `server.py` が動作するサーバーと通信し，暗号化されたフラグを解読するCrypto問題です．独自の乱数生成関数を用いたXOR暗号化が実装されていますが，その乱数の範囲指定に起因する偏りを利用して平文を復元します．

## 2. 問題の分析

### ファイル構成
* `server.py`: 暗号化処理を行うサーバーのソースコード

### ソースコードの確認（server.py）
プログラムの主な処理は以下の通りです．

1.  **乱数生成**: `secure_randint(a, b)` は `secrets` モジュールを用いて `a` 以上 `b` 以下の乱数を生成します．
2.  **暗号化処理**: `encrypt(plain)` では，平文の長さと同じ回数だけ `secure_randint(1, 255)` を呼び出して鍵（`key`）を生成し，平文と鍵を `zip()` で1バイトずつペアにしてXOR演算を行います．
3.  **無限ループ**: `while True` ループ内で，ユーザーがEnterキーを押すたびに，同じフラグ（`FLAG`）に対する新しい暗号文を16進数文字列として出力します．

## 3. 脆弱性と攻略方針

### 脆弱性
鍵の生成に `secure_randint(1, 255)` が使われている点が最大の脆弱性です．1バイトのデータは本来 0 から 255 までの256通りですが，この指定により鍵（ $K$ ）として **0 が絶対に選ばれません**．

### 攻略方針：消去法による平文の特定
XOR演算には $P \oplus 0 = P$ という性質があります．しかし今回，$K$ は絶対に 0 にならないため，暗号文（ $C$ ）が平文（ $P$ ）と同じ値になることは絶対にありません（ $C \neq P$ ）．

この性質を利用し，以下の手順でフラグを取得できます．

1.  **データ収集**: サーバーに何度もリクエストを送り，同じフラグの暗号文を大量（数千回程度）に取得する．
2.  **候補の除外**: 各バイト位置において，0 から 255 までの全候補から「暗号文として出現した値」を削除していく．
3.  **平文の特定**: 最終的に，各バイト位置で「1度も出現しなかった（候補として1つだけ残った）値」が，元のフラグの文字コードであると特定できる．

## 4. エクスプロイトコード

ノートブック環境でも動作するよう，標準ライブラリの `socket` を利用して，サーバーから大量の暗号文を取得し，消去法でフラグを復元するスクリプトを作成しました．

```python
import socket

# ヘルパー関数
def recvuntil(sock, suffix):
    data = b""
    while not data.endswith(suffix):
        chunk = sock.recv(1)
        if not chunk:
            raise ConnectionError("サーバーから切断されました")
        data += chunk
    return data

# 接続設定
HOST = '34.170.146.252'
PORT = 10790

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))

# フラグ長の取得
recvuntil(s, b"Press Enter to get the encrypted flag...")
s.sendall(b"\n")
recvuntil(s, b"Encrypted flag: ")
first_cipher = bytes.fromhex(recvuntil(s, b"\n").strip().decode())
flag_length = len(first_cipher)

# 候補の初期化
candidates = [set(range(256)) for _ in range(flag_length)]
ITERATIONS = 3000

# 暗号文の収集と消去法
for i in range(ITERATIONS):
    recvuntil(s, b"Press Enter to get the encrypted flag...")
    s.sendall(b"\n")
    recvuntil(s, b"Encrypted flag: ")
    cipher = bytes.fromhex(recvuntil(s, b"\n").strip().decode())

    for j in range(flag_length):
        if cipher[j] in candidates[j]:
            candidates[j].remove(cipher[j])

s.close()

# フラグの復元
flag = bytearray()
for j in range(flag_length):
    if len(candidates[j]) == 1:
        flag.append(list(candidates[j])[0])
    else:
        flag.append(ord('?'))

print("\n[+] Recovered Flag:")
print(flag.decode(errors='ignore'))
```