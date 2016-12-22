# digimon

Author: meh@HITCON  

### Description  

This service is about an anime called [Digimon](https://en.wikipedia.org/wiki/Digimon).
Players can move within the map and battle with digimons. There are some important commands:  
* a, s, d, w - move, and randomly meet digimons  
* info - show player's info  
* bag  - show player's items
* mons - show status of monsters  
* talk - talk to a NPC if one is nearby  
* task - assign task to a digimon (train/work/rest)  
* notifi - check whether any notification exists  
* item - use item  
* nick - give digimon a nickname (Should use Naming Document first).  

When talk to Home, we can change partner.  
When talk to Shop, we can buy items.  
When talk to Toolmon, we can challenge or order it.  
Toolmon has two functions:  
* Make money  
* Carry player to somewhere  

To make replay attack harder, we add some randomness. Writing an AI before exploiting is neccessary.  
Note: In this service, we use a sandbox to prevent DoS attack. And network connection is also blocked.  

### Vulnerability  

There are several vulnerabilities in this service. Only list part of them.  

* Toolmon  
After beating the Toolmon, player can set the "AI". We can write some C functions and they will be compiled with "[libtcc](http://bellard.org/tcc/)" (static link). The function will be called by a child process in seccomp with all fds closed except for the pipe fds. This bug can leak flag directly or simply leak address and canary and then exploit with other bug.  
The seccomp filter is simple:  
```c  
struct sock_filter filter[] = {
    // check arch
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 4),
    BPF_JUMP(BPF_JMP+BPF_JEQ, 0xc000003e, 0, 4),
    // check syscall
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 0),
    BPF_JUMP(BPF_JMP+BPF_JEQ, 59, 2, 0),
    BPF_JUMP(BPF_JMP+BPF_JEQ, 2, 1, 0),
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL)
};
```  

* Buy items  
The data type of index should be unsigned. It's able to input negative numbers to modify the bss section.  

* Nickname  
The buffer of input is not null terminated. This will cause a null byte off-by-one and lead to heap overflow.  

* Achievements  
The array of achievements can be overflowed.  

### Exploit  

Using the vulnerabilities mentioned above, there are at least five exploits.  

* Bypass the seccomp. For example, using other syscalls to open the flag such as "openat". Of course there are various methods can achieve this. 
* Load the flag at compile time, like "include" the flag directly.  
* Buy items and change the name of monster, which will change the image path. Before buying things, earning lots of money is necessary. And Toolmon can achieve this perfectly. (That's the spirit of Toolmon! :P)  
* Use nickname to trigger heap overflow and change a monster's id and level, which will make the image function read the fake path.  
* Overflow achievements will change the amount of monsters. And then we can make a fake monster.  

We designed one more exploit (and a few more vulnerabilities) actually. It's interesting and a bit harder. We may modify and put this challenge on pwnable.tw (after this site is completed). So we are not going to disclose it right now.

