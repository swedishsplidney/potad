# good and not bad plan frfr

---

## parts list:

(lcsc preferred for any raw chips)

* allwinner h616 (cpu)
* MTFC32GAZAQHD-IT
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


# unknown
dont delete any questoins
* storage?
  * 1gb emmc and a 128gb sd card
  * 32 emmc and a sd card slot just not the main thing
* ram?
  * i havent reaserched ram yet so idk witch ones we want
  * ik we want like 4gb ddr4
* who is going to do the asitone stuff for the case
  * idk i am pretty sure swedish is printing idk
* what screen do we want?
  * oled or lcd
  * we can output with the hdmi stuff, idk what he tv_out pins are or we might be able to just use gpio pins
* toutch screen?
  * toutch pad 
  * we could do a mouse
  * we could like not have a mouse or cursor at all idk