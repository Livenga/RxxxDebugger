# script ファイル

## download_log.sh
`/mnt/spp/log/*.log` に格納されているログファイルのダウンロード.  
事前に SpeedwayConnect がインストールされており, `/cust/lighttpd/webroot` に `/mnt/spp/log` のシンボリックリンクがはられていることが前提.  
SpeedwayConnect がインストールされている必要はないが, http(s) 経由でログファイルにアクセスさせる必要がある.
