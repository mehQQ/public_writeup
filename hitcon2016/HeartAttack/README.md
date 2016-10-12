# Heart Attack

Author: meh@Hitcon  

### Description  

Heart Attack is a common card game in Taiwan, and I made a socket server for this game. Players can chat and play games together on this server. The binary is slightly bigger than other pwnable challenges, which makes it harder to reverse. But actually the huge structure on the shared memory is not important at all. lol

### Vulnerability

There are two bugs in this challenge, one is totally unexpected. I found that bug after looking into the payload of Shellphish.  

* Null byte off-by-one  
The bug is in the card_name function. This function generation the name(e.g. ♠   Spades  A   ♠) on the heap and return the address.
It looks normal if you only count the characters(or you probably close it immediately after open it XD). But when you use debugger to see the memory, you will find that every ♠ consists of three bytes.
As a result, when the "Diamonds 10" is flipped, the bug triggered.

```c
char *card_name(unsigned int n)
{
    char *buf = (char *)calloc(1, 24);
    char num[4];
    switch(n%13)
    {
        // set number
        case 0:
            strcpy(num, "A");
            break;
        // ...
    }

    switch(n/13)
    {
        // set suit
        case 0:
            sprintf(buf, "♠   Spades  %s   ♠", num);
            break;
        // ...
    }
    return buf;
}
```

* Newline removing out of bound  
This bug is totally a BUG in my program ... didn't notice that at all.  
It is located in the leave_msg function. I use a function to remove newline of messages which will iterate the buffer and change the newline to null byte(to avoid double newlines which makes the output ugly). This function ends when it reaches a newline or null byte. 
And I make a mistake in the position of function call. I call it "before" I set the last byte to null byte. 
So when the function is called, the buffer may be full and there is no null byte between it and the size of next chunk.
It you prepare a size like 0xaf0 there, the 0x0a will be changed to 0 and the size become 0xf0.  
This bug is very interesting and hard to find, although it is not intended.
The only thing makes me sad is, they don't need to play the game to exploit it ... really want someone to play Heart Attack with me. :(

```c
    read(sockfd, new->str, end_size);
    remove_line(new->str);
    new->str[end_size-1] = '\0';
```

### Exploit
Exploiting the first bug is slightly harder and annoying because you don't know when the magic card is flipped. And playing a game is such a trouble. XD  
Despite this, the exploit doesn't vary greatly because both method "shrink" the chunk.
To arrange heap easier, I create four players with same heap arrangement(You can find the method here: http://www.slideshare.net/AngelBoy1/advanced-heap-exploitaion p26-41).
After that, the four players play a game and check whether the magic card appears when they flip the first card. So the possibility of success is about 1/13.  

An interesting thing of this challenge is that I carefully make the stack unusable when using normal fastbin corruption. The buffer is cleared, and no size like 0x40xxxx or even 0x7fxxxxxx can be used. Because calloc takes 8 bytes as chunk size when doing memset.  
There is a little trick in my exploit to solve this. I use fastbin corruption to leave a fake chunk size "inside" the main arena(being the fasttop). And then do corruption again to gain a chunk inside the main arena.  
I can change the pointer of top chunk(just beneath the fastbins) directly in this way. And there is only one check in malloc when using top chunk(whether the size is big enough). So I can simply make the top chunk point to the (saved rbp-8). Then it will consider saved rbp as the top size. And the chunk I get will locate in the position of return address. Just do ROP easily.  
There are many solutions of it. PPP and Shellphish both solve this with pretty amazing method. And after reading the writeup of ricky, I think he did this challenge much better than me(the heap arrangement is godlike!). Learn a lot from these teams. :) 
