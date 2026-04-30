#!/usr/bin/env python3
import re
from pathlib import Path

DBC_DIR = Path('dbc')
OUT_C = Path('components/dbc_schema/dbc_schema.c')
OUT_H = Path('components/dbc_schema/include/dbc_schema.h')

bo_re = re.compile(r'^BO_\s+(\d+)\s+([^:]+):\s+(\d+)\s+')
sg_re = re.compile(r'^\s*SG_\s+([^:]+)\s*:\s*(\d+)\|(\d+)@([01])([+-])\s*\(([-+0-9.eE]+),\s*([-+0-9.eE]+)\)\s*\[[^\]]*\]\s*"([^"]*)"')

def c_float(v: float) -> str:
    s = f"{v:.9g}"
    if 'e' not in s and 'E' not in s and '.' not in s:
        s += '.0'
    return s + 'f'

def c_str(s: str) -> str:
    return s.replace('\\', '\\\\').replace('"', '\\"')

def pick_first_dbc_file() -> Path:
    dbc_files = sorted([p for p in DBC_DIR.glob('*.dbc') if p.is_file()], key=lambda p: p.name.lower())
    if not dbc_files:
        raise FileNotFoundError(f"Aucun fichier .dbc trouve dans {DBC_DIR}")
    return dbc_files[0]

DBC = pick_first_dbc_file()

signals = []
cur_id = None
for line in DBC.read_text(encoding='utf-8', errors='ignore').splitlines():
    m = bo_re.match(line)
    if m:
        cur_id = int(m.group(1))
        continue
    m = sg_re.match(line)
    if m and cur_id is not None:
        name = m.group(1).strip()
        start = int(m.group(2))
        length = int(m.group(3))
        order = int(m.group(4))
        sign = m.group(5)
        factor = float(m.group(6))
        offset = float(m.group(7))
        unit = m.group(8).strip()
        signals.append((name, unit, cur_id, start, length, order, 1 if sign == '-' else 0, factor, offset))

OUT_H.write_text('''#ifndef DBC_SCHEMA_H
#define DBC_SCHEMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *name;
    const char *unit;
    uint32_t frame_id;
    uint8_t start_bit;
    uint8_t bit_length;
    uint8_t byte_order;
    uint8_t is_signed;
    float factor;
    float offset;
} dbc_signal_desc_t;

extern const char *g_dbc_name;
extern const dbc_signal_desc_t g_dbc_signals[];
extern const size_t g_dbc_signal_count;

#endif
''', encoding='utf-8')

with OUT_C.open('w', encoding='utf-8') as f:
    f.write('#include "dbc_schema.h"\n\n')
    f.write(f'const char *g_dbc_name = "{DBC.stem}";\n\n')
    f.write('const dbc_signal_desc_t g_dbc_signals[] = {\n')
    for s in signals:
        f.write('    {"%s", "%s", 0x%03X, %d, %d, %d, %d, %s, %s},\n' % (
            c_str(s[0]), c_str(s[1]), s[2], s[3], s[4], s[5], s[6], c_float(s[7]), c_float(s[8])
        ))
    f.write('};\n\n')
    f.write('const size_t g_dbc_signal_count = sizeof(g_dbc_signals) / sizeof(g_dbc_signals[0]);\n')

print(f'Using DBC: {DBC}')
print(f'Generated {OUT_H} and {OUT_C} with {len(signals)} signals from {DBC.name}.')
