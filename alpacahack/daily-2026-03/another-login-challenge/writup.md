# another-login-challenge Writeup

## 1. 概要
`another-login-challenge` は，提供されたWebアプリケーションのログイン機能をバイパスし，サーバー上に設定されたフラグを取得するWeb問題です．ソースコードが提供されており，JavaScriptの仕様を突いた認証回避が求められます．

## 2. 問題の分析

### ファイル構成
* `web/index.js`: プログラムのメインとなるソースコード
* `web/Dockerfile` / `compose.yaml`: サーバーの実行環境設定ファイル
* `web/package.json` / `web/package-lock.json`: 依存パッケージの設定

### ソースコードの確認（index.js）
プログラムの主な処理は以下の通りです．

1.  **ユーザーの設定**: `users` オブジェクトに `admin` ユーザーを定義し，`crypto.randomBytes(32).toString("base64")` を用いてランダムな文字列をパスワードとして設定します [cite: 23]．
2.  **入力の受け取り**: POSTリクエストに対して，`const { username, password } = req.body;` により入力値を受け取ります．
3.  **認証処理**: `users[username]` でユーザーオブジェクトを取得します．その後，`if (!user || user.password !== password)` という条件式でパスワードの検証を行います．
4.  **実行**: 認証を通過できれば，環境変数から読み込んだフラグがレスポンスとして返されます [cite: 22, 25]．

## 3. 脆弱性と攻略方針

### 脆弱性
JavaScriptのオブジェクトの組み込みプロパティに対するアクセス仕様と，分割代入時に値が存在しない場合の `undefined` の挙動が脆弱性となります．`users` オブジェクトにはプロトタイプが存在するため，`__proto__` や `constructor` などの継承されたプロパティを `username` として指定することが可能です．

### 攻略方針：プロパティアクセスと undefined の悪用
以下の手順でパスワードチェックをバイパスし，フラグを取得できます．

1.  **usernameの指定**: `username` に `constructor` を指定して送信します．これにより，サーバー側での `users["constructor"]` は組み込みの `Object` 関数（truthy な値）となります．
2.  **passwordの省略**: POSTリクエストのボディに `password` パラメータを含めずに送信します．これにより，`req.body` からの分割代入時に `password` 変数には `undefined` が代入されます．
3.  **比較のバイパス**: `Object` 関数には `password` プロパティが存在しないため，`user.password` は `undefined` となります．結果として，`user.password !== password` の比較は `undefined !== undefined` となり，`false` と評価されるため，エラーを返さずに認証を通過できます．

## 4. エクスプロイトコード

Pythonの `requests` モジュールを利用して，パスワードを意図的に欠落させたリクエストを送信するスクリプトを作成しました．

```python
import requests

# 接続先設定
URL = 'http://localhost:3000/'

# ペイロードの構築
# passwordを含めないことで，サーバー側でundefinedとして評価させる
data = {
    'username': 'constructor'
}

# 送信
response = requests.post(URL, data=data)

# 結果の受け取り
print(response.text)
```
