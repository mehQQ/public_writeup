#Sleepy Holder

Author: meh@Hitcon  

### Description  

This challenge only removes the "free" of huge secret originally. But I made it "Sleepy" after I found some teams brute forcing the heap base. It is impossible to brute force before ctf ends unless you are pretty lucky.  
The differences between the Secret and Sleepy are:  
1. Sleep and random malloc to prevent brute force  
2. No update and wipe for huge secret  
3. Huge secret can be kept only once  
Apparently, if you can solve Sleepy, you can solve Secret with the same method.  
I released a hint: "malloc consolidate" when the ctf was 9 hours left.

### Exploit

This challenge is about another myth of ptmalloc: "the freed chunks with size 0x20 to 0x80 are always in fastbin."  
Again, this is not true in this challenge. :P  
Searching for the hint in malloc.c, you may find what I want to hint(from the weird design of huge secret):
https://github.com/lattera/glibc/blob/master/malloc/malloc.c#L3397  
When there is a large request(largebin size is enough) of malloc, it will do consolidating first in order to prevent fragmentation problem.
Every fastbin is moved the unsortbin, consolidates if possible, and finally goes to smallbin.  
So I make a chunk in the fastbin(an inused big secret beneath it to avoid consolidating into top chunk), and trigger the malloc consolidate by keeping a huge secret.
After the chunk goes into smallbin, it is able to pass the double-free check because the check only compares it with the fasttop(putting chunk into fastbin will not change the inuse bit because the fastbin do not consolidate usually).
Then we can free it and malloc it again to modify the prev_size and create a fake chunk for unlink. The remaining is just the same as Secret.  
Finding this characteristic out is not so easy without some tools related to heap exploit. Maybe it's time to install some tools like pwngdb(made by angelboy)? :P   

The first blood went to !SpamAndHex when the ctf left about only two hours.
At that time I was just checking the traffic and found the service pwned. 
So many congrats and thanks because I was so upset and worrying no one would solve this challenge, it was such a torture. XD  

btw, the flag is: hitcon{The Huuuuuuuuuuuge Secret Really MALLOC a difference!}  
That is true, isn't it? :)

