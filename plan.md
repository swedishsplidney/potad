# good and not bad plan frfr

---

## parts list:

(lcsc preferred for any raw chips)

* allwinner h616 (cpu)
* some kind of 32gbit (4GB) lpddr4 bga chip (micron MT53D1024M32D4DT-046 WT:D equivalent)
* realtek rtl8211f-cg (wifi)
* samsung klmbg2jetd-b041 (storage)
* adafruit 2500mah lipoly
* adafruit micro-lipo charger for lipoly battery (also functions as usbc data breakout)
* asa 3dp filament

---

## timeline:

(not final, just for reference)

| week   | swedish                                | flyingfish                                 |
|--------|----------------------------------------|--------------------------------------------|
| week 1 | qemu buildroot & kernel boot           | basic 3d model / pcb                       |
| week 2 | passwordless login & busybox           | full pcb schemadic                         |
| week 3 | nvim & git cross-compling              | blulk of the pcb                           |
| week 4 | bake lazyvim plugins & configs         | refine pcb and do all the checks           |
| week 5 | device tree (.dts) layout              | case cad modeling and smd component prep   |
| week 6 | console input matrix keymap driver     | recieve boards / solder power + SoC        |
| week 7 | wifi stack, wpa supplicant & dhcp      | solder remaining parts, power rail testing |
| week 8 | flash sd card, test on actual hardware | assembly, testing, finalizing              |