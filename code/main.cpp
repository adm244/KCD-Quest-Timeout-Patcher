/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

/*
  NOTE: all addresses are specified for WHGame.dll (md5: AA71E438D0CAEBB081E4945633283E9D) game version of 1.8.2.0
  
  Preventing quests from failing by a timeout (autocomplete_timeout):
    - Patch "__int64 __fastcall IsTimedOut(QuestObjectiveTimer *timer)" at RVA:0xFB0970 to always return 0
    NOTE: quests that are programmed to fail will still fail (i.e. q_ratajTournament)
  
  Displaying a timeleft for timed quests (autocomplete_timeout):
    - Calculate timeleft:
      - Get current time:
        - Get C_Calendar object by calling "C_Calendar *__fastcall C_Calendar__GetCalendar()" at RVA:0x36EB58
        - Get either GameTime or WorldTime (based on a quest timer flag) at [C_Calendar+0x8] or [C_Calendar+0x18]
      - Get quest EndTime:
        - TODO: Get C_QuestObjective object
        - Get AutoCompleteTimeout at [C_QuestObjective+0xA8]
        - Get EndTime at [QuestObjectiveTimer+0x8]
        - Get timer flags at [QuestObjectiveTimer+0x0]
      - Calculate EndTime - CurrentTime (consider timer type, world or game time)
    - Display timeleft:
      - TODO: ...
    
    ObjectiveTimer flags:
      GameTime = (1 << 3), //0x8
      WorldTime = (1 << 4), //0x10
*/

#include <windows.h>

#define PSAPI_VERSION 1
#include <psapi.h>

#include <stdio.h>

#define internal static

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef float real32;
typedef double real64;

typedef long long pointer;
typedef unsigned int uint;

typedef uint32 bool32;

internal uint64 FindSignature(uint64 start, uint64 size, char *pattern, char *mask, uint64 offset)
{
  uint64 startAddress = start;
  uint64 blockSize = size;
  uint64 patternSize = strlen(mask);
  uint64 endAddress = startAddress + blockSize - patternSize;

  for (uint64 i = startAddress; i < endAddress; ++i) {
    bool matched = true;
    for (uint64 j = 0; j < patternSize; ++j) {
      if (mask[j] == '?') continue;
      if (pattern[j] != *((char *)(i + j))) {
        matched = false;
        break;
      }
    }
    
    if (matched) return (i + offset);
  }
  
  return 0;
}

internal bool PatchWHGame(char *filepath, bool removePatch)
{
  // 0) Check if filename if WHGame.dll
  const char *whgame_filename = "WHGame.dll";
  char *exename_start = strrchr(filepath, '\\');
  exename_start = (!exename_start) ? filepath : exename_start + 1;
  if (strncmp(whgame_filename, exename_start, 50)) {
    printf("Wrong file was specified.\n\tExpected: %s\n\tGot: %s\n", whgame_filename, exename_start);
    return false;
  }
  
  // 1) Open file
  HANDLE whgame = CreateFileA(filepath, GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (whgame == INVALID_HANDLE_VALUE) {
    //TODO(adm244): check for ERROR_FILE_NOT_FOUND
    printf("Cannot open \"%s\" file\n", filepath);
    return false;
  }
  
  // 1.1) Get file size
  LARGE_INTEGER filesize = {0};
  if (!GetFileSizeEx(whgame, &filesize)) {
    printf("Cannot get file size of \"%s\"\n", filepath);
    return false;
  }
  
  // 2) Create memory-mapped file
  HANDLE file = CreateFileMappingA(whgame, 0, PAGE_READWRITE, 0, 0, 0);
  if (!file) {
    printf("Cannot create memory-mapped file of \"%s\"\n", filepath);
    return false;
  }
  
  // 3) Create view of a file
  void *filebase = MapViewOfFile(file, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (!filebase) {
    printf("Cannot create a view of a memory-mapped file of \"%s\"\n", filepath);
    return false;
  }
  
  // 4) Apply patch
  
  // 4.1) Get IsObjectiveTimedOut address
  uint64 IsObjectiveTimedOutAddr = FindSignature((uint64)filebase, filesize.QuadPart,
    "\x5B\xC3\xA8\x04\x75\xF4\xA8\x08\x74\x16",
    "xxxxxxxxxx", -0x15);
  if (!IsObjectiveTimedOutAddr) {
    printf("Cannot find address of \"IsObjectiveTimedOut\" in \"%s\"\n", filepath);
    return false;
  }
  
  /*
    BEFORE:
      000007FEE1DD0970 | 40:53      | push rbx     |
      000007FEE1DD0972 | 48:83EC 20 | sub rsp,0x20 |
    AFTER:
      000007FEE1DD0970 | 30C0       | xor al,al    |
      000007FEE1DD0972 | C3         | ret          |
      000007FEE1DD0973 | CC         | int3         |
      000007FEE1DD0974 | CC         | int3         |
      000007FEE1DD0975 | CC         | int3         |
  */
  
  // 4.2) Check if address is valid
  const uint64 _funcsig_unpatched = 0x20EC83485340;
  const uint64 _funcsig_patched = 0xCCCCCCC3C030;
  uint64 _funcsig = (removePatch) ? _funcsig_patched : _funcsig_unpatched;
  uint64 _funcsig_actual = (*((uint64 *)IsObjectiveTimedOutAddr) & 0xFFFFFFFFFFFF);
  if (_funcsig != _funcsig_actual) {
    printf("Function signatures doesn't match:\n\tExpected: 0x%llX\n\tGot: 0x%llX\n\tAt: 0x%llX\n", _funcsig, _funcsig_actual, IsObjectiveTimedOutAddr);
    return false;
  }
  
  // 4.3) Patch function
  uint8 *p = (uint8 *)IsObjectiveTimedOutAddr;
  
  if (removePatch) {
    p[0] = 0x40; p[1] = 0x53; // push rbx
    p[2] = 0x48; p[3] = 0x83; p[4] = 0xEC; p[5] = 0x20; // sub rsp, 0x20
  } else {
    p[0] = 0x30; p[1] = 0xC0; // xor al, al
    p[2] = 0xC3; // ret
    p[3] = 0xCC; // int3
    p[4] = 0xCC; // int3
    p[5] = 0xCC; // int3
  }
  
  // 4.4) Check if patched correctly
  _funcsig = (removePatch) ? _funcsig_unpatched : _funcsig_patched;
  uint64 _funcsig_patched_actual = (*((uint64 *)IsObjectiveTimedOutAddr) & 0xFFFFFFFFFFFF);
  if (_funcsig != _funcsig_patched_actual) {
    printf("Function signatures after patch doesn't match:\n\tExpected: 0x%llX\n\tGot: 0x%llX\n\tAt: 0x%llX\n", _funcsig_patched, _funcsig_patched_actual, IsObjectiveTimedOutAddr);
    return false;
  }
  
  // 5) Flush memory-mapped file
  if (!FlushViewOfFile(filebase, 0)) {
    printf("Cannot flush memory-mapped file of \"%s\"\n", filepath);
    return false;
  }
  
  return true;
}

internal void ShowUsage(char *exepath)
{
  char exename[50] = {0};
  char *exename_start = strrchr(exepath, '\\');
  exename_start = exename_start ? exename_start + 1 : exepath;
  strncpy(exename, exename_start, 50);
  
  printf("Usage: %s <command> <filepath>\n\t<command> - patch|unpatch\n\t<filepath> - a path to WHGame.dll", exename);
}

int main(int argc, char *argv[])
{
  if (argc == 3) {
    const char *cmd_patch = "patch";
    const char *cmd_unpatch = "unpatch";
  
    char *cmd = argv[1];
    char *filepath = argv[2];
    
    bool removePatch = false;
    if (strncmp(cmd, cmd_patch, strlen(cmd_patch))) {
      if (strncmp(cmd, cmd_unpatch, strlen(cmd_unpatch))) {
        ShowUsage(argv[0]);
        return 0;
      } else {
        removePatch = true;
      }
    }
    
    //printf("%s\n", removePatch ? "unpatch" : "patch");
    
    if (PatchWHGame(filepath, removePatch)) {
      printf("Successfully %s \"%s\"", removePatch ? "unpatched" : "patched", filepath);
    } else {
      printf("Failed to %s \"%s\"", removePatch ? "unpatch" : "patch", filepath);
    }
  } else {
    ShowUsage(argv[0]);
  }
  
  return(0);
}
