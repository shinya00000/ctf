# permission-denied-3 writeup
<https://alpacahack.com/daily/challenges/permission-denied-3S>

## 1. 概要
本問題は削除されたファイルにアクセスする手法を問う問題です．実行中のプロセスが保持するファイルディスクリプタを利用してフラグを取得します．

## 2. 問題の分析

### ファイル構成
* chal.sh
* compose.yml
* Dockerfile

### ソースコードの確認
chal.shでは標準入力からフラグを読み込みflag.txtを作成しています．その後rmコマンドによってカレントディレクトリのファイルがすべて削除されshが起動します．
Dockerfileではsocatを用いてTCP接続時にbash chal.shを実行するよう設定されています．

## 3. 脆弱性と攻略方針

### 脆弱性
Linux環境においてファイルがディスク上から削除されてもプロセスがそのファイルを開いている限り/proc/[PID]/fd/経由で内容を読み取ることができる点です．
本問題の実行環境では親プロセスのbashがchal.shを実行しており，子プロセスのshが終了するまでbashはchal.shをファイルディスクリプタ255番として保持し続けています．

### 攻略方針：
起動したシェルから/proc/*/fd/255を読み取ることで削除されたchal.shの中身を直接出力しフラグを取得します．

## 4. エクスプロイトコード
```bash
cat /proc/*/fd/255 2>/dev/null
```