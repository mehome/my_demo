
all:
	gcc -Wall -O2 -o msign base64.c  ed25519.c  edsign.c  f25519.c  fprime.c  main.c  sha512.c encrypt.c

compat:
	gcc -Wall -O2 -DCOMPACT_VERSION -o msign base64.c  ed25519.c  edsign.c  f25519.c  fprime.c  main.c  sha512.c encrypt.c

clean:
	rm -f msign *.o *~

