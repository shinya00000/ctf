# another-login-challenge Writeup

## 1. 概要
[cite_start]`another-login-challenge` は，提供されたWebアプリケーションのログイン機能をバイパスし，サーバー上に設定されたフラグを取得するWeb問題です [cite: 5, 25][cite_start]．ソースコードが提供されており，JavaScriptの仕様を突いた認証回避が求められます [cite: 13, 17]．

## 2. 問題の分析

### ファイル構成
* [cite_start]`web/index.js`: プログラムのメインとなるソースコード [cite: 17, 21]
* [cite_start]`web/Dockerfile` / `compose.yaml`: サーバーの実行環境設定ファイル [cite: 9, 100]
* [cite_start]`web/package.json` / `web/package-lock.json`: 依存パッケージの設定 [cite: 26, 91]

### ソースコードの確認（index.js）
プログラムの主な処理は以下の通りです．

1.  [cite_start]**ユーザーの設定**: `users` オブジェクトに `admin` ユーザーを定義し，`crypto.randomBytes(32).toString("base64")` を用いてランダムな文字列をパスワードとして設定します [cite: 23]．
2.  [cite_start]**入力の受け取り**: POSTリクエストに対して，`const { username, password } = req.body;` により入力値を受け取ります [cite: 25]．
3.  [cite_start]**認証処理**: `users[username]` でユーザーオブジェクトを取得します [cite: 25][cite_start]．その後，`if (!user || user.password !== password)` という条件式でパスワードの検証を行います [cite: 25]．
4.  [cite_start]**実行**: 認証を通過できれば，環境変数から読み込んだフラグがレスポンスとして返されます [cite: 22, 25]．

## 3. 脆弱性と攻略方針

### 脆弱性
[cite_start]JavaScriptのオブジェクトの組み込みプロパティに対するアクセス仕様と，分割代入時に値が存在しない場合の `undefined` の挙動が脆弱性となります [cite: 25][cite_start]．`users` オブジェクトにはプロトタイプが存在するため，`__proto__` や `constructor` などの継承されたプロパティを `username` として指定することが可能です [cite: 25]．

### 攻略方針：プロパティアクセスと undefined の悪用
以下の手順でパスワードチェックをバイパスし，フラグを取得できます．

1.  [cite_start]**usernameの指定**: `username` に `constructor` を指定して送信します [cite: 25][cite_start]．これにより，サーバー側での `users["constructor"]` は組み込みの `Object` 関数（truthy な値）となります [cite: 25]．
2.  [cite_start]**passwordの省略**: POSTリクエストのボディに `password` パラメータを含めずに送信します [cite: 25][cite_start]．これにより，`req.body` からの分割代入時に `password` 変数には `undefined` が代入されます [cite: 25]．
3.  [cite_start]**比較のバイパス**: `Object` 関数には `password` プロパティが存在しないため，`user.password` は `undefined` となります [cite: 25][cite_start]．結果として，`user.password !== password` の比較は `undefined !== undefined` となり，`false` と評価されるため，エラーを返さずに認証を通過できます [cite: 25]．

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

## 5. フラグの獲得

[cite_start]スクリプトを実行する（あるいは `curl -X POST -d "username=constructor" http://localhost:3000/` を実行する）ことで，サーバーからフラグが返されます [cite: 25, 26]．

**Flag:**
[cite_start]`Alpaca{REDACTED}` （実際のサーバー環境に依存します [cite: 22, 104]）