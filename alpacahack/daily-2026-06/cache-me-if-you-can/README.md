# cache-me-if-you-can writeup
<https://alpacahack.com/daily/challenges/cache-me-if-you-can>


## 1. 概要

本問題はNginxによるキャッシュ機構とFlaskアプリの実装の隙間を突くWeb系の問題である．

## 2. 問題の分析

### ファイル構成

配布されたアーカイブファイルには主に以下のファイルが含まれている．

* `app/server.py`
* `app/Dockerfile`
* `nginx/default.conf`
* `nginx/Dockerfile`
* `compose.yaml`

### ソースコードの確認

`app/server.py`を確認すると`/flag`エンドポイントはグローバル変数`first_request`を利用しており最初のアクセス時に`Cache me if you can.`という文字列を返し2回目以降で`FLAG`を返す仕様となっている．しかし`nginx/default.conf`を確認すると`location /`において`proxy_cache flag_cache;`および`proxy_cache_valid 200 365d;`が設定されている．これにより最初に`/flag`へアクセスした際の200 OKレスポンスが365日間キャッシュされるため同じURLにアクセスし続けると常にダミーの文字列が返ってくる状態となる．

## 3. 脆弱性と攻略方針

### 脆弱性

Nginxのデフォルトのキャッシュキーにはクエリストリングが含まれる．一方でFlaskはルーティングにおいてクエリストリングを無視するため`/flag?a=1`のようなリクエストであっても`/flag`として処理を行う．このNginxとFlask間のURL解釈の差異が脆弱性となる．

### 攻略方針：

まず`/flag`に通常のアクセスを行いバックエンドの`first_request`変数をFalseに変更させる．次にNginxのキャッシュを回避することでバックエンドへ再度リクエストを到達させる必要があるため任意のクエリにアクセスする．これにより新たなキャッシュキーとして認識されFlaskアプリへと到達するためフラグを入手できる．

## 4. エクスプロイトコード

```python
import requests

URL = "http://..."

# 1回目のリクエスト（キャッシュされるが，バックエンドのfirst_requestフラグをFalseにする）
requests.get(f"{URL}/flag")

# 2回目のリクエスト（クエリストリングを付与してキャッシュを回避し，フラグを取得する）
res = requests.get(f"{URL}/flag?shinya=1")
print(res.text)

```