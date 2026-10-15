#!/usr/bin/env python3
"""Generate native pixel-art PNG icons and a batched atlas; no dependencies."""
from pathlib import Path
import json, math, struct, zlib
ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'assets/ui'
DEST.mkdir(parents=True,exist_ok=True)
INK=(37,49,43,255); GOLD=(234,191,100,255); GREEN=(108,169,107,255); BLUE=(103,181,202,255); WHITE=(225,228,207,255)

def png(path,w,h,pixels):
    def chunk(kind,data): return struct.pack('!I',len(data))+kind+data+struct.pack('!I',zlib.crc32(kind+data)&0xffffffff)
    raw=b''.join(b'\0'+bytes(sum(pixels[y*w:(y+1)*w],())) for y in range(h))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('!IIBBBBB',w,h,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b''))

def artwork(kind,color=GOLD):
    p=[(0,0,0,0)]*1024
    def pixel(x,y,c):
        if 0<=x<32 and 0<=y<32:p[y*32+x]=c
    def rect(x,y,w,h,c):
        for yy in range(y,y+h):
            for xx in range(x,x+w):pixel(xx,yy,c)
    def line(x,y,xx,yy,c,width=1):
        n=max(abs(xx-x),abs(yy-y),1)
        for i in range(n+1):rect(round(x+(xx-x)*i/n),round(y+(yy-y)*i/n),width,width,c)
    def circle(x,y,r,c):
        for yy in range(y-r,y+r+1):
            for xx in range(x-r,x+r+1):
                if (xx-x)**2+(yy-y)**2<=r*r:pixel(xx,yy,c)
    def poly(points,c):
        for yy in range(32):
            for xx in range(32):
                inside=False
                for i,a in enumerate(points):
                    b=points[(i+1)%len(points)]
                    if (a[1]>yy)!=(b[1]>yy) and xx<(b[0]-a[0])*(yy-a[1])/(b[1]-a[1])+a[0]:inside=not inside
                if inside:pixel(xx,yy,c)
    # A consistent shadow keeps all icons legible on both tree-cell colors.
    rect(7,26,19,2,(19,30,26,150))
    if kind=='heart':
        poly([(4,9),(8,4),(14,4),(16,8),(19,4),(25,4),(29,9),(27,18),(16,28),(5,18)],(208,104,112,255));rect(9,7,4,4,(242,168,159,255))
    elif kind=='person':
        circle(16,9,6,(222,179,137,255));poly([(10,17),(22,17),(26,29),(6,29)],GREEN);rect(15,18,3,10,GOLD)
    elif kind=='map':
        poly([(3,7),(11,4),(21,7),(29,4),(29,26),(21,29),(11,26),(3,29)],WHITE);line(11,5,11,25,GREEN);line(21,8,21,27,GREEN);line(5,23,25,13,BLUE,2);line(14,13,20,19,GOLD,2);line(14,19,20,13,GOLD,2)
    elif kind in ('repair','engineering','citizen_engineering','craft_items','crafting','apprenticeship'):
        line(8,27,24,8,GOLD,3);circle(22,8,7,WHITE);circle(23,7,3,INK);rect(25,2,7,5,(0,0,0,0));circle(8,26,3,WHITE)
    elif kind=='reeds':
        for xx in (8,16,23):
            line(xx,28,xx+2,5,GREEN,2);rect(xx,4,5,8,(178,144,79,255));line(xx,23,xx-4,16,GREEN,2)
    elif kind in ('wood','gathering','gather_sites'):
        rect(14,15,5,11,(147,102,59,255));poly([(16,2),(4,20),(28,20)],GREEN);poly([(16,3),(9,14),(23,14)],(149,199,122,255))
    elif kind=='stone':poly([(5,23),(7,10),(17,5),(27,13),(26,25)],(156,173,166,255));poly([(7,10),(17,5),(18,16),(5,23)],WHITE)
    elif kind in ('berries','medicine','potion'):
        if kind=='potion':rect(13,4,7,4,GOLD);rect(12,8,9,6,WHITE);circle(16,20,9,BLUE);rect(11,16,3,7,WHITE)
        else:
            for x,y in [(10,18),(21,18),(16,24)]:circle(x,y,6,(193,93,116,255));circle(x-2,y-2,2,(238,160,155,255))
            poly([(14,13),(9,5),(21,8)],GREEN)
    elif kind in ('reeds','rod','fishing','catch_fish'):
        line(7,27,17,4,GOLD,2);line(17,4,25,15,WHITE);line(25,15,25,23,WHITE);circle(24,24,2,BLUE)
    elif kind=='fish':
        poly([(4,16),(14,7),(25,11),(29,6),(29,26),(24,22),(14,25)],BLUE);circle(12,14,2,INK);line(17,11,21,20,WHITE)
    elif kind=='leather':poly([(8,5),(15,9),(24,4),(28,12),(24,18),(26,27),(16,24),(6,28),(9,19),(4,11)],(168,118,70,255));line(11,13,19,23,GOLD,2)
    elif kind in ('iron','copper','steel','crystal','pearl','core','relic'):
        if kind=='pearl':circle(16,16,10,WHITE);circle(12,12,3,(250,247,225,255))
        elif kind in ('core','relic'):rect(7,8,19,18,color);rect(10,11,13,12,INK);circle(16,17,4,BLUE)
        else:poly([(6,22),(10,8),(21,5),(27,20),(18,27)],color);poly([(10,8),(16,11),(18,27),(6,22)],WHITE)
    elif kind in ('sword','blade','spear','axe','pickaxe','club','hammer','combat','tools','arrows'):
        if kind=='spear':line(8,27,22,6,(159,114,67,255),2);poly([(19,8),(25,2),(24,12)],WHITE)
        elif kind in ('axe','pickaxe','tools'):line(10,27,20,6,GOLD,3);poly([(9,7),(23,4),(27,13),(15,16)],color)
        elif kind in ('club','hammer'):line(10,27,20,9,GOLD,3);rect(11,5,16,8,color)
        elif kind=='arrows':
            for x in (9,15,21):line(x,26,x+2,5,GOLD);poly([(x-1,7),(x+2,2),(x+5,7)],WHITE)
        else:line(7,26,12,21,GOLD,3);poly([(11,19),(23,4),(28,3),(27,9),(15,23)],color);line(8,16,20,26,GOLD,3)
    elif kind=='bow':line(9,5,9,26,WHITE);line(9,5,21,12,GOLD,2);line(21,12,21,20,GOLD,2);line(21,20,9,26,GOLD,2);line(5,16,27,16,WHITE)
    elif kind in ('outfit','militia'):
        poly([(11,5),(16,9),(21,5),(28,12),(24,17),(21,14),(22,27),(9,27),(10,14),(7,17),(3,12)],color);rect(14,12,3,12,WHITE)
    elif kind=='raft':
        for x in (7,13,19,25):line(x,7,x,26,GOLD,4)
        line(4,12,28,12,GREEN,2);line(4,22,28,22,GREEN,2)
    elif kind=='canoe':
        poly([(2,19),(9,14),(24,14),(30,19),(24,24),(9,24)],GOLD);line(8,19,25,19,INK,3);line(11,9,22,28,WHITE,2)
    elif kind in ('boat','skiff','ship','cutter','brigantine','flagship','sailing','navigation_school'):
        poly([(3,22),(29,22),(23,28),(9,28)],GOLD)
        if kind in ('boat','skiff'):
            line(9,17,23,27,WHITE,2);line(23,17,9,27,WHITE,2)
        else:
            masts=3 if kind=='flagship' else 2 if kind=='brigantine' else 1
            for mast in range(masts):
                x=8+mast*8 if masts>1 else 16
                line(x,5,x,23,WHITE,2);poly([(x+2,5),(x+10,20),(x+2,20)],WHITE)
    elif kind in ('submarine','diving','oxygen','sonar'):
        circle(9,19,6,color);circle(23,19,6,color);rect(9,13,14,12,color);rect(14,7,8,6,color);line(19,7,19,3,GOLD,2)
        for x in (9,16,23):circle(x,18,2,BLUE)
    elif kind in ('town','construction','fortification','town_orders','diplomacy','diplomatic_corps','civic_charter'):
        rect(7,14,19,13,color);poly([(4,14),(16,3),(29,14)],GOLD);rect(13,19,6,8,INK);rect(9,17,3,4,BLUE);rect(22,17,3,4,BLUE)
    elif kind=='campfire':line(5,27,26,23,GOLD,3);line(5,23,26,27,GOLD,3);poly([(9,23),(12,9),(17,15),(21,5),(25,23)],(231,131,67,255));poly([(14,23),(18,14),(22,23)],GOLD)
    elif kind in ('coins','trading','commerce_school'):
        for x,y in [(12,21),(22,17),(14,9)]:circle(x,y,6,GOLD);circle(x,y,3,(191,140,66,255));line(x,y-2,x,y+2,WHITE)
    else:
        rect(5,6,23,21,color);rect(15,6,2,21,INK);line(9,11,12,11,WHITE);line(20,11,24,11,WHITE);line(9,17,12,17,WHITE);line(20,17,24,17,WHITE)
    return [p[(y//2)*32+x//2] for y in range(64) for x in range(64)]

icons={}
def add(key,kind,color=GOLD):icons[key]=artwork(kind,color)
for i,kind in enumerate(['wood','stone','berries','reeds','fish','leather','iron','copper','crystal','pearl','steel','core','relic']):
    add('good_'+str(i),kind,[(166,180,183,255),(198,128,83,255),(127,192,218,255)][i%3])
for kind in ['coins','potion','campfire','arrows','repair','axe','pickaxe','rod','boat','ship','submarine','sonar','oxygen','town','book','heart','person','map','gather_sites','craft_items','catch_fish','town_orders','civic_charter']:add(kind,kind)
for path in sorted((ROOT/'content/weapons').glob('*.json')):
    item=json.loads(path.read_text());add('weapon_'+item['id'],item['kind'],[(201,205,179,255),(146,198,195,255),(205,160,213,255)][item['rarity']%3])
for path in sorted((ROOT/'content/outfits').glob('*.json')):
    item=json.loads(path.read_text());add('outfit_'+item['id'],'outfit',tuple(item['color'])+(255,))
for key in ['gathering','trading','crafting','tools','combat','fishing','sailing','medicine','construction','diplomacy','engineering']:add('skill_'+key,key)
for path in sorted((ROOT/'content/settlements').glob('*.json')):
    item=json.loads(path.read_text());add('tech_'+item['technology'],item['technology'])
for key in ['apprenticeship','militia','commerce_school','navigation_school','citizen_engineering','diplomatic_corps','guild_charter']:add('tech_'+key,key)
add('tech_civic_charter','civic_charter')
for path in sorted((ROOT/'content/vessels').glob('*.json')):
    vessel=json.loads(path.read_text())
    kind=vessel['id']
    add('vessel_'+vessel['id'],kind,tuple(vessel.get('color',GOLD)))
keys=list(icons);width=16*64;height=math.ceil(len(keys)/16)*64;atlas=[(0,0,0,0)]*(width*height)
for i,key in enumerate(keys):
    pixels=icons[key];png(DEST/(key+'.png'),64,64,pixels)
    for y in range(64):
        start=(i//16*64+y)*width+i%16*64;atlas[start:start+64]=pixels[y*64:(y+1)*64]
png(DEST/'icons.png',width,height,atlas)
(DEST/'icons.json').write_text(json.dumps(keys,indent=2)+'\n')
print(f'Generated {len(keys)} icons and {width}x{height} PNG atlas')
