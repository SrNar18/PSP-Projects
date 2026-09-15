"""Python port of pspsdk tools/PrxEncrypter (bbtgp). Produces a Sony-tag-signed
~PSP executable from a homebrew PRX (ELF) by reusing a pre-signed header and
forging a CMAC collision. Non-pspemu mode only (0x150 PSP header, 0x110 kirk)."""
import re, struct, sys
from Crypto.Cipher import AES

KIRK1_KEY = bytes([0x98,0xC9,0x40,0x97,0x5C,0x1D,0x10,0xE8,0x7F,0xE6,0x0E,0xA3,0xFD,0x03,0xA8,0xBA])
KIRK_BASE = 0x110
PSP_HDR = 0x150
RB = bytes(15) + b'\x87'

def load_headers(path):
    src = open(path).read()
    out = {}
    for m in re.finditer(r'(psp_\w+Header_\w+)\s*\[[^\]]*\]\s*=\s*\{(.*?)\};', src, re.S):
        out[m.group(1)] = bytes(int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{1,2})', m.group(2)))
    return [(out['psp_pspHeader_' + n], out['psp_kirkHeader_' + n]) for n in ('small2', 'small', 'big')]

def xor(a, b): return bytes(x ^ y for x, y in zip(a, b))
def ecb(key): return AES.new(key, AES.MODE_ECB)
def cbc_enc(key, data): return AES.new(key, AES.MODE_CBC, iv=bytes(16)).encrypt(data)
def cbc_dec(key, data): return AES.new(key, AES.MODE_CBC, iv=bytes(16)).decrypt(data)

def shl1(b):
    v = int.from_bytes(b, 'big') << 1
    return (v & ((1 << 128) - 1)).to_bytes(16, 'big')

def subkeys(k):
    L = ecb(k).encrypt(bytes(16))
    K1 = shl1(L) if not (L[0] & 0x80) else xor(shl1(L), RB)
    K2 = shl1(K1) if not (K1[0] & 0x80) else xor(shl1(K1), RB)
    return K1, K2

def _cmac_prefix(k, msg):
    """Returns (X after n-1 blocks, M_last, n) per RFC4493."""
    K1, K2 = subkeys(k)
    n = (len(msg) + 15) // 16
    if n == 0:
        n, full = 1, False
    else:
        full = len(msg) % 16 == 0
    last = msg[16 * (n - 1):]
    if full:
        m_last = xor(last, K1)
    else:
        pad = last + b'\x80' + bytes(15 - len(last))
        m_last = xor(pad, K2)
    c = ecb(k)
    X = bytes(16)
    for i in range(n - 1):
        X = c.encrypt(xor(X, msg[16 * i:16 * i + 16]))
    return X, m_last, n

def cmac(k, msg):
    X, m_last, _ = _cmac_prefix(k, msg)
    return ecb(k).encrypt(xor(X, m_last))

def cmac_forge(k, msg, target):
    """Modify last 16 bytes of msg (bytearray) so cmac(k,msg)==target. Only valid
    for full last block (as in PrxEncrypter usage)."""
    X, m_last, n = _cmac_prefix(k, msg)
    Y = xor(X, m_last)
    pre = ecb(k).decrypt(target)
    forge = xor(pre, Y)
    newlast = xor(forge, msg[16 * (n - 1):16 * n])
    msg[16 * (n - 1):16 * n] = newlast

def kirk_size(kirk_hdr):
    s = struct.unpack('<I', kirk_hdr[0x70:0x74])[0]
    if s % 16: s += 16 - s % 16
    return s + KIRK_BASE

def encrypt(elf, headers):
    target = None
    for psp_hdr, kirk_hdr in headers:
        if kirk_size(kirk_hdr) - PSP_HDR >= len(elf):
            target = (psp_hdr, kirk_hdr); break
    if target is None:
        raise SystemExit('PRX SIGNER: Elf is too big')
    psp_hdr, kirk_hdr = target
    if struct.unpack('<H', psp_hdr[6:8])[0] == 1:
        raise SystemExit('compressed header selected; not supported in this port')
    kraw_size = kirk_size(kirk_hdr)
    data_size = struct.unpack('<I', kirk_hdr[0x70:0x74])[0]
    data_off = struct.unpack('<I', kirk_hdr[0x74:0x78])[0]
    assert struct.unpack('<I', kirk_hdr[0x60:0x64])[0] == 1, 'mode != CMD1'
    chk = data_size + (16 - data_size % 16 if data_size % 16 else 0)

    keys = cbc_dec(KIRK1_KEY, kirk_hdr[:0x20])
    aes_key, cmac_key = keys[:16], keys[16:32]

    buf = bytearray(kraw_size)
    buf[:KIRK_BASE] = kirk_hdr
    buf[KIRK_BASE:KIRK_BASE + len(elf)] = elf
    # kirk_CMD0: encrypt payload (IV=0) — header restored afterwards, so skip CMAC/keys steps
    start = 0x90 + data_off
    buf[start:start + chk] = cbc_enc(aes_key, bytes(buf[start:start + chk]))
    buf[:0x90] = kirk_hdr[:0x90]
    # kirk_forge
    assert cmac(cmac_key, bytes(buf[0x60:0x90])) == kirk_hdr[0x20:0x30], 'header hash mismatch'
    region = bytearray(buf[0x60:0x60 + 0x30 + chk + data_off])
    cmac_forge(cmac_key, region, kirk_hdr[0x30:0x40])
    buf[0x60:0x60 + len(region)] = region
    assert cmac(cmac_key, bytes(buf[0x60:0x60 + 0x30 + chk + data_off])) == kirk_hdr[0x30:0x40], 'forge failed'

    out = psp_hdr + bytes(buf[KIRK_BASE:kraw_size])
    # self-check: decrypt back and compare
    dec = cbc_dec(aes_key, bytes(buf[start:start + chk]))
    assert dec[:len(elf) - (len(elf) % 16 or 16) - 0] [:len(elf) - 16] == elf[:len(elf) - 16], 'roundtrip mismatch'
    tail_ok = dec[:len(elf)] == elf
    return out, tail_ok, len(elf), chk

if __name__ == '__main__':
    inp, outp = sys.argv[1], sys.argv[2]
    hdrs = load_headers(sys.argv[3] if len(sys.argv) > 3 else 'psp_headers.h')
    elf = open(inp, 'rb').read()
    assert elf[:4] == b'\x7fELF', 'input is not ELF'
    out, tail_ok, n, chk = encrypt(elf, hdrs)
    open(outp, 'wb').write(out)
    print(f'wrote {outp}: {len(out)} bytes; elf {n} bytes, payload {chk}; elf fully intact after forge: {tail_ok}')
