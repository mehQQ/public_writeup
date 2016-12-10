# mboard

Author: meh@HITCON  

### Description  
```
mboard
Host : mboard.pwn.seccon.jp
Port : 8273
Execute command : ./mvees_sandbox --replicas=1 --level=2 --out-limit=8192 --deny=11 ./mboard 2>&1
```
This challenge is running in a sandbox. This sandbox runs two processes simultaneously and check the output and syscalls.  
The check of memcpy is unused, so leaking some address is available.  

### Vulnerability  

The vulnerability of mboard is just the same as HeartAttack ... found it immediately. Many vulnerabilities in this CTF are the same as past CTFs. :/  
The vulnerable function:
```c
size_t getnline(char *buf, size_t nbytes)
{
  char *p;
  read(0, buf, nbytes);
  p = strchr(buf, '\n');
  if(p)
    *p = 0;
  return strlen(buf);
}
```
It's pretty obvious because other challenges use "fgets" instead of "read" here.  
The buf is not null-terminated, so the result of strchr may be out of bounds.  

### Exploit  

We use the vul to shrink the chunk size, and then use unlink to get a big chunk.  
The chunks after unlink is like:  
```  
/          freed in unsortbin          \
.------------.---------.---------------.
|   unused   |  entry  |   untracked   |
.------------.---------.---------------.
```  
Then we can add an entry and the message will overwrite the entry in this chunk.  
So we can modify the msg pointer to anywhere and free/change it.  
The main problem of this challenge is the sandbox. We need to keep both processes alive, but using an address will lead the second process to crash because of segfault (invalid access).  
It seems impossible to bypass because we have to use the libc function, and I cannot find a way without using an address. So we decide to brute force the libc address. The architecture is x86, so the possibility of two processes having same libc base is 2^12=4096. Seems acceptable even if this program sleeps a lot. Really hate brute force ...  
To avoid using heap/text/stack address, we put the data on the mmap page and attack with dtor_list.  

First, we register many times to make the list extend, and modify the overflowed entry (normally).  
```  
heap offset
0x758         0x7d8 
.---------------.-----------------------.
|      msg      |         list          |
.---------------.-----------------------.
```  
Then we partially overwrite the msg pointer and make it points to the list. Now, the list is fully controlled by modifying the entry.  
We make a fake entry in the TLS section, just in front of dtor_list, then modify this entry. The behaviour of realloc is the same as malloc when the old pointer is null, so the dtor_list will be set to our msg.  
```c
struct dtor_list
{
  dtor_func func;
  void *obj;
  struct link_map *map;
  struct dtor_list *next;
};
```
Finally, we set the func to somewhere in set_context and obj to the address of our chunk on the mmap page, then exit. And we can do ROP to run shellcode and do open-read-write. It only took a few minutes to get the flag after we changed the filename to flag.txt (connected from Taiwan, 16 threads).  
