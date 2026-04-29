# fushigi-crawler writeup
<https://alpacahack.com/daily/challenges/fushigi-crawler?month=2026-01>
webhook.site <https://webhook.site>

## 1. 概要
本問題は与えられたURLをクローリングするWebアプリケーションに存在する脆弱性を突いてフラグを取得する問題です．

## 2. 問題の分析

### ファイル構成
Node.jsとExpressを用いたアプリケーションです．
配布されたファイルの中では`compose.yaml`や`bot/index.js`が重要な部分となります．

## 3. 脆弱性と攻略方針

### 脆弱性
サーバー側において外部URLへリクエストを送信する際，`fetch`関数のオプションとしてヘッダーに環境変数の`FLAG`をそのまま含めてしまっている箇所が脆弱性です．

### 攻略方針：
自身でHTTPリクエストを受信できるWebhookサーバー（webhook.siteなど）のURLを用意します．
対象サーバーのクローリング機能にそのWebhookのURLを指定することで攻撃者が用意したサーバーへリクエストを強制的に送信させることができます．
受信したHTTPリクエストのヘッダーの内容を確認するとフラグを読み取ることができます．
