#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

SSIZE_T send(BOOL& success, HANDLE hWrite, const char* b, SSIZE_T k)
{
  SSIZE_T totalWritten = 0;
  DWORD bytesWritten = 0;

  while (totalWritten < k)
  {
    success = WriteFile(
        hWrite,
        b + totalWritten,
        static_cast<DWORD>(k - totalWritten),
        &bytesWritten,
        NULL
    );

    if (!success || bytesWritten == 0)
    {
      break;
    }

    totalWritten += bytesWritten;
  }

  return totalWritten;
}

SSIZE_T sendSize(BOOL& success, HANDLE hWrite, size_t sz)
{
  return send(success, hWrite, reinterpret_cast<const char*>(&sz), sizeof(size_t));
}

int main()
{
  std::string inputLine;
  if (!std::getline(std::cin, inputLine))
  {
    if (std::cin.bad())
    {
      fprintf(stderr, "parent: input reading error\n");
      return 1;
    }
    return 0;
  }

  inputLine += "\n";
  const char* line = inputLine.c_str();
  size_t len = inputLine.length();

  SECURITY_ATTRIBUTES saAttr = {};
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  HANDLE hRead = NULL;
  HANDLE hWrite = NULL;

  if (!CreatePipe(&hRead, &hWrite, &saAttr, 0))
  {
    fprintf(stderr, "parent: pipe creation error (Error Code: %lu)\n", GetLastError());
    return 1;
  }

  if (!SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, 0))
  {
    fprintf(stderr, "parent: SetHandleInformation error (Error Code: %lu)\n", GetLastError());
    CloseHandle(hRead);
    CloseHandle(hWrite);
    return 1;
  }

  char cmdArgs[256] = {};
  sprintf_s(cmdArgs, sizeof(cmdArgs), "child.exe %llu", reinterpret_cast<unsigned long long>(hRead));

  STARTUPINFOA si = {};
  PROCESS_INFORMATION pi = {};
  si.cb = sizeof(STARTUPINFOA);

  BOOL procCreated = CreateProcessA(
      NULL,
      cmdArgs,
      NULL,
      NULL,
      TRUE,
      0,
      NULL,
      NULL,
      &si,
      &pi
  );

  if (!procCreated)
  {
    fprintf(stderr, "parent: CreateProcess error (Error Code: %lu)\n", GetLastError());
    CloseHandle(hRead);
    CloseHandle(hWrite);
    return 1;
  }

  if (!CloseHandle(hRead))
  {
    fprintf(stderr, "parent: hRead close error (Error Code: %lu)\n", GetLastError());
    CloseHandle(hWrite);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 1;
  }

  BOOL sendSuccess = FALSE;
  sendSize(sendSuccess, hWrite, len);
  if (!sendSuccess)
  {
    fprintf(stderr, "parent: sendSize error (Error Code: %lu)\n", GetLastError());
    CloseHandle(hWrite);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 1;
  }

  send(sendSuccess, hWrite, line, static_cast<SSIZE_T>(len));
  if (!sendSuccess)
  {
    fprintf(stderr, "parent: send error (Error Code: %lu)\n", GetLastError());
    CloseHandle(hWrite);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 1;
  }

  if (!CloseHandle(hWrite))
  {
    fprintf(stderr, "parent: hWrite close error (Error Code: %lu)\n", GetLastError());
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 1;
  }

  DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);
  if (waitResult != WAIT_OBJECT_0)
  {
    fprintf(stderr, "parent: WaitForSingleObject error (Error Code: %lu)\n", GetLastError());
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 1;
  }

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return 0;
}
