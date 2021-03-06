#include "findXBE.h"
#include <algorithm>
#include <cstdio>

#ifdef NXDK
#include <windows.h>
#include "xbe.h"
#endif

#define SECTORSIZE 0x1000

int findXBE(std::string const& path, MenuXbe *list) {
  std::string workPath = path;
  if (workPath.back() != '\\') {
    workPath += '\\';
  }
#ifdef NXDK
  std::string searchmask = workPath + "*";
  std::string tmp;
  char xbeName[XBENAMESIZE + 1];
  char *xbeData = static_cast<char*>(malloc(SECTORSIZE));
  FILE* tmpFILE = nullptr;
  WIN32_FIND_DATAA fData;
  HANDLE fHandle = FindFirstFileA(searchmask.c_str(), &fData);
  if (fHandle == INVALID_HANDLE_VALUE) {
    return -1;
  }

  do {
    if (fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      tmp = workPath + fData.cFileName + "\\default.xbe";
      tmpFILE = fopen(tmp.c_str(), "rb");
      if (tmpFILE != nullptr) {
        size_t read_bytes = fread(xbeData, 1, SECTORSIZE, tmpFILE);
        XBE *xbe = (XBE*)xbeData;
        if (xbe->sizeOfHeaders > read_bytes) {
          xbeData = static_cast<char*>(realloc(xbeData, xbe->sizeOfHeaders));
          xbe = (XBE*)xbeData;
          read_bytes += fread(&xbeData[read_bytes], 1, xbe->sizeOfHeaders - read_bytes, tmpFILE);
        }
        if(xbe->type != XBETYPEMAGIC ||
           xbe->baseAddress != BASEADDRESS ||
           xbe->baseAddress > xbe->certAddress ||
           xbe->certAddress + 4 >= (xbe->baseAddress + xbe->sizeOfHeaders) ||
           xbe->sizeOfHeaders > read_bytes) {
          continue;
        }
        XBECert *xbeCert = (XBECert*)&xbeData[xbe->certAddress -
                                              xbe->baseAddress];
        memset(xbeName, 0x00, sizeof(xbeName));
        int offset = 0;
        while(offset < XBENAMESIZE) {
          if (xbeCert->xbeName[offset] < 0x0100) {
            sprintf(&xbeName[offset], "%c", (char)xbeCert->xbeName[offset]);
          } else {
            sprintf(&xbeName[offset], "%c", '?');
          }
          if(xbeCert->xbeName[offset] == 0x0000) {
            break;
          }
          ++offset;
        }
        list->addNode(std::make_shared<MenuLaunch>(xbeName, tmp));
        fclose(tmpFILE);
        tmpFILE = nullptr;
      }
    }
  } while (FindNextFile(fHandle, &fData) != 0);
  free(xbeData);
  FindClose(fHandle);
#else
  const char* mask = "*";
  for (int i = 0; i < 7; ++i) {
    list->addNode(std::make_shared<MenuLaunch>(workPath, mask));
  }
#endif
  return 0;
}
