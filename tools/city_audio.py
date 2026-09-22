"""Original layered city ambience and a dual-tone car horn, generated offline.
No downloaded recordings. Integer PCM mixing only on the PSP audio thread.
"""
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
rng=np.random.default_rng(29122)
sr=11025
n=sr*32;t=np.arange(n)/sr
def noise(low,high):
    f=np.fft.rfftfreq(n,1/sr)
    spectrum=np.fft.rfft(rng.normal(0,1,n))
    band=np.exp(-(f/high)**4)*(1-np.exp(-(f/low)**4))
    x=np.fft.irfft(spectrum*band,n)
    return x/np.std(x)
# Seamless street air, several passing tyres/engines and distant crowd murmur.
bed=noise(40,500)*.018+noise(500,2500)*.004
for centre,width in [(4,2.3),(13,3.2),(24,2.6)]:
    d=(t-centre+16)%32-16;env=np.exp(-(d/width)**2)
    bed+=env*(noise(70,850)*.025+np.sin(2*np.pi*75*t)*.018)
for centre in [2,8,18,29]:
    d=(t-centre+16)%32-16;env=np.exp(-(d/2.7)**2)
    murmur=noise(220,1300)*(.5+.5*np.sin(2*np.pi*3*t+.3))
    bed+=env*murmur*.011
# Small intermittent distant birds, not a regular electronic beep.
for start in [6.2,6.65,20.1,20.7]:
    k=int(start*sr);q=np.arange(int(.19*sr))/sr
    bird=np.sin(2*np.pi*(1900*q+1700*q*q))*np.sin(np.pi*q/.19)**2*.011
    bed[k:k+len(q)]+=bird
q=np.arange(22050)/sr
engine=(np.sin(2*np.pi*68*q)+.45*np.sin(2*np.pi*136*q)+.18*np.sin(2*np.pi*204*q))*.06
hsr=22050;q=np.arange(12127)/hsr
env=np.minimum(q/.022,1)*np.minimum((len(q)/hsr-q)/.09,1)
horn=sum((np.sin(2*np.pi*f*q)+.28*np.sin(2*np.pi*2*f*q)+.10*np.sin(2*np.pi*3*f*q)) for f in [410,505])*.21*env
for name,data in [('city-bed',bed),('city-engine',engine),('city-horn',horn)]:
    (ROOT/'assets'/f'{name}.pcm').write_bytes((np.clip(data,-.95,.95)*32767).astype('<i2').tobytes())
print('City bed, local engines and event horn generated.')
