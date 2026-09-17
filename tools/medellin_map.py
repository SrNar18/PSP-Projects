"""Generate a compact urban outline from Medellin's official commune polygons.
Geographic footprint is real; roads and relief are deliberately simplified.
"""
import json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
data=json.loads((ROOT/'assets/reference/medellin-comunas.geojson').read_text())
features=[f for f in data['features'] if f['properties']['codigo'].isdigit() and 1<=int(f['properties']['codigo'])<=16]
def rings(f):
    g=f['geometry'];return g['coordinates'] if g['type']=='Polygon' else [r for p in g['coordinates'] for r in p]
points=[p for f in features for r in rings(f) for p in r]
south=min(p[1] for p in points);north=max(p[1] for p in points)
west=min(p[0] for p in points);east=max(p[0] for p in points)
scale=3360/(north-south)
def project(lon,lat):return ((lon-west)*scale*math.cos(math.radians(6.25)),(north-lat)*scale)
river=[(6.34,-75.551),(6.30,-75.560),(6.27,-75.569),(6.24,-75.577),(6.21,-75.578),(6.18,-75.581),(6.15,-75.589)]
def river_lon(lat):
    for (a,x),(b,y) in zip(river,river[1:]):
        if b<=lat<=a:return x+(y-x)*(a-lat)/(a-b)
    return river[0][1] if lat>river[0][0] else river[-1][1]
profile=[]
for i in range(33):
    # Sample just inside endpoints, which otherwise collapse to a single point.
    lat=north-(north-south)*min(.992,max(.008,i/32));hits=[]
    for f in features:
        for ring in rings(f):
            for a,b in zip(ring,ring[1:]):
                if (a[1]>lat)!=(b[1]>lat):hits.append(a[0]+(b[0]-a[0])*(lat-a[1])/(b[1]-a[1]))
    l,r=min(hits),max(hits);mid=min(r-.0004,max(l+.0004,river_lon(lat)))
    profile.append([project(l,lat)[0],project(mid,lat)[0],project(r,lat)[0]])
# Keep the narrow tips playable with PSP-sized cars. The source outline is
# smoothed and widened locally, rather than shrinking vehicles or roads to zero.
for _ in range(2):
    profile=[[sum(profile[max(0,min(32,i+d))][k] for d in (-1,0,1))/3 for k in range(3)] for i in range(33)]
profile=[[min(l,c-550),c,max(r,c+550)] for l,c,r in profile]
out=['/* Generated from Alcaldia de Medellin commune boundaries, 2026-09-17. */','static const float medellin_profile[33][3]={']
out+=['{'+','.join(f'{x:.3f}f' for x in row)+'},' for row in profile];out+=['};']
(ROOT/'src/medellin_profile.h').write_text('\n'.join(out)+'\n')
(ROOT/'assets/reference/medellin-profile.json').write_text(json.dumps({'bounds':[west,south,east,north],'profile':profile,'height':3360,'note':'Urban envelope of 16 communes; 33 smoothed cross-sections; narrow ends widened for playable streets. River centerline manually approximated, relief artistic.'},indent=2))
print('Urban bounds:',west,south,east,north,'game width',max(r[2] for r in profile))
