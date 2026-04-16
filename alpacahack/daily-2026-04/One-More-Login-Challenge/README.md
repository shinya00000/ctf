# one-more-login-challenge Writeup

## 1. 概要

本問題はNode.jsとMongoDBで構築されたWebアプリケーションのログイン認証を突破するチャレンジです．ログイン画面が提供されており，本来は正しい管理者の認証情報を入力する必要がありますが，NoSQLインジェクションを用いて認証のバイパスを行いフラグを獲得します．

## 2. 問題の分析

### ファイル構成
配布されたファイル群は以下の通り構成されています．

* `compose.yaml`: アプリケーション本体とMongoDBのコンテナ起動設定
* `web/index.js`: ExpressベースのWebサーバーの実装コード
* `web/package.json` 等: 依存パッケージの定義
* `web/Dockerfile`: アプリケーションの実行環境定義

### ソースコードの確認
`web/index.js`のソースコードを確認すると，以下の設定が存在します．

```javascript
app.use(express.json());
```

これにより，サーバーは標準のフォームデータだけでなく，JSON形式のリクエストも受け付ける仕様になっています．

また，ログイン処理部分は以下のようになっています．

```javascript
const { username, password } = req.body;
const user = await client.db("db").collection("users").findOne({
  username,
  password,
});
```

リクエストのボディから抽出した`username`と`password`を，そのままMongoDBの`findOne`メソッドのクエリとして渡しています．パスワード自体はデータベース初期化時に32バイトの乱数で生成されているため，推測することは不可能です．

## 3. 脆弱性と攻略方針

### 脆弱性
検索クエリの組み立て部分にMongoDBのNoSQLインジェクションの脆弱性が存在します．
ブラウザから通常のフォーム送信を行うと文字列としてデータが送られますが，`Content-Type: application/json`を指定してJSON形式でリクエストを送信することで，文字列ではなくオブジェクト（構造化されたデータ）をサーバー側の検索クエリに直接渡すことができます．

### 攻略方針：
JSON形式のリクエストを利用し，パスワードのフィールドにMongoDBのクエリ演算子を埋め込みます．
ユーザー名に`admin`を指定し，パスワード部分に「nullと一致しない」という意味を持つ演算子`{ "$ne": null }`をオブジェクトとして送信します．これにより，パスワードが文字列として完全一致しなくても，データベース上のadminユーザーのレコードをヒットさせることができ，不正にログインを成功させることが可能です．

## 4. エクスプロイトコード

以下の`curl`コマンドを実行することで，NoSQLインジェクションが成立しフラグを取得できます．

```bash
curl -X POST http://34.170.146.252:19790/ \
     -H "Content-Type: application/json" \
     -d '{"username": "admin", "password": {"$ne": null}}'
```
