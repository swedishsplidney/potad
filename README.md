```text
                                        dP                  dP
                                        88                  88
                    88d888b. .d8888b. d8888P .d8888b. .d888b88
                    88'  `88 88'  `88   88   88'  `88 88'  `88
                    88.  .88 88.  .88   88   88.  .88 88.  .88
                    88Y888P' `88888P'   dP   `88888P8 `88888P8
                    88
                    dP       portable, open-source, tool (for) all-in-one development!
```

POTAD (portable open-source tool (for) all-in-one development) is a small, battery-powered coding machine so you can develop and code on the fly!

note: potad is currently still in development and is not complete yet :)

---

# overview:

potad is a custom, portable, folding handheld device intended to run neovim (and other development tools)

it runs a custom linux-based distro designed using buildroot for lightweight efficiency

### features:

* fully custom single board computer based around the allwinner h616
* custom buildroot linux distro
* folding form factor with collapsing mechanical keyboard and oled touchscreen
* pre-prepared nvim, git, make, and more

---

# hardware specs:

* allwinner h616
* 4gb lpddr4
* 160x80mm oled touchscreen
* 32gb emmc + sd card slot
* custom collapsing mechanical keyboard
* 5000mah lipo battery
* wifi, usbc, usb, exposed gpio
* asa 3d printed shell

hardware design led by FlyingFish

---

# software specs:

* buildroot-generated aarch64 linux kernel
* custom c++ pid 1 process manager
* busybox shell
* virtual filesystems
* cross-compiled neovim, git, make and more
* dhcp network integration
* graceful shutdown (`exit`)

software development led by SwedishSplidney

---

# build and run:

no pre-prepared `.iso` files are available yet, but you can run it locally:

```bash
# build
mkdir -p build && cd build
cmake ..
cmake

# trigger the qemu run script
./scripts/run-qemu.sh
```

---

# license

potad is licenced under the gplv3 licence

check out `LICENCE.md` for more information

---

built by **FlyingFish** and **SwedishSplidney**