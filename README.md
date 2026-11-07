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

### note: potad is currently still in development and is not complete yet :)

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

# software specs:

* buildroot-generated aarch64 linux kernel
* u-boot bootloader configuration
* custom c++ pid 1 process manager
* busybox shell
* virtual filesystems
* cross-compiled neovim, git, make and more
* dhcp network integration
* graceful shutdown (`exit`)
* 

software development led by SwedishSplidney

---

# build and run:

no pre-prepared `.iso` files are available yet, but you can run it locally:
> note: make sure your parent directory doesn't have spaces in it (buildroot will freak out)

* clone the repo:
  ```bash
  git clone https://github.com/swedishsplidney/potad.git
  cd potad
  ```

* build the os image:
  ```bash
  ./scripts/build-app.sh
  ```
> note: setting up buildroot can take a while!

* run in qemu:
  ```bash
  ./scripts/run-qemu.sh
  ```
* or, if you want to test u-boot you can run:
  ```bash
  ./scripts/run-qemu-uboot.sh
  ```
  * then to boot into potadOS, run the following inside the u-boot terminal:
    ```text
    setenv bootcmd "ext4load virtio 0:0 0x40400000 /boot/Image; booti 0x40400000 - ${fdtcontroladdr}"
    setenv bootargs "console=ttyAMA0 root=/dev/vda rw earlycon init=/sbin/potad_init"
    boot
    ```

---

# Hardware

POTAD features a bunch of honestly quite cool hardware. Being a fully featured SBC it can realistically be used to run a full linux distribution for whatever is desired! Of course, the primary use case of POTAD is on-the-go development, even with its own USB OTG ports!

<sup>(get it: USB on-the-go? i don't even know if there is OTG support but hey)</sup>

The following hardware is on the SBC.

### HARDWARE SPECIFICATIONS:
---

* The Allwinner H616 CPU
* 4GB LPDDR4X SDRAM (the Samsung K4UBE3D4AB-MGCL)
* A 16 cm x 8 cm touchscreen OLED
* A 32GB onboard eMMC (the Samsung KLMAG1JETD-B041)

Alongside the SBC's computational hardware is also:
* An SD Card slot
* WiFi
* 2x USB-C ports
* 2x USB-A ports
* exposed gpio
* A custom collapsing mechanical keyboard
* 5000mah Lithium Polimer battery

All of this is encompassed in an 3d printed shell (ASA).

Seeing as this is an SBC, the schematic is quite huge, but I'll try to walk you through it.

#### RTL8723DS-CG

In the top left, you'll see the RTL8723DS-CG. 

<img width="821" height="735" alt="image" src="https://github.com/user-attachments/assets/72b21920-53a6-49c7-8e4b-cb2c3fafb1aa" />

The RTL is a WiFi chip, capable of up to 150mbps transfer speeds, on the 2.4 GHz spectrum. Not much more there is to say about it, I'm afraid, so onto the next. Well other then the fact that it is a six by six millimetre 48 pin WFQFM.

<img width="500" height="500" alt="image" src="https://github.com/user-attachments/assets/3f37f0c9-c151-45b9-bad3-0cca668ae907" />

#### Samsung K4UBE3D4AB-MGCL

Next up is Samsung's K4UBE3D4AB-MGCL. The K4 is a 32 Gigabit / 4GB LPDDR4X SDRAM mobile memory integrated circuit manufactured by Samsung. It boasts a data rate up to 4266 Mbps at a clock frequency of 2.133 GHz.

Probably the most annoying thing about this chip has got to be the fact that it has 200 balls in a 10 x 15 mm FBGA package. This results in it having such a huge schematic symbol, which is just really a pain to work with.

<img width="670" height="870" alt="image" src="https://github.com/user-attachments/assets/3a9c3b7b-3caa-4cce-aebb-70627f8de3e0" />

<img width="503" height="906" alt="image" src="https://github.com/user-attachments/assets/66788180-9633-436d-b0ad-7dd5c68ae474" />

Here's a real life picture of how the ram looks. Keep in mind that on the wider side its *just 1.5 cm* and on the shorter side *approximately one centimeter*.

<img width="653" height="281" alt="image" src="https://github.com/user-attachments/assets/5006bc66-c1ac-45e1-b2ac-5d5336c991e6" />

#### Samsung KLMAG1JETD-B041

Eh two Samsungs in a row? Well I guess yeah, but this Samsung KLMAG1JETD-B041 serves a completely different purpose than the last one. The KLM is a 16 GB, not gigabit but gigabyte, eMMC. eMMCs are non-volatile NAND flashes. They are quite similar to SSDs as in they have no moving parts, but eMMCs have a much lower transfer speed, unfortunately, compared to SSDs. The other side of that is that it's much easier to route a slower chip. No need to worry about picoseconds, just nanoseconds.

<img width="405" height="829" alt="image" src="https://github.com/user-attachments/assets/f4ea92b0-fa30-40c2-9d93-76460921196c" />

Hence, you can see, there are far, far less pins connected for data transfer, and most of the pins are just No Connect or for power.

Here's a real chip as well.

<img width="555" height="268" alt="image" src="https://github.com/user-attachments/assets/0325d597-16a5-432a-b9f8-dfde3644a86e" />

Note that this 153-ball FBGA package is just 11.5 x 13 mm, meaning on both sides it's just barely more than *one individual centimeter*.

#### TFP401PZP

Next up is Texas Instruments' TFP401PZP, a display driver. Google says its a "DVI (Digital Visual Interface) 1.0-compliant digital receiver and deserializer IC" whatever that is. ~~Let me get back to you after researching this.~~

So apparently its a DVI interface IC. Why we have that, I know not, but hey that's cool!

<img width="554" height="626" alt="image" src="https://github.com/user-attachments/assets/b7d48d3b-f2df-4ca8-ab9b-b7b63e8b8753" />

Compared to the other ICs, the TFP is much bigger, at 14 x 14 mm, and well yeah! Here's a real life photo!

<img width="640" height="640" alt="image" src="https://github.com/user-attachments/assets/b4634ce9-a637-4c41-8939-2c1d204d4b69" />

Well yeah as you can see it actually has leads and stuff which is part of the reason why it's almost 1.5 x 1.5 cm in size.

#### USB

And of course, we have USB ports because they are super nice, two of each type!

<img width="1750" height="808" alt="image" src="https://github.com/user-attachments/assets/a3c14ee4-e23e-4811-9d26-314fd9b21623" />

Definitely has got to be the simplest part because of how universal they are!

<sub> Hardware design was led by FlyingFish, accompanied by Madhav and Kush. </sub>

---

# license

potad is licenced under the gplv3 licence

check out `LICENCE.md` for more information

---

built by **FlyingFish** and **SwedishSplidney**
