<p align="center">

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

</p>

POTAD (portable open-source tool (for) all-in-one development) is a small, battery-powered coding machine so you can develop and code on the fly!

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

# build and run:

no pre-prepared `.iso` files are available yet, but you can run it locally:

```bash
# build
mkdir -p build && cd build
cmake..
cmake

# trigger the qemu run script
./scripts/run-qemu.sh
```

---

built by **FlyingFish** and **SwedishSplidney**