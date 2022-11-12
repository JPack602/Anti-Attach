#include <stdio.h>
#include <stdint.h>

#include <windows.h>

#ifndef RELEASE
#define DEBUG
#endif

// 自作のフック関数
int WINAPI MyDbgUiRemoteBreakin()
{
	MessageBoxA(NULL, "Detected Debugger!!", "anti-attatch", MB_OK);

	ExitProcess(1);

	return;
}

int AntiAttach()
{
#ifdef _WIN64
	// do something...
	return 1;
#endif

	HMODULE hModule = NULL;
	FARPROC fnDbgUiRemoteBreakin = NULL;

	DWORD dwOldProtect = 0;

	// 関数のエントリに書き込むデータ
	BYTE lpJumpCode[1 + sizeof(uint32_t)] = { 0 };

	uint32_t *lpTemp = NULL;

	// ntdllのアドレスを取得
	hModule = LoadLibraryA("ntdll.dll");
	if (! hModule) return 1;

	// DbgUiRemoteBreakinのアドレスを取得
	fnDbgUiRemoteBreakin = GetProcAddress(hModule, "DbgUiRemoteBreakin");
	if (! fnDbgUiRemoteBreakin) return 1;

	// 0xE9 ジャンプ命令
	//  E9 00 00 00 00
	// jmp 00 00 00 00
	lpJumpCode[0] = 0xE9;

	lpTemp = &lpJumpCode[1];

	// アドレスの計算
	// DbgUiRemoteBreakin関数のエントリから自作のフック関数までの相対距離を計算
	*lpTemp  = (uint32_t)MyDbgUiRemoteBreakin;
	*lpTemp -= (uint32_t)fnDbgUiRemoteBreakin;
	*lpTemp -= (uint32_t)sizeof(lpJumpCode);

#ifdef DEBUG
	printf("hook func : %p\n", MyDbgUiRemoteBreakin);
	printf("org  func : %p\n", fnDbgUiRemoteBreakin);
#endif

	// メモリアクセス属性を変更
	VirtualProtect(fnDbgUiRemoteBreakin, sizeof(lpJumpCode), PAGE_EXECUTE_READWRITE, &dwOldProtect);

	// メモリを書き換え
	memcpy(fnDbgUiRemoteBreakin, lpJumpCode, sizeof(lpJumpCode));

	// メモリアクセス属性を復元
	VirtualProtect(fnDbgUiRemoteBreakin, sizeof(lpJumpCode), dwOldProtect, &dwOldProtect);

	return 0;
}

int main(int argc, char **argv)
{
	// DbgUiRemoteBreakin関数のエントリを書き換える
	AntiAttach();

	// 一時停止
	char c = getchar();

	return 0;
}