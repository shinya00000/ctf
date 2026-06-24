# honk-the-klaxon writeup

## 1. 概要

本問題は与えられた暗号化オラクルと通信し未知のチャレンジ変数を復元するCrypto問題です．暗号化処理が有限体 $GF(2^8)$ 上の線形変換と定数の加算のみで構成されていることに着目するとアフィン変換としてモデル化して解読が可能です．

## 2. 問題の分析

### ファイル構成

提供されたファイルは以下の2つです．

* `honk_the_klaxon.py`：メインの暗号化スクリプトおよびサーバー処理
* `MIX.py`：暗号化に使用される定数 `PBOX` が定義されたファイル

### ソースコードの確認

`honk_the_klaxon.py`を確認するとランダムな32バイトの `KEY` と `CHALLENGE` が生成され，`CHALLENGE` を暗号化した結果が表示されます．
その後の通信では最大420回のクエリを送信できます．
16進数文字列を送信するとその値を暗号化して返します．
`guess` を送信すると`CHALLENGE` の値を当てるフェーズに移行し，正解するとフラグが得られます．
暗号化関数 `encrypt` を確認すると32ラウンドのループの中において`pt = pbox(bxor(pt, key))` を実行しています．

## 3. 脆弱性と攻略方針

### 脆弱性

本問題の暗号化関数はバイト単位でのXOR操作と位置の入れ替えのみで構成されている点が脆弱性です．
XOR演算は有限体 $GF(2^8)$ における加算と等価です．
入力ベクトルを $x$ とし，鍵ベクトルを$k$とし，`PBOX`による線形変換を行列$P$で表すと1ラウンドの処理は $P(x \oplus k)$ となります．
これを32ラウンド繰り返すため暗号化処理全体 $E(x)$ は以下のように表せます．

$$E(x)=P^{32}x \oplus \sum_{i=1}^{32}P^ik$$

右辺の第2項は $x$ に依存しない定数ベクトルです．これを行列計算で$C_{key}$と置くと $E(x)=P^{32}x \oplus C_{key}$ となるため，全体が $GF(2^8)$ 上のアフィン変換になっていることがわかります．

### 攻略方針：

上記の性質を利用し以下の手順で `CHALLENGE` を復元します．

1. サーバーに32バイトのオール0のデータを送信し $E(0)$ を取得します．$x=0$のとき $E(0)=C_{key}$ となるためこれで定数項が求まります．
2. 最初に与えられた `CHALLENGE` の暗号文と $C_{key}$ をXORすることで $P^{32}x$ に相当する値を取得します．
3. `PBOX` の定義から32×32の行列$P$を構築し，ガロア体 $GF(2^8)$ 上において $P^{32}$ の逆行列 $(P^{32})^{-1}$ を計算します．
4. 取得した値に逆行列を掛けることで元の `CHALLENGE` を復元し，`guess` として送信します．

## 4. エクスプロイトコード

```python
import galois
import numpy as np
from pwn import *
from MIX import PBOX

GF256 = galois.GF(2**8)

N = 32
M_arr = np.zeros((N, N), dtype=int)
for i, row in enumerate(PBOX):
    for j in row:
        M_arr[i, j] = 1

M = GF256(M_arr)
M32 = np.linalg.matrix_power(M, 32)
M32_inv = np.linalg.inv(M32)

io = remote("34.170.146.252", 59978)

io.recvuntil(b"CHALLENGE: ")
chal_ct_hex = io.recvline().strip().decode()
chal_ct = bytes.fromhex(chal_ct_hex)

io.recvuntil(b"pt: ")
io.sendline(b"00" * 32)
c_key_hex = io.recvline().strip().decode()
c_key = bytes.fromhex(c_key_hex)

target = bytes([b1 ^ b2 for b1, b2 in zip(chal_ct, c_key)])
target_vec = GF256(list(target))

chal_vec = M32_inv @ target_vec
chal_bytes = bytes(chal_vec.tolist())

io.recvuntil(b"pt: ")
io.sendline(b"guess")
io.recvuntil(b"challenge: ")
io.sendline(chal_bytes.hex().encode())

print(io.recvall().decode())

```