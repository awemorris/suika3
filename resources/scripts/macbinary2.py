import os
import struct

def calc_crc(data):
    crc = 0
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc <<= 1
            crc &= 0xFFFF
    return crc

def create_macbinary_ii(base_name, out_name):
    d_data = open(base_name, 'rb').read() if os.path.exists(base_name) else b''
    rsrc_path = f".rsrc/{base_name}"
    r_data = open(rsrc_path, 'rb').read() if os.path.exists(rsrc_path) else b''

    finf_path = f".finf/{base_name}"
    f_data = open(finf_path, 'rb').read() if os.path.exists(finf_path) else b'\x00'*16
    file_type = f_data[0:4] if len(f_data) >= 4 else b'APPL'
    file_creator = f_data[4:8] if len(f_data) >= 8 else b'????'

    header = bytearray(128)
    header[1] = len(base_name)
    header[2:2+len(base_name)] = base_name.encode('ascii')[:63]
    header[65:69] = file_type
    header[69:73] = file_creator
    header[83:87] = struct.pack('>I', len(d_data))
    header[87:91] = struct.pack('>I', len(r_data))

    header[102] = 0x00
    header[122] = 0x02
    header[123] = 0x02

    crc = calc_crc(header[:124])
    header[124:126] = struct.pack('>H', crc)

    def pad(data):
        return data + b'\x00' * ((128 - len(data) % 128) % 128)

    with open(out_name, 'wb') as f:
        f.write(header)
        f.write(pad(d_data))
        f.write(pad(r_data))

    print(f"Generated MacBinaryII: {out_name}")

create_macbinary_ii('suika3', 'suika3.bin')
