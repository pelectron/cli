#include <cstddef>

extern "C" {
extern char _end;
static char *heap_end = NULL;

void *_sbrk(ptrdiff_t incr) {
  char *prev_heap_end;
  if (heap_end == NULL) {
    heap_end = &_end;
  }
  prev_heap_end = heap_end;

  // Check for stack collision here if necessary
  heap_end += incr;
  return (void *)prev_heap_end;
}

// Minimal stubs for I/O functions
int _write(int file, char *ptr, int len) {
  // Retarget to UART or other peripheral here
  return len;
}

int _read(int file, char *ptr, int len) { return 0; }
int _close(int file) { return -1; }
int _fstat(int file, struct stat *st) { return 0; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
void _exit(int status) {
  while (1)
    ;
}
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { return -1; }
}
