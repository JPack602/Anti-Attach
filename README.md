![title_letter_01](https://user-images.githubusercontent.com/80070644/201461433-65831267-4ca5-4bc2-88ed-a29a16b0a0b3.png)

# Anti-Attach  
Anti-Attachを行っているサンプルコードです。  
  
今回はデバッガによるアタッチを妨害する手法について、サンプルコード・図を用いて紹介していきたいと思います。アンチアタッチといえば「Themida」や「Obsidium」などの商用のプロテクターを連想される方が多いかと思われます。また、デバッガによる解析を妨害する手法(以後アンチデバッグとする)としては、Windows APIの中でも```kernel32.IsDebuggerPresent```[^1]関数が有名です。これは自身がデバッグされていると非0、デバッグされていない時には0が返ってきます。この関数は仕組みとしては単純で、PEB構造体の中にある"BeingDebugged"と呼ばれるデバッグ状態を保持するフラグを取得して関数の戻り値として返しているだけなのです。しかし、これでデバッガを検出することは可能ですが、デバッガによりアタッチした瞬間に自身のプロセスを終了させるような動作を実現するのは困難です。なので、今回はあまり紹介させることの少ない(少ないと思われる)アンチアタッチに焦点を当てて紹介していきます。
<br>
<br>

サンプルでは以下の様に、デバッガを用いて対象となるプロセスにアタッチした瞬間、メッセージボックスが表示されプロセスが終了しています。

https://user-images.githubusercontent.com/80070644/201461457-07bdee30-41ea-4b04-9716-f1319141e36b.mp4

※サンプルプログラムはOllyDbgでしか動作確認していません。<br>
※サンプルプログラムは32bitプログラムとしてビルドしてください。<br>
※サンプルコードを閲覧する場合は画面上にコードを表示した状態で、"github.com"というドメインを"github 1s .com"に書き換えることで、VSCode上で閲覧することが可能です。
<br>
<br>

このプログラムでは以下の様に、```ntdll.DbgUiRemoteBreakin```のエントリ付近をジャンプ命令に書き換えます。
![Ntdll DbgUiUiIssueRemoteBreakin_compare](https://user-images.githubusercontent.com/80070644/201461465-5151a23a-462f-4571-9889-f2c909e0c457.png)
<br>
<br>

**「なぜそのようなことが、アンチアタッチになるのか？」**<br>
それはプロセスがデバッガにアタッチされる際、デバッギ（デバッガの解析対象となるプロセス）内では```ntdll.DbgUiRemoteBreakin```と呼ばれる関数が呼び出されます。その為この関数のエントリを、ジャンプ命令へと書き換えています。そうすることで、デバッガにアタッチされたことをトリガーにして自身が定義した関数に無理やり遷移させることが可能となるのです。
![simple_diagram_01](https://user-images.githubusercontent.com/80070644/201461471-bac51587-6585-4227-bd0f-1ea4e87e4938.png)
<br>
<br>

通常デバッガにりアタッチされる時、デバッガが```kernelbase.DebugActiveProcess```関数を使用して、デバッギ内にリモートスレッドを作成＋```ntdll.DbgUiRemoteBreakin```の呼出しを試みます。
その為、関数のエントリを書き換えることで、```ntdll.DbgUiRemoteBreakin```の呼出しに失敗し、結果的にアタッチに失敗します。
![inside_of_KernelBase DebugActiveProcess](https://user-images.githubusercontent.com/80070644/201461478-b3fce3e2-e0e3-4e3e-b0e2-a177ff966288.png)
<br>
<br>

▼ちなみに商用のプロテクターとして有名なThemida君も、本サンプルプログラム同様に```ntdll.DbgUiRemoteBreakin```を書き換えているようです。筆者がTemida Demoを使用してみたところ、以下の様に無理やり```ntdll.LdrShutdownProcess```にジャンプさせ、アタッチを妨害しようとしていました。
<br>
<br>

![Themida-Demo-AntiAttach](https://user-images.githubusercontent.com/80070644/201461501-0579023a-6f92-4228-85eb-b32fa2fe1283.png)
<br>
<br>

このように、特定の関数をフックすることで、アンチアタッチは機能します。関数を"フックする"と言うと、IATを書き換える方が安全かつ一般的かと思われます。しかし、アンチアタッチを利用するようなケースでは、アタッチされた瞬間に自身のプロセスを終了させる処理や、自身の削除処理が予想される為、関数のエントリが書き変わってしまうことによる副作用等は一切気にしていません。

流れとしては以下の様になります。
<br>
<br>

1. フック関数(フック後の遷移先)を用意。
2. DbgUiRemoteBreakinのアドレスを取得。
3. フック関数とDbgUiRemoteBreakin関数の距離(アドレス差)を計算。
4. DbgUiRemoteBreakinのエントリを書き換える。
<br>

## 1. フック関数を用意

```C
int WINAPI MyDbgUiRemoteBreakin()
{
	MessageBoxA(NULL, "Detected Debugger!!", "anti-attatch", MB_OK);

	ExitProcess(1);

	return;
}
```

メッセージボックスを表示後にプロセスを終了させる関数となります。
その他、お好みでプロセス終了後に自身を削除する様な処理を加えても良いでしょう。
<br>
<br>

## 2. DbgUiRemoteBreakinのアドレスを取得

``` C
// ntdllのアドレスを取得
hModule = LoadLibraryA("ntdll.dll");
if (! hModule) return 1;

// DbgUiRemoteBreakinのアドレスを取得
fnDbgUiRemoteBreakin = GetProcAddress(hModule, "DbgUiRemoteBreakin");
if (! fnDbgUiRemoteBreakin) return 1;
```

LoadLibrary関数を用いて、DLLのアドレスを取得→GetProcAddressにて関数のアドレスを動的に解決。
一般的な手法を用いて、ターゲットとなる関数のアドレスを取得しています。
<br>
<br>

## 3. フック関数とDbgUiRemoteBreakin関数の相対距離を計算

```C
// 0xE9 ジャンプ命令
lpJumpCode[0] = 0xE9;

lpTemp = &lpJumpCode[1];

// アドレスの計算
// DbgUiRemoteBreakin関数のエントリから自作のフック関数までの相対距離を計算
*lpTemp  = (uint32_t)MyDbgUiRemoteBreakin;
*lpTemp -= (uint32_t)fnDbgUiRemoteBreakin;
*lpTemp -= (uint32_t)sizeof(lpJumpCode);
```

このアドレス計算が少々複雑に見えますが、これは先ほど取得したDbgUiRemoteBreakin関数と **「1. フック関数を用意」** で紹介した、自作のフック関数とのアドレスの差を求めています。
このアドレス差(相対アドレス)をジャンプ命令のオペランドとして使用します。
また、コードの最上部に```lpJumpCode[0] = 0xE9;```という式があります。これは0xE9がジャンプ命令と対応している為にlpJumpCodeの1バイト目にE9をセットしているのです。
なので、フックを行う際は以下のようなバイナリを用意します。
<br>
<br>

![address_description_top](https://user-images.githubusercontent.com/80070644/201461516-729a975d-cc00-44ed-97b0-d361a575535a.png)

最後に
```C
*lpTemp -= sizeof(lpJumpCode);
```
とありますが、ジャンプ命令ではオペランドに、 **ジャンプ命令を含むニーモニックの次のニーモニックの先頭アドレスを基準とした相対値を用います。** [^2]なので、「自作のフック関数とフック対象の関数のアドレス差(相対アドレス)から、ニーモニック自体のサイズを引いたアドレス」をオペランドとして指定する必要があります。なので、ニーモニックのサイズ分の5バイト```sizeof lpJumpCode;```を最後にマイナスしています。
なので実際に使用するバイナリでは

関数のエントリ 0x77C0DFE0<br>
遷移先が　　　 0x00AB1000<br>

だと仮定すると、（遷移先のアドレス－関数のエントリのアドレス－5バイト）がジャンプ先までの相対アドレスとなるので、
0x00AB1000 - 0x77C0DFE0 - 0x5 = **0x88EA301B** がオペランドとなります。実際に```lpJumpCode```がメモリ上に書き込まれる場合は、エンディアンネスを考慮すると以下の様になるでしょう。

![address_description_bottom](https://user-images.githubusercontent.com/80070644/201461528-7c802100-ba3b-4f79-a7d6-237c3e8c2b25.png)
<br>
<br>

## 4. DbgUiRemoteBreakinのエントリを書き換える

```C
// メモリアクセス属性を変更
VirtualProtect(fnDbgUiRemoteBreakin, sizeof(lpJumpCode), PAGE_EXECUTE_READWRITE, &dwOldProtect);

// メモリを書き換え
memcpy(fnDbgUiRemoteBreakin, lpJumpCode, sizeof(lpJumpCode));

// メモリアクセス属性を復元
VirtualProtect(fnDbgUiRemoteBreakin, sizeof(lpJumpCode), dwOldProtect, &dwOldProtect);
```

後は、VirtualProtect関数を用いてメモリの保護を変更しています。
そして、memcpyにて先ほどの **「3. フック関数とDbgUiRemoteBreakin関数の相対距離を計算」** にて作成した命令(ニーモニック)を、関数のエントリ部分に書き込んでいます。
以上でメモリの書き換えは完了です。
<br>
<br>

## さいごに
今回のサンプルコードでは、DbgUiRemoteBreakin関数のエントリをジャンプ命令に書き換えましたが、ジャンプ命令に書き換えずとも、関数のエントリをret命令に書き換えるだけでも、アタッチに失敗するようです。これはまだ、検証出来ていないので真偽は定かではありません。
<br>
<br>

この記事を見ている方のお役に立てれば幸いです。
<br>

### 参考文献
[^1]: [IsDebuggerPresent](https://learn.microsoft.com/ja-jp/windows/win32/api/debugapi/nf-debugapi-isdebuggerpresent)
[^2]: [デバッガによるx86プログラム解析入門 p.36](https://www.shuwasystem.co.jp/book/9784798042053.html)
