# camelid-match writeup
<https://alpacahack.com/daily/challenges/camelid-match>

## 1. 概要
本問題はランダムに生成された2つの真理値の論理積を推測するゲームです．
各ラウンドにおいてサーバーは0か1の乱数を2つ生成し指定された文字列にエンコードして結合します．
結合した文字列の先頭2文字を入れ替えた上でランダムな文字数だけローテーションさせた文字列をプレイヤーに提示します．
プレイヤーは提示された文字列を分析し元の2つの乱数が両方とも1であるかどうかを「y」または「n」で回答します．
10回のラウンドすべてに連続して正解するとフラグを獲得できます．

## 2. 問題の分析

### ファイル構成

* `chall.py`
* `Dockerfile`
* `compose.yaml`

### ソースコードの確認
`chall.py`では`a`と`b`という2つのランダムなビット（0または1）が生成されます．
これらは特定の文字列に変換された後，`MID`を挟んで結合されます．
その後文字列のインデックス0と1が入れ替えられ，`rot`関数によって0から4のランダムなシフト量でローテーションされます．

## 3. 脆弱性と攻略方針

### 脆弱性
生成される文字列のパターンが極めて少ないことが挙げられます．
`a`と`b`の組み合わせが4通りであり，ローテーションのシフト量が5通りであるため生成される文字列は最大でも20通りしか存在しません．

### 攻略方針：
ブルートフォース（全探索）による事前計算が有効です．
ローカル環境において20通りのすべての文字列パターンを生成しそれぞれに対する正解（`a`と`b`が共に1なら`y`，それ以外なら`n`）を対応付けた辞書を作成します．
サーバーから送られてくる文字列をこの辞書と照らし合わせることで確実に正解を導き出すことができます．

## 4. エクスプロイトコード
```python
from pwn import remote

YES = '♡♧'
NO = '♧♡'
MID = '♡'

def rot(s, k):
    k %= len(s)
    return s[k:] + s[:k]

def enc(bit):
    return YES if bit else NO

# 辞書の作成
ans_dict = {}
for a in range(2):
    for b in range(2):
        s = enc(a) + MID + enc(b)
        s = s[1] + s[0] + s[2:]
        for k in range(5):
            rotated_s = rot(s, k)
            ans_dict[rotated_s] = 'y' if (a and b) else 'n'

def main():
    io = remote("34.170.146.252", 43344)

    for _ in range(10):
        io.recvuntil(b'Open cards: ')
        
        cards = io.recvline().decode().strip()
        
        ans = ans_dict[cards]
        io.sendlineafter(b'> ', ans.encode())

    print(io.recvall().decode())

if __name__ == '__main__':
    main()
```