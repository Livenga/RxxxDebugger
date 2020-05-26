#!/bin/bash


if [ $# -lt 1 ]
then
  echo "Usage: $0 [READER HOST]"
  echo "Example: $0 xspan-xx-yy-zz.local"
  exit 127
fi

echo "$1: 接続確認"
ping -c 3 -W 5 $1 > /dev/null 2>&1

if [ ! $? -eq 0 ]
then
  exit 128
fi

_CURRENT_DIR=$(pwd)

LOG_NAMES=(
  'dhcpcd.log'
  'syslog-app.log'
  'syslog-app.log.1'
  'syslog-app.log.2'
  'syslog-app.log.3'
  'syslog-app.log.4'
  'syslog-err.log'
  'syslog-err.log.1'
  'syslog-err.log.2'
  'syslog-err.log.3'
  'syslog-err.log.4'
)


WORK_DIRECTORY=$(date +%Y_%m_%d_%H_%M_%S)-$1
echo "作業ディレクトリ: $WORK_DIRECTORY の作成"
mkdir -v $WORK_DIRECTORY

if [ ! $? -eq 0 ]
then
  echo "作業ディレクトリ $WORK_DIRECOTRY の作成に失敗."
  exit 129
fi

cd $WORK_DIRECTORY

for LOG_NAME in ${LOG_NAMES[@]}
do
  echo "https://$1/log/$LOG_NAME: ダウンロード開始"
  # -v で詳細が表示される
  curl -OL -k -u root:impinj \
    https://$1/log/$LOG_NAME
done

cd $_CURRENT_DIR

echo 'ダウンロードファイルの圧縮'
tar zcvf $WORK_DIRECTORY.tar.gz $WORK_DIRECTORY

if [ $? -eq 0 ]
then
  rm -rv $WORK_DIRECTORY
else
  echo 'ダウンロードファイルの圧縮に失敗しました. ダウンロードファイルの削除を中止します.'
  exit 130
fi
