#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
//单位是M
#define MAX_MEM_CNT 1024 * 1024  //内存池容量
#define MIN_DO_BATCH 1024        //触发平衡操作的最小差值
using namespace std;

typedef struct MEMPACKED {
  char name1[20];
  unsigned long MemTotal;
  char name2[20];
  unsigned long MemFree;
  char name3[20];
  unsigned long Buffers;
  char name4[20];
  unsigned long Cached;
  char name5[20];
  unsigned long Available;  // MemAvailable

} MEM_OCCUPY;

typedef struct os_line_data {
  char *val;
  int len;
} os_line_data;

static char *os_getline(char *sin, os_line_data *line, char delim) {
  char *out = sin;
  if (*out == '\0') return NULL;
  //	while (*out && (*out == delim)) { out++; }
  line->val = out;
  while (*out && (*out != delim)) {
    out++;
  }
  line->len = out - line->val;
  //	while (*out && (*out == delim)) { out++; }
  if (*out && (*out == delim)) {
    out++;
  }
  if (*out == '\0') return NULL;
  return out;
}
int Parser_EnvInfo(char *buffer, int size, MEM_OCCUPY *lpMemory) {
  int state = 0;
  char *p = buffer;
  while (p) {
    os_line_data line = {0};
    p = os_getline(p, &line, ':');
    if (p == NULL || line.len <= 0) continue;

    if (line.len == 8 && strncmp(line.val, "MemTotal", 8) == 0) {
      char *point = strtok(p, " ");
      memcpy(lpMemory->name1, "MemTotal", 8);
      lpMemory->MemTotal = atol(point);

    } else if (line.len == 7 && strncmp(line.val, "MemFree", 7) == 0) {
      char *point = strtok(p, " ");
      memcpy(lpMemory->name2, "MemFree", 7);
      lpMemory->MemFree = atol(point);
    } else if (line.len == 7 && strncmp(line.val, "Buffers", 7) == 0) {
      char *point = strtok(p, " ");
      memcpy(lpMemory->name3, "Buffers", 7);
      lpMemory->Buffers = atol(point);
    } else if (line.len == 6 && strncmp(line.val, "Cached", 6) == 0) {
      char *point = strtok(p, " ");
      memcpy(lpMemory->name4, "Cached", 6);
      lpMemory->Cached = atol(point);
    } else if (line.len == 12 && strncmp(line.val, "MemAvailable", 12) == 0) {
      char *point = strtok(p, " ");
      memcpy(lpMemory->name5, "MemAvailable", 12);
      lpMemory->Available = atol(point);
    }
  }
}

int get_procmeminfo(MEM_OCCUPY *lpMemory) {
  FILE *fd;
  char buff[128] = {0};
  fd = fopen("/proc/meminfo", "r");
  if (fd == NULL) return -1;
  fgets(buff, sizeof(buff), fd);
  Parser_EnvInfo(buff, sizeof(buff), lpMemory);

  fgets(buff, sizeof(buff), fd);
  Parser_EnvInfo(buff, sizeof(buff), lpMemory);

  fgets(buff, sizeof(buff), fd);
  Parser_EnvInfo(buff, sizeof(buff), lpMemory);

  fgets(buff, sizeof(buff), fd);
  Parser_EnvInfo(buff, sizeof(buff), lpMemory);

  fclose(fd);
}
unsigned long pool_malloc(char **base, unsigned long n, unsigned long ultodo) {
  for (unsigned long i = 0; i < ultodo; i++) {
    if (n + i >= MAX_MEM_CNT) {
      return n + i;
    }
    *(base + n + i) = new char[1024 * 1024];
    memset(*(base + n + i), 0, 1024 * 1024);
    // std::cout<<i<<std::endl;
  }

  return n + ultodo;
}
unsigned long pool_free(char **base, unsigned long n, unsigned long ultodo) {
  for (unsigned long i = 0; i < ultodo; i++) {
    if (n - i == 0 || n < i) {
      return 0;
    }
    if (*(base + n - i) != NULL) {
      delete[] * (base + n - i);
      *(base + n - i) = NULL;
    };
  }
  return n - ultodo;
}
int main() {
  MEM_OCCUPY mo = {0};
  char **p = new char *[MAX_MEM_CNT];
  unsigned long cnt = 0;
  unsigned long mTotal = 0;
  unsigned long mFree = 0;
  unsigned long mUsed = 0;
  unsigned long mAvailable = 0;
  unsigned long mTodo = 0;

  while (true) {
    get_procmeminfo(&mo);
    std::cout << mo.MemTotal << std::endl;
    std::cout << mo.MemFree << std::endl;
    //单位是M
    mTotal = mo.MemTotal / 1024;
    mFree = mo.MemFree / 1024;
    mAvailable = mo.Available / 1024;
    // mUsed = mTotal - mFree;
    mUsed = mTotal - mAvailable;
    if (mUsed < mTotal / 2) {
      mTodo = mTotal / 2 - mUsed;
      std::cout << "malloc mTodo : " << mTodo << std::endl;
      if (mTodo > MIN_DO_BATCH) {  //有1024M的空间才操作
        cnt = pool_malloc(p, cnt, mTodo);
      }

    } else {
      mTodo = mUsed - mTotal / 2;
      std::cout << "free mTodo : " << mTodo << std::endl;
      if (mTodo > MIN_DO_BATCH) {
        cnt = pool_free(p, cnt, mTodo);
      }
    }
    std::cout << "cnt" << cnt << std::endl;
    sleep(30);
  }
}
