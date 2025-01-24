debug: src/*.c src/*.h
	clang -std=c99 -g -fsanitize=address,undefined,null -fomit-frame-pointer -Wall -Werror -Wswitch-enum src/*.c -o diamond

release: src/*.c src/*.h
	clang -std=c99 -Wall -Werror -Wswitch-enum -o3 src/*.c -o diamond