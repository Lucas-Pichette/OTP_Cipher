################################################################################
# Author: Lucas Pichette
# Date: 4th March 2021
# OSU CS344 W21
################################################################################

CC = gcc
CFLAGS = -std=gnu99
default: separate

separate: enc_server.o enc_client.o dec_server.o dec_client.o keygen.o
    $(CC) $(CFLAGS) -o enc_server enc_server.c
    $(CC) $(CFLAGS) -o enc_client enc_client.c
    $(CC) $(CFLAGS) -o dec_server dec_server.c
    $(CC) $(CFLAGS) -o dec_client dec_client.c
    $(CC) $(CFLAGS) -o keygen keygen.c

enc_server.o: enc_server.c
    $(CC) $(CFLAGS) -c enc_server.c

enc_client.o: enc_client.c
    $(CC) $(CFLAGS) -c enc_client.c

dec_server.o: dec_server.c
    $(CC) $(CFLAGS) -c dec_server.c

dec_client.o: dec_client.c
    $(CC) $(CFLAGS) -c dec_client.c

keygen.o: keygen.c
    $(CC) $(CFLAGS) -c keygen.c