FROM archlinux:base

RUN patched_glibc=glibc-linux4-2.33-4-x86_64.pkg.tar.zst && \
    curl -LO "https://repo.archlinuxcn.org/x86_64/$patched_glibc" && \
    bsdtar -C / -xvf "$patched_glibc"
RUN echo "Server = http://ftp.jaist.ac.jp/pub/Linux/ArchLinux/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist
RUN pacman -Syuq --noconfirm gcc make binutils glibc git