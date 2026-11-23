import sys,struct
sys.path.insert(0,__import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from swftags import load,tags
for p in sys.argv[1:]:
    d=load(p)
    for code,o,ln in tags(d):
        if code==75:
            fid,flags=struct.unpack_from('<HB',d,o); lang=d[o+3]; nl=d[o+4]; name=d[o+5:o+5+nl].decode('latin1').rstrip('\0')
            q=o+5+nl; ng=struct.unpack_from('<H',d,q)[0]; q+=2
            wide_off=flags&8; wide_codes=flags&4; has_layout=flags&0x80
            offs=[struct.unpack_from('<I' if wide_off else '<H',d,q+i*(4 if wide_off else 2))[0] for i in range(ng)]
            q+= (4 if wide_off else 2)*ng; codeoff=struct.unpack_from('<I' if wide_off else '<H',d,q)[0]
            codes_at=q-(4 if wide_off else 2)*ng+codeoff
            codes=[struct.unpack_from('<H',d,codes_at+2*i)[0] for i in range(ng)]
            lat=sum(1 for c in codes if c<0x250); acc=[chr(c) for c in codes if 0xC0<=c<0x180]
            print(p.split('/')[-1],'font',repr(name),'glyphs',ng,'flags',hex(flags),'layout',bool(has_layout),'latin<0x250',lat,'accents',len(acc),'fr', all(ch in acc for ch in 'éèàçêôû'), 'de', all(ch in acc for ch in 'äöüß'))
        if code in (88,):
            print(' name tag',d[o:o+60])
