# NOTE: This hash is pinned to avoid version and environment differences.
FROM ubuntu:24.04@sha256:186072bba1b2f436cbb91ef2567abca677337cfc786c86e107d25b7072feef0c

RUN apt-get update && \
    apt-get install -y --no-install-recommends xinetd libseccomp2 && \
    rm -rf /var/lib/apt/lists/*

RUN groupadd -r pwn && useradd -r -g pwn -d /home/pwn -s /usr/sbin/nologin pwn && \
    mkdir -p /home/pwn

RUN cat > /etc/xinetd.d/pwn << 'EOF'
service pwn
{
  type           = UNLISTED
  disable        = no
  socket_type    = stream
  protocol       = tcp
  wait           = no
  user           = pwn
  bind           = 0.0.0.0
  port           = 1337
  server         = /usr/bin/timeout
  server_args    = 180 /home/pwn/chal
}
EOF

RUN chmod 444 /etc/xinetd.d/pwn

WORKDIR /home/pwn

COPY --chmod=550 chal /home/pwn/chal
COPY --chmod=444 flag.txt /flag.txt

RUN chown -R root:pwn /home/pwn && \
    chown root:root /flag.txt

EXPOSE 1337

CMD ["xinetd", "-dontfork"]