#include <stdio.h>
#include <stdint.h>

#include <windows.h>

#ifndef RELEASE
#define DEBUG
#endif

// ����̃t�b�N�֐�
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

	// �֐��̃G���g���ɏ������ރf�[�^
	BYTE lpJumpCode[1 + sizeof(uint32_t)] = { 0 };

	uint32_t *lpTemp = NULL;

	// ntdll�̃A�h���X���擾
	hModule = LoadLibraryA("ntdll.dll");
	if (! hModule) return 1;

	// DbgUiRemoteBreakin�̃A�h���X���擾
	fnDbgUiRemoteBreakin = GetProcAddress(hModule, "DbgUiRemoteBreakin");
	if (! fnDbgUiRemoteBreakin) return 1;

	// 0xE9 �W�����v����
	//  E9 00 00 00 00
	// jmp 00 00 00 00
	lpJumpCode[0] = 0xE9;

	lpTemp = &lpJumpCode[1];

	// �A�h���X�̌v�Z
	// DbgUiRemoteBreakin�֐��̃G���g�����玩��̃t�b�N�֐��܂ł̑��΋������v�Z
	*lpTemp  = (uint32_t)MyDbgUiRemoteBreakin;
	*lpTemp -= (uint32_t)fnDbgUiRemoteBreakin;
	*lpTemp -= (uint32_t)sizeof(lpJumpCode);

#ifdef DEBUG
	printf("hook func : %p\n", MyDbgUiRemoteBreakin);
	printf("org  func : %p\n", fnDbgUiRemoteBreakin);
#endif

	// �������A�N�Z�X������ύX
	VirtualProtect(fnDbgUiRemoteBreakin, sizeof(lpJumpCode), PAGE_EXECUTE_READWRITE, &dwOldProtect);

	// ����������������
	memcpy(fnDbgUiRemoteBreakin, lpJumpCode, sizeof(lpJumpCode));

	// �������A�N�Z�X�����𕜌�
	VirtualProtect(fnDbgUiRemoteBreakin, sizeof(lpJumpCode), dwOldProtect, &dwOldProtect);

	return 0;
}

int main(int argc, char **argv)
{
	// DbgUiRemoteBreakin�֐��̃G���g��������������
	AntiAttach();

	// �ꎞ��~
	char c = getchar();

	return 0;
}