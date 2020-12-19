FROM archlinux:base

RUN echo "Server = http://ftp.jaist.ac.jp/pub/Linux/ArchLinux/$repo/os/$arch" > /etc/pacman.d/mirrolist
RUN pacman -Syuq --noconfirm gcc make binutils glibc git