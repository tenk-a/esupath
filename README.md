# esupath.exe & besupath.bat

Windows にて SYSTEM/USER リポジトリや、(cmd)カレントプロセスの  
環境変数 PATH のディレクトリ表示/追加/削除

## esupath

```
Usage: esupath [オプション] <dir...>
  環境変数 PATH の表示/追加/削除.
[オプション]
 -l --list              PATH のディレクトリ一覧出力.
 -r --remove  <DIR...>  ディレクトリを削除. ワイルドカード指定可.
 -p --prepend <DIR...>  PATH の先頭にディレクトリ追加.
 -a --append  <DIR...>  PATH の最後にディレクトリ追加.
 -u --user              ユーザー環境変数を対象(レジストリ操作)
 -s --system            システム環境変数を対象(レジストリ操作)
 -e --env               現在プロセスの環境変数を対象.
 -b --batch <FILE>      現在プロセスの環境変数を対象、変更結果はバッチ出力.
 -y --yes               書き込み前のキー入力待ちをスキップ.
 -c --clear             PATH を全削除 (--var 変更時での使用を想定).
    --var <NAME>        PATH でなく対象環境変数を <NAME> に変更.
    --delete-var        --var で指定した環境変数を削除.
    --silent            確認のための編集前後の内容表示や入力待ちを行わない.
 @file                  file からコマンドライン引数を取得.
```

オプション -r -p -a の引数 dir... は次の -r -p -a まで複数指定可能。  
また 一つの文字列で ; で区切って複数指定もできる。

環境変数 PATH から -r,-p,-a で指定のディレクトリを削除したあと、  
先頭および最後への追加を行う。

```bat: ex1
:例: USER PATH の後ろに 3つパスを追加.
esupath -u -a c:\foo\bin "d:bar\bin;e:\baz\bin"
```

削除でのワイルドカード文字は ? * **。  
```
 ? は 1文字、  
 * は / \ 以外の0文字以上、  
 ** は / \ 含め 0文字以上  
にマッチする。
```

```bat:
::例: USER の PATH から 既存の /CMake を含むディレクトリを削除して、新規に先頭に C:\tools\CMake を追加.
esupath -u -r "**/CMake**" -p C:\tools\CMake
```

直接編集できるのは SYSTEM/USER レジストリのみで、  
現プロセスの環境変数へは編集結果を直接反映できない。  
現在の環境変数へ反映するには -b --batch で出力したバッチを実行して行う。  

```bat: ex3 
::例: PATH の先頭に c:\foo\bin を追加し set PATH=...をtmp.batに出力. 成功してたら実行.
esupath -e -b tmp.bat -p "c:\foo\bin"
if exist tmp.bat  call tmp.bat
```

SYSTEM レジストリの変更は管理者権限が必要で、ない場合は昇格ダイアログが出る。  
昇格後、ツールはもう一度最初から実行される。  
※ ので再度状態表示されたり入力待ちになった場合はやりなおしてください。


### ログ

SYSTEM/USER 環境変数の内容が変わる操作では、編集前後の値を次の履歴ログへ追記する。

```text
C:\ProgramData\esupath\Logs\esupath_system.his
C:\Users\<ユーザー>\AppData\esupath\Logs\esupath_user.his
```

SYSTEM 環境変数の書き込み・削除結果は、次の操作ログへ UTF-8 で追記する。

```text
C:\ProgramData\esupath\Logs\esupath_system.log
```

ログ用ディレクトリとファイルは、初回の書き込み時に自動作成される。

## besupath.bat

besupath.bat は esupath -b を用いて カレントプロセスの PATH 設定をする。

必ず -b "テンポラリファイル" が指定され、バッチの引数は  
全て esupath.exe 渡される。  
※ ただしバッチの引数文字列の仕様/制限の影響をうける。

```bat: ex4
::例: カレントプロセスの PATH の最後に c:\bin を追加.
besupath -a "c:\bin"

::例: カレントプロセスと USER の PATH から /CMake を含むディレクトリを削除し、c:\Tools\CMakeを先頭に追加.
besupath -u -r "**/CMake**" -p "c:\Tools\CMake"
```


writen by tenk* ( https://github.com/tenk-a/ )
