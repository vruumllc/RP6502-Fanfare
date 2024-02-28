#!/usr/bin/python3

from PIL import Image

def rp6502_rgb_to_bpp4(r1,g1,b1,r2,g2,b2):
    return (((b1>>7)<<6)|((g1>>7)<<5)|((r1>>7)<<4)|((b2>>7)<<2)|((g2>>7)<<1)|(r2>>7))

def rp6502_rgb_to_bpp16(r,g,b):
    if r==0 and g==0 and b==0:
        return 0
    else:
        return ((((b>>3)<<11)|((g>>3)<<6)|(r>>3))|1<<5)

def conv_spr(name_in, size, name_out):
    with Image.open(name_in) as im:
        with open("./" + name_out, "wb") as o:
            rgb_im = im.convert("RGB")
            im2 = rgb_im.resize(size=[size,size])
            for y in range(0, im2.height):
                for x in range(0, im2.width):
                    r, g, b = im2.getpixel((x, y))
                    o.write(
                        rp6502_rgb_to_bpp16(r,g,b).to_bytes(
                            2, byteorder="little", signed=False
                        )
                    )


conv_spr("breadboard.png", 64, "breadboard.bin")
conv_spr("circuitboard.png", 64, "circuitboard.bin")
conv_spr("portable.png", 64, "portable.bin")

