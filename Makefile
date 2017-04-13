LIBS = -pthread
CFLAGS = -O3 -Wall -Werror

default:
	gcc $(CFLAGS) $(LIBS) src/*.c -o sha256

clean:
	rm -f ./sha256