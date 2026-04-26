# multi-xor writeup
<https://alpacahack.com/daily/challenges/multi-xor>

## 1. 概要
本問題はカスタムされた非線形なバイト変換関数とXOR演算を組み合わせた暗号処理を解読するCrypto問題です．

## 2. 問題の分析

### ファイル構成
暗号化処理を実装したPythonスクリプトの `chall.py` と暗号文および各ラウンド鍵が記録された `output.txt` が与えられます．

### ソースコードの確認
ソースコードを確認するとフラグの各バイトに対して関数 `f` または関数 `g` を適用し，ランダムに生成された鍵とXORをとる操作を40ラウンド繰り返しています．
各ラウンドで使用される鍵はすべて `output.txt` に出力されており既知のデータとなっています．
バイト変換に用いられる `f_byte` と `g_byte` はビットシフトやXORを用いた処理ですが，文字列全体ではなく各バイトに対して個別に処理が行われます．

## 3. 脆弱性と攻略方針

### 脆弱性
関数 `f` および関数 `g` によるバイト変換とXORは各文字の位置に対して完全に独立して適用されています．
そのため前の文字の暗号化結果が次の文字の処理に影響を与えるような拡散の仕組みが存在しません．
また入力は1バイトの文字コードであるため，1文字につき取り得る値は0から255までの256通りしか存在しません．

### 攻略方針：
フラグ全体の長さを `cipher_f` から取得し，各インデックスにおいて0から255までの候補となる全バイト値を順に試します．
それぞれの候補値に対して40ラウンドの `f_byte` 関数および `g_byte` 関数の処理と既知のラウンド鍵を用いたXORをシミュレーションします．
最終結果が `cipher_f` および `cipher_g` の対応する位置のバイト値と両方一致した場合に，その候補値が正しいフラグの文字であると特定できます．

## 4. エクスプロイトコード
```python
def f_byte(x: int) -> int:
    x &= 0xFF
    return (x ^ ((x << 1) & 0xFE) ^ ((x >> 1) & 0x7F)) & 0xFF

def g_byte(x: int) -> int:
    x &= 0xFF
    return (x ^ ((x << 3) & 0xF8) ^ ((x >> 3) & 0x1F)) & 0xFF

if __name__ == "__main__":
    cipher_f_hex = "24c450f131b6b7430e3db3e57dd36494fc8a670db55e69bc6ccfd62e4c2dc4b2379a88c24dab026b"
    cipher_g_hex = "6a9a80fab53cc465ee01fdfcecc87757ec3abeaef947cd538e4150b133cdcb5957c83e35d151187d"
    
    f_keys_hex = [...# 与えられたfkのリスト（省略）]
    g_keys_hex = [...# 与えられたgkのリスト（省略）]

    cipher_f = bytes.fromhex(cipher_f_hex)
    cipher_g = bytes.fromhex(cipher_g_hex)
    f_keys = [bytes.fromhex(k) for k in f_keys_hex]
    g_keys = [bytes.fromhex(k) for k in g_keys_hex]

    R = 40
    n = len(cipher_f)
    flag = bytearray(n)

    for idx in range(n):
        for c in range(256):
            state_f = c
            for i in range(R):
                state_f = f_byte(state_f)
                state_f ^= f_keys[i][idx]
            
            if state_f != cipher_f[idx]:
                continue
                
            state_g = c
            for i in range(R):
                state_g = g_byte(state_g)
                state_g ^= g_keys[i][idx]
                
            if state_g == cipher_g[idx]:
                flag[idx] = c
                break

    print("Flag:", flag.decode())
```