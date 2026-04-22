# litealpaca Writeup

## 1. 概要
本問題はPythonの悪意のあるパッケージ（Wheelファイル）を模したCTF問題です．Pythonの`.pth`ファイルを利用したコード実行の仕組みを理解し，隠蔽されたペイロードからフラグを抽出することが目的となります．

## 2. 問題の分析

### ファイル構成
アーカイブ内には`litealpaca-1.0.0-py3-none-any.whl`を展開したディレクトリが存在します．この中にはパッケージ本体である`litealpaca`ディレクトリやメタデータが格納された`litealpaca-1.0.0.dist-info`が含まれています．特に注目すべき点として，Wheelのルートディレクトリに`litealpaca_init.pth`というファイルが配置されています．

### ソースコードの確認
`litealpaca_init.pth`の中身を確認すると，`import`文から始まり，`subprocess`を用いてBase64エンコードされた文字列をデコードして実行するPythonコードが記述されています．また，問題を作成したスクリプトである`build.py`を確認すると，ダミーのパッケージング処理の過程で，フラグを出力するペイロードをBase64化し，この`.pth`ファイルに書き込んでいることがわかります．

## 3. 脆弱性と攻略方針

### 脆弱性
Pythonではパッケージ群が配置されるディレクトリに`.pth`ファイルを置くことで，パスの追加などを行えます．この中で`.pth`ファイルの行が`import`で始まる場合，Pythonの初期化時にその行がそのままコードとして自動実行されるという仕様が存在します．本問題はこの仕様を悪用したサプライチェーン攻撃（悪意のあるパッケージによる任意コード実行）をテーマにしています．

### 攻略方針：
悪意のある処理の本体である`litealpaca_init.pth`内のBase64エンコードされた文字列を抽出してデコードし，背後で実行されようとしていたOSコマンドを特定してフラグを取得します．

## 4. エクスプロイトコード
`litealpaca_init.pth`に記述されているBase64文字列を取り出してデコードします．

```python
import base64

encoded_payload = "aW1wb3J0IG9zOyBvcy5zeXN0ZW0oImVjaG8gJ0FscGFjYXtQeVBJX3A0Y2s0ZzNzX2M0bl9iM19kNG5nM3IwdXN9JyA+IC90bXAvZmxhZy50eHQiKQ=="
decoded_payload = base64.b64decode(encoded_payload).decode('utf-8')

print(decoded_payload)
```