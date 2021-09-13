######## r0memdump ########
ring 0 memory dumper for linux

-- usage --
simply read/write /proc/<pid> to directly read and modify its memory
(pid being the pid of the proc u want to fuck with>)

examples:
hexdumping memory --  hexdump -C /proc/<pid>
memory scanning -- grep "string" /proc/<pid>
memory editing -- dd if=payload of=/proc/<pid> skip=offset

and more !

-- why --
its intended use is for avoiding anticheat/antiviruses
as r0memdump doesnt use ptrace, or any other suspicious syscalls

greetz jimmy ty for the idea
