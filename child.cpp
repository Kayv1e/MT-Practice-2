#include <windows.h>
#include <cstdio>
#include <cstdlib>

SSIZE_T recv(BOOL& success, HANDLE hRead, char* b, SSIZE_T k)
{
  SSIZE_T totalRead = 0;
  DWORD bytesRead = 0;

  while (totalRead < k)
  {
    success = ReadFile(
        hRead,
        b + totalRead,
        static_cast<DWORD>(k - totalRead),
        &bytesRead,
        NULL
    );

    if (!success || bytesRead == 0)
    {
      break;
    }

    totalRead += bytesRead;
  }

  return totalRead;
}

SSIZE_T recvSize(BOOL& success, HANDLE hRead, size_t& len)
{
  return recv(success, hRead, reinterpret_cast<char*>(&len), sizeof(size_t));
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "child: incorrect args\n");
    return 1;
  }

  HANDLE hRead = reinterpret_cast<HANDLE>(std::strtoull(argv[1], NULL, 10));
  if (hRead == NULL || hRead == INVALID_HANDLE_VALUE)
  {
    fprintf(stderr, "child: incorrect handle\n");
    return 1;
  }

  BOOL success = FALSE;
  size_t len = 0;

  SSIZE_T recvErr = recvSize(success, hRead, len);
  if (!success || recvErr <= 0)
  {
    fprintf(stderr, "child: recvSize error (Error Code: %lu)\n", GetLastError());
    return 1;
  }

  char* line = static_cast<char*>(malloc((len + 1) * sizeof(char)));
  if (!line)
  {
    fprintf(stderr, "child: memory allocation error\n");
    return 1;
  }

  recvErr = recv(success, hRead, line, static_cast<SSIZE_T>(len));
  if (!success || recvErr <= 0)
  {
    fprintf(stderr, "child: recv error (Error Code: %lu)\n", GetLastError());
    free(line);
    return 1;
  }

  line[len] = '\0';

  if (!CloseHandle(hRead))
  {
    fprintf(stderr, "child: handle close error (Error Code: %lu)\n", GetLastError());
    free(line);
    return 1;
  }

  int printErr = printf("%s", line);
  if (printErr < 0)
  {
    fprintf(stderr, "child: printf error\n");
    free(line);
    return 1;
  }

  free(line);
  return 0;
}
