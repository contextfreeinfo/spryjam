## Info

```sh
cp spryjam.uf2 /media/tom/RP2350/
```

```
Key: 105 76800 2000C6C0 2000C6C0 20001188 20001188
Hi there!: 80
H: 2000C6C0 200012C0 200012C8
i: 2000C6D8 200013C8 200013D0
 : 2000C6F0 20001180 20001188
t: 2000C708 20001420 20001428
h: 2000C720 200013C0 200013C8
e: 2000C738 200013A8 200013B0
r: 2000C750 20001410 20001418
e: 2000C768 200013A8 200013B0
!: 2000C780 20001188 20001190
```

```sh
build $ arm-none-eabi-size spryjam.elf
   text	   data	    bss	    dec	    hex	filename
  44108	      0	 108144	 152252	  252bc	spryjam.elf
build $ arm-none-eabi-size -A spryjam.elf
spryjam.elf  :
section                 size        addr
.flashtext               496   268435456
.rodata                    0   268435952
.ARM.exidx                 8   268435952
.binary_info              32   268435960
.ram_vector_table        272   536870912
.uninitialized_data        0   536871184
.text                  32936   536871184
.data                   5344   536904120
.tdata                     0   536909464
.tbss                      0   536909464
.bss                  105824   536909464
.heap                   2048   537015288
.scratch_x              1176   537395200
.scratch_y                 0   537399296
.stack1_dummy           2048   537396376
.stack_dummy            2048   537399296
.flash_end                20   268475448
.ARM.attributes           52           0
.comment                  38           0
.debug_info           246208           0
.debug_abbrev          44339           0
.debug_loclists       120044           0
.debug_aranges          5936           0
.debug_rnglists        23658           0
.debug_line           148768           0
.debug_str             47958           0
.debug_frame           14984           0
.debug_line_str          593           0
Total                 804830
```

## Some frame timings

```
Blink 80 at 4830: 17.974300 / 19.212063
Blink 81 at 4890: 17.840946 / 19.146839
Blink 82 at 4950: 17.992846 / 21.015739
Blink 83 at 5010: 17.995762 / 22.179741
Blink 84 at 5070: 17.974083 / 19.194380
Blink 85 at 5130: 17.994299 / 19.205284
Blink 86 at 5190: 17.999344 / 21.011311
Blink 87 at 5250: 17.996538 / 21.970873
Blink 88 at 5310: 17.960608 / 22.159161
Blink 89 at 5370: 17.997261 / 19.230339
Blink 90 at 5430: 17.877981 / 19.226841
Blink 91 at 5490: 17.999929 / 21.105619
Blink 92 at 5550: 17.749296 / 22.118202
Blink 93 at 5610: 17.951870 / 22.076639
Blink 94 at 5670: 17.995123 / 19.202267
Blink 95 at 5730: 17.913067 / 19.197161
```

But I don't get a display when doing calculations. Maybe slows down just enough
extra???
