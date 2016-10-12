src = $(wildcard src/*.c)
header = $(wildcard src/*.h)
target = HeartAttack_dbg
target2 = HeartAttack

all: $(target) $(target2)

$(target): $(src) $(header)
	gcc $(src) -o $(target) -lpthread -g

$(target2): $(src) $(header)
	gcc $(src) -o $(target2) -lpthread -s -fPIC -pie -z relro -z now

.PHONY: clean
clean:
	rm $(target) $(target2)

full: $(target2)

