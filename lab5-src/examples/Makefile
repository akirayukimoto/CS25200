all: daytime_server tls_server

daytime_server: daytime_server.c
	$(CC) -o $@ $^

tls_server: tls_server.c
	$(CC) `pkg-config --cflags openssl` -o $@ $^ `pkg-config --libs openssl`
	@echo "Don't forget to create cert.pem and key.pem"
