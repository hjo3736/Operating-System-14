#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  char *fn_copy;

  switch (f->R.rax) {
    case SYS_HALT:
      halt();
      break;

    case SYS_EXIT:
      exit(f->R.rdi);
      break;

    case SYS_EXEC:
      if (exec(f->R.rdi) == -1) {
        exit(-1);
      }
      break;

    case SYS_WAIT:
      f->R.rax = process_wait(f->R.rdi, f);
      break;

    case SYS_CREATE:
      f->R.rax = create(f->R.rdi, f->R.rsi);
      break;

    case SYS_REMOVE:
      f->R.rax = remove(f->R.rdi);
      break;

    case SYS_OPEN:
      f->R.rax = open(f->R.rdi);
      break;

    case SYS_FILESIZE:
      f->R.rax = filesize(f->R.rdi);
      break;
    
    case SYS_READ:
      f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
      break;

    case SYS_WRITE:
      f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
      break;

    case SYS_SEEK:
      seek(f->R.rdi, f->R.rsi);
      break;

    case SYS_TELL:
      f->R.rax = tell(f->R.rdi);
      break;

    case SYS_CLOSE:
      close(f->R.rdi);
      break;

    default:
      exit(-1);
      break;

  }

}

void
halt(void) {

  power_off();

}

void exit (int status) {

  struct thread *cur = thread_current();
  cur->exit_status = status;

  printf("%s : exit(%d)\n", thread_name(), status);
  thread_exit();

}

int
exec(char *file_name) {

  check_address(file_name) + 1;

  int file_size = strlen(file_name) + 1;
  char *fn_copy = palloc_get_page(PAL_ZERO);

  if(fn_copy == NULL) {

    return -1;

  }

  NOT_REACHED();
  return 0;

}

void
check_address(const uint64_t *addr) {

  struct thread *cur = thread_current();
  if(addr == NULL || !(is_user_vaddr(addr)) || pml4_get_page(cur->pml4, addr) == NULL) {

    exit(-1);

  }

}

bool
create(const char *file, unsigned initial_size) {

  check_address(file);
  return filesys_create(file, initial_size);

}

bool
remove(const char *file) {

  check_address(file);
  return filesys_remove(file);

}

int
open(const char *file) {

  check_address(file);
  struct file *open_file = filesys_open(file);

  if(open_file == NULL) {

    return -1;

  }

  int fd = add_file_to_fdt(open_file);

  if(fd == -1) {
  
    file_close(open_file);

  }
  return fd;

}

int add_file_to_fdt(struct file *file) {

  struct thread 8cur = thread_current();
  struct file **fdt = cur->fd_table;

  while(cur->fd_idx < FDCOUNT_LIMIT && fdt[cur->fd_idx]) {

    cur->fd_idx++;

  }

  if(cur->fd_idx >= FDCOUNT_LIMIT) {

    return -1;

  }

  fdt[cur->fd_idx] = file;
  return cur->fd_idx;

}

int
filesize(int fd) {

  struct file *open_file = find_file_by_fd(fd);
  if(open_file == NULL) {

    return -1;

  }
  return file_length(open_file);

}

static struct file
*find_file_by_fd(int fd) {

  struct thread *cur = thread_current();

  if(fd < 0 || fd >= FDCOUNT_LIMIT) {

    return NULL;

  }
  return cur->fd_table[fd];

} 

int
read(int id, void *buffer, unsigned size) {

  check_address(buffer);
  
  int read_result;
  struct thread *cur = thread_current();
  struct file *file_fd = find_file_by_fd(fd);

  if(fd == 0) {

    *(char *)buffer = input_getc();
    read_result = size;

  }else {
 
    if(find_file_by_fd(fd) == NULL) {

      return -1;

    }else {

      lock_acquire(&filesys_lock);
      read_result = file_read(find_file_by_fd(fd), buffer, size);
      lock_release(&filesys_lock);

    }

  }
  return read_result;

}

int
write(int fd, const void *buffer, unsigned size) {

  check_address(buffer);

  int write_result;
  lock_acquire(&filesys_lock);
  if(fd == 1) {

    putbuf(buffer, size);
    write_result = size;

  }else {

    if(find_file_by_fd(fd) != NULL) {

      write_result = file_write(find_file_by_fd(fd), buffer, size);

    }else {

      write_result = -1;

    }

  }
  lock_release(&filesys_lock);
  return write_result;

}

void
seek(int fd, unsigned position) {

  struct file *seek_file = find_file_by_fd(fd);

  if(seek_file <= 2) {

    return;

  }

  seek_file->pos = position;

}

unsigned
tell(int fd) {

  struct file *tell_file = find_file_by_fd(fd);

  if(tell_file <= 2) {

    return;

  }
  return file_tell(tell_file);

}

void
close(int fd) {

  syruct file *fileobj = find_file_by_fd(fd);

  if(fileobj == NULL) {

    return;

  }
  
  remove_file_from_fdt(fd);

}


