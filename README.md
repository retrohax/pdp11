# Fun with the PDP-11

## Overview

This project demonstrates connecting a vintage PDP-11 computer running Research Unix V7 to the modern internet for performing API calls to web services. It uses an ESP32 microcontroller running the "hiterm" firmware as a serial-to-TCP bridge to provide internet connectivity to the PDP-11.

The project includes tools for sending HTTP requests over the serial connection (`req.c`) and parsing JSON responses (`jsox.c`). Example usage is provided for interacting with the Alertra website monitoring API to fetch and update device configurations.

The README provides step-by-step instructions for setting up SIMH PDP-11 simulation, booting Research Unix V7, modifying device drivers, and establishing the serial connection to the ESP32.



### Install SIMH/PDP-11

```bash
cd
git clone https://github.com/simh/simh.git
cd simh
make pdp11
```

### Boot the PDP-11

```bash
cd
git clone https://github.com/narukeh/research_unix_v7.git
cd research_unix_v7
../simh/BIN/pdp11 ../nboot.ini
boot
hp(0,0)unix
ctrl+d (starts multi-user)
login root/root
echo 'stty erase "^h" kill "^u" nl0 cr0' 9600 > .profile
```

### Modify DZ driver

Update the DZ driver so that /dev/tty00 (line=0 in nboot.ini) will not wait for carrier detect.

```bash
cd /usr/sys/dev

ed dz.c
67a
	if (dev == 0)
		tp->t_state |= CARR_ON;
.
w
q

cc -c -O dz.c
ar r LIB2 dz.o
```

### Enable DZ driver

Make darn sure the tabs get pasted in correctly.

```bash
cd /usr/sys/conf
ed mkconf.c
249a
	"dz",
	0, 300, CHAR+INTR,
	"	dzin; br5+%d.\n	dzou; br5+%d.\n",
	".globl	_dzrint\ndzin:	jsr	r0,call; jmp _dzrint\n",
	".globl	_dzxint\ndzou:	jsr	r0,call; jmp _dzxint\n",
	"",
	"	dzopen, dzclose, dzread, dzwrite, dzioctl, nulldev, dz_tty,",
	"",
	"int	dzopen(), dzclose(), dzread(), dzwrite(), dzioctl();\nstruct	tty	dz_tty[];",
.
45a
	"dz",
.
w
q

cc -o mkconf mkconf.c
cp hphtconf myconf
echo "dz" >> myconf
mkconf < myconf
rm *.o
make
mv unix /munix
```

### Create the DZ ttys

```bash
/etc/mknod /dev/tty00 c 19 0
/etc/mknod /dev/tty01 c 19 1
/etc/mknod /dev/tty02 c 19 2
/etc/mknod /dev/tty03 c 19 3
/etc/mknod /dev/tty04 c 19 4
/etc/mknod /dev/tty05 c 19 5
/etc/mknod /dev/tty06 c 19 6
/etc/mknod /dev/tty07 c 19 7
/etc/mknod /dev/tty08 c 19 8
/etc/mknod /dev/tty09 c 19 9
/etc/mknod /dev/tty10 c 19 10
/etc/mknod /dev/tty11 c 19 11
/etc/mknod /dev/tty12 c 19 12
/etc/mknod /dev/tty13 c 19 13
/etc/mknod /dev/tty14 c 19 14
/etc/mknod /dev/tty15 c 19 15
```

### Update /etc/ttys

This is so unix will start shells on telnet connections. We are not changing 00tty00 because it is reserved for the outgoing (notelnet) serial connection, we don't want a shell started for it.

```bash
ed /etc/ttys
3,16s/00tty/12tty/
w
q
```

### Restart with new kernel

```bash
sync
sync
sync
sync
ctrl+e
sim> quit
$ ../simh/BIN/pdp11 ../nboot.ini
boot
hp(0,0)munix
```

### Connect ESP32 to PDP-11 serial

Connect an ESP32 running *hiterm* (https://github.com/retrohax/hiterm) to USB and make sure Linux is seeing it.

I have not been able to get simh to forward serial from the PDP-11 properly so just start this socat bridge:

```bash
socat -v \
    TCP-LISTEN:6500,reuseaddr,fork \
    file:/dev/ttyUSB0,raw,nonblock,echo=0,b9600,cs8,parenb=0,cstopb=1
```

### Connect a terminal to PDP-11

```bash
socat PTY,link=/tmp/v7_terminal TCP:localhost:6501
```

```bash
minicom -D /tmp/v7_terminal
```

### Copy files to PDP-11

Set minicom character tx delay to 5ms (ctrl+a tf).

```bash
cd /usr/dmr
cat > req.c
# paste in the code
ctrl+d
cc -o req req.c
# repeat for jsox.c
```

Copy alertra/dupdate.sh and all the .req files.

Test:

```bash
cat hello.req | req api.alertra.com 443 use_tls
```
