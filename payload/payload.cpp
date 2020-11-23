#include <Windows.h>
#include <ShlObj.h>
#include <string>

BOOL WINAPI DllMain(
	HINSTANCE,
	DWORD callReason,
	LPVOID
)
{
	if (callReason == DLL_PROCESS_ATTACH)
	{
		PWSTR szPath, szWindowsDir;

		auto hr = SHGetKnownFolderPath(FOLDERID_System, 0, nullptr, &szPath);
		if (FAILED(hr))
			ExitProcess(static_cast<DWORD>(hr));
		hr = SHGetKnownFolderPath(FOLDERID_Windows, 0, nullptr, &szWindowsDir);
		if (FAILED(hr))
		{
			CoTaskMemFree(szPath);
			ExitProcess(static_cast<DWORD>(hr));
		}

		std::wstring systemPath{ szPath };
		CoTaskMemFree(szPath);

		const auto result = SetEnvironmentVariableW(L"systemroot", szWindowsDir);
		CoTaskMemFree(szWindowsDir);
		if (!result)
			ExitProcess(GetLastError());
		
		systemPath += L"\\cmd.exe";

		PROCESS_INFORMATION processInfo;
		STARTUPINFOW startupInfo{ sizeof STARTUPINFOW, nullptr };

		if (!CreateProcessW(systemPath.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo,
		                    &processInfo))
		{
			ExitProcess(GetLastError());
		}

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		ExitProcess(ERROR_SUCCESS);
	}

	return TRUE;
}
