#include <iostream>
#include <windows.h>
#include <tlhelp32.h>


void printMyPid()
{
	wchar_t myTitle[1024];
	GetConsoleTitle(&myTitle[0], 1024);

	HWND myWindow = FindWindow(NULL, myTitle);

	DWORD pid;
	GetWindowThreadProcessId(myWindow, &pid);

	printf("My pid is %d\n", pid);
}

void printExplorerPid()
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			std::wstring binaryPath = entry.szExeFile;
			if (binaryPath.find(L"explorer.exe") != std::wstring::npos)
			{
				printf("Explorer's pid is %d\n", entry.th32ProcessID);
				break;
			}
		}
	}

	CloseHandle(snapshot);
}

template<typename T>
T readMemoryAPI(HANDLE process, LPVOID address)
{
	T value;
	ReadProcessMemory(process, address, &value, sizeof(T), NULL);
	return value;
}

template<typename T>
void writeMemoryAPI(HANDLE process, LPVOID address, T value)
{
	WriteProcessMemory(process, address, &value, sizeof(T), NULL);
}

template<typename T>
DWORD protectMemory(HANDLE process, LPVOID address, DWORD prot)
{
	DWORD oldProt;
	VirtualProtectEx(process, address, sizeof(T), prot, &oldProt);
	return oldProt;
}

void readAndWriteMemoryAPI(HANDLE process, LPVOID address)
{
	DWORD value = readMemoryAPI<DWORD>(process, address);
	
	printf("Current mem value is %d\n", value);
	value++;

	DWORD oldProt = protectMemory<DWORD>(process, address, PAGE_READWRITE);
	writeMemoryAPI<DWORD>(process, address, value);
	protectMemory<DWORD>(process, address, oldProt);


	value = readMemoryAPI<DWORD>(process, address);

	printf("New mem value is %d\n", value);
}

template<typename T>
T readMemoryPointer(LPVOID address)
{
	return *((T*)address);
}

template<typename T>
void writeMemoryPointer(LPVOID address, T value)
{
	*((T*)address) = value;
}

template<typename T>
T* pointMemory(LPVOID address)
{
	return ((T*)address);
}
void readAndWriteMemoryMarshall(LPVOID address)
{
	DWORD value = readMemoryPointer<DWORD>(address);
	
	printf("Current mem value is %d\n", value);
	value++;

	writeMemoryPointer<DWORD>(address, value);
	value = readMemoryPointer<DWORD>(address);

	printf("New mem value is %d\n", value);
}


DWORD getMyBaseAddressGMH()
{
	return (DWORD)GetModuleHandle(NULL);
}

DWORD getMyBaseAddressFS()
{
	DWORD newBase;
	__asm
	{
		MOV EAX, DWORD PTR FS:[0x30]
		MOV EAX, DWORD PTR DS:[EAX+0x8]
		MOV newBase, EAX
	}
	return newBase;
}

DWORD getMyBaseRemoteGMH(HANDLE Process)
{
	LPVOID TIB;
	__asm
	{
		MOV EAX, DWORD PTR FS:[0x18]
		ADD EAX, 0x30
		MOV TIB, EAX
	}
	// read 0x30 bytes past _the_game's_ TIB to get the PEB
	DWORD PEB = readMemoryAPI<DWORD>(Process, TIB);
	// read 0x8 bytes past _the_game's_ PEB to get the base
	return readMemoryAPI<DWORD>(Process, (LPVOID)(PEB + 0x08));
}

void printMyBaseAddresses(HANDLE Process)
{
	DWORD base1 = getMyBaseAddressGMH();
	DWORD base2 = getMyBaseAddressFS();
	DWORD base3 = getMyBaseRemoteGMH(Process);
	if (base1 != base2 || base2 != base3)
		printf("Woah, this should be impossible!\n");
	else
		printf("My base address is 0x%08x\n", base1);
}

int main(void)
{
	HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());

	printMyBaseAddresses(proc);
	printMyPid();
	printExplorerPid();

	// lets do some memory stuff.. to ourself
	DWORD someValue = 1234;
	readAndWriteMemoryAPI(proc, &someValue);
	readAndWriteMemoryMarshall(&someValue);

	system("pause");
}