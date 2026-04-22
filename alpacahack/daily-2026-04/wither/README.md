# wither　Writeup
<https://alpacahack.com/daily/challenges/wither?month=2026-04>

## 1. 概要

AND演算を用いた独自の暗号化スクリプトからフラグを復元するCrypto問題です．同一の平文に対して複数回の暗号化を要求できる仕様を利用し，暗号文のサンプルを収集してフラグを復元します．

## 2. 問題の分析

### ファイル構成

 `wither_server.py` 

### ソースコードの確認

サーバーは `secrets.randbelow` を用いて0から255までのセキュアな乱数を生成し，入力と同じ長さのランダムな鍵を生成しています．
暗号化処理である `encrypt` 関数では，平文と生成された鍵の各バイト間で論理積（AND）をとって暗号文を作成しています．
また，`while True` ループにより，ユーザーがEnterキーを入力するたびに，同じ平文（FLAG）に対して毎回異なるランダムな鍵で暗号化された結果が16進数文字列として出力される仕様となっています．

## 3. 脆弱性と攻略方針

### 脆弱性

暗号化に排他的論理和（XOR）ではなく論理積（AND）を用いている点が本質的な脆弱性です．
AND演算の性質上，平文の特定のビットが `0` であれば，鍵のビットにかかわらず暗号文の対応するビットも必ず `0` となります．一方，平文のビットが `1` の場合，鍵のビットが `1` になったときにのみ暗号文のビットが `1` となります（ランダムな鍵を使用しているため，この確率は50%となります）．
加えて，同一の平文に対して無制限に暗号化結果を要求できる設計も致命的です．

### 攻略方針：

単一の暗号文から元データを逆算することは不可能ですが，複数回の暗号化結果を取得し，それらのビットごとの論理和（OR）を累積していくアプローチをとります．
試行回数を重ねるごとに，平文のビットが `1` であるべき位置にはいずれ必ず `1` が出現します．十分な回数（例えば100回程度）試行して論理和をとることで，すべての `1` のビットが揃い，元のフラグ文字列が完全に復元されます．

## 4. エクスプロイトコード
<https://github.com/shinya00000/ctf/blob/main/alpacahack/daily-2026-04/wither/solve.ipynb>

```python
from pwn import *

io = remote("34.170.146.252", 29127)

recovered_flag = None

for i in range(100):
    io.sendline(b"")
    io.recvuntil(b"Encrypted flag: ")
    
    hex_str = io.recvline().strip().decode()
    cipher = bytes.fromhex(hex_str)

    if recovered_flag is None:
        recovered_flag = bytearray(cipher)
    else:
        for j in range(len(cipher)):
            recovered_flag[j] |= cipher[j]

print(recovered_flag.decode('utf-8'))

io.close()
```