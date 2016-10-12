# Secret Holder

Author: meh@Hitcon  

### Description  
Secret Holder is a pwnable challenge, obviously playing with heap.  
This program can help you with keeping secrets. There are three types of secrets:  
1. Small secret     (size 40)  
2. Big secret       (size 4000)  
3. Huge secret      (size 400000)  
After keeping it, you can choose to Wipe(free) or Renew(update) it.  

### Vulnerability

Bug can be found easily in the wipe function. It doesn't check before free, and neither nullify the pointers after free.
This will cause a double-free problem.

```c
switch(choice)
{
    case 1:
        free(f_ptr);
        f_flag = 0;
        break;
    case 2:
        free(s_ptr);
        s_flag = 0;
        break;
    case 3:
        free(q_ptr);
        q_flag = 0;
        break;
}
```
### Exploit

Exploiting this challenge is much harder than finding the bug because the size is totally limited, and you can't malloc the same size before you free it.
If you have solved some heap problems before, you may know: "If we malloc a huge size (probably about 0x21000), it will call mmap directly."
That is true ... but not in this challenge :P  
If you free the huge secret and malloc it again, you will find something interesting ... it goes to the heap section!
I believe this can be found easily using ltrace or debuggers, unless you are misled by yourself XD. Cause the HUGE secret is soooooo big that you should notice rather than ignore it.  
The behavior change because of the mmap threshold, ptmalloc use dynamic threshold to avoid fragmentation. You can see it in details here: https://github.com/lattera/glibc/blob/master/malloc/malloc.c#L963  
With this finding, we can solve this problem easily. Just arrange the chunks, trigger unlink and use the pointers to modify GOT.

This characteristic was found by my teammate when I was testing Secret Holder(Sleepy Holder now), and I decided to split this challenge.  
Hope you guys enjoy this challenge, although it is only 100 pts :(
