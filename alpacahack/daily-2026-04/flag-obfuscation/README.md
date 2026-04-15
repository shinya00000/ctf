# flag-obfuscation Writeup
<https://alpacahack.com/daily/challenges/flag-obfuscation?month=2026-04>

## 1. 概要

IPv6アドレスの文字列形式に変換されて難読化された実行ファイルを元のバイナリデータに復元し，内部に分割して保持されているフラグ（Stack Strings）を抽出する問題です．

## 2. 問題の分析

### ファイル構成
配布ファイルには，難読化処理を行う `obfuscator.c` と，難読化されたデータが格納された `data.h` が含まれています．

### ソースコードの確認
`obfuscator.c` を確認すると，入力されたバイナリファイルを16バイトのチャンクに分割し，`inet_ntop` 関数を用いてIPv6アドレスの文字列形式に変換しています．この文字列群は `data.h` の `ipv6_data` 配列として出力されています．この変換処理は可逆であるため，逆変換を行うことで元の実行ファイルを復元可能です．

## 3. 脆弱性と攻略方針

### 脆弱性
難読化が単なるフォーマット変換（バイナリからIPv6文字列へのエンコード）に留まっており，暗号学的な保護が施されていません．

さらに，復元後のバイナリはフラグを8バイトずつスタックに積む手法（Stack Strings）で保持していますが，平文のまま分割されているだけであるため，静的解析ツールや `strings` コマンドで容易に抽出可能です．

なお，`strings` コマンドの出力には `D$ H` や `T$h` などの文字列が混入しますが，これは x86-64 アーキテクチャにおけるスタック操作命令（`mov [rsp+offset], reg`）の機械語バイト列が，偶然 ASCII の印字可能文字と一致したものです．たとえば，64ビット命令を示す REX.W プレフィックス（`0x48`）は `H` となり，`rsp` をベースとする SIB バイト（`0x24`）は `$` となります．これらの特徴から，抽出された文字列が単純な文字列データではなく，アセンブリ命令の破片と混ざった Stack Strings であると判断できます．

### 攻略方針：
Python の `ipaddress` モジュールを用いて `data.h` 内のIPv6文字列を16バイトのバイナリデータにデコードし，元の PE ファイルを復元します．

復元したバイナリに対して `strings` コマンドを実行し，フラグのプレフィックスを起点に分割された文字列の断片を抽出します．その際，機械語に由来するノイズ（`H` や `D$` など）を取り除き，本来のフラグ文字列のみを結合します．

## 4. エクスプロイトコード
<https://github.com/shinya00000/ctf/blob/main/alpacahack/daily-2026-04/flag-obfuscation/solve.ipynb>
まず，以下のPythonスクリプトを実行して元のバイナリを復元します．

```python
import ipaddress
import re

def solve():
    with open("data.h", "r", encoding="utf-8") as f:
        content = f.read()

    matches = re.findall(r'"([^"]+)"', content)
    
    output_data = b""
    for ip_str in matches:
        try:
            ip = ipaddress.IPv6Address(ip_str)
            output_data += ip.packed
        except ValueError:
            continue

    with open("restored.exe", "wb") as f:
        f.write(output_data)

if __name__ == "__main__":
    solve()
```

次に，復元したバイナリからフラグを抽出します．

```bash
strings restored.exe | grep -C 10 "Alpaca"
```

出力されたスタック文字列の断片から，アセンブリの機械語由来の文字（`H` や `D$` など）を除外し，順番に結合するとFLAGが得られます．