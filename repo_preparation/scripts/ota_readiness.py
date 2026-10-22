"""Read-only build report; never flashes or changes a partition table."""
import csv
import pathlib
import sys


def number(value):
    value = value.strip()
    if value.upper().endswith('K'):
        return int(value[:-1], 0) * 1024
    if value.upper().endswith('M'):
        return int(value[:-1], 0) * 1024 * 1024
    return int(value, 0)


def report(binary, table):
    size = pathlib.Path(binary).stat().st_size
    with open(binary, 'rb') as stream:
        if size < 24 or stream.read(1) != b'\xe9':
            raise ValueError('Expected an ESP application firmware.bin image')
    rows = []
    with open(table, encoding='utf-8') as stream:
        for row in csv.reader(line for line in stream if line.strip() and not line.lstrip().startswith('#')):
            if len(row) < 5:
                raise ValueError('Invalid partition row')
            rows.append([v.strip() for v in row])
    slots = [number(r[4]) for r in rows if r[1] == 'app' and r[2].startswith('ota_')]
    data = any(r[1] == 'data' and r[2] == 'ota' for r in rows)
    ready = len(slots) >= 2 and data and size <= min(slots)
    print(f'Firmware: {size:,} bytes')
    print(f'OTA application slots: {len(slots)}; OTA metadata: {data}')
    if slots:
        print(f'Smallest OTA slot: {min(slots):,} bytes')
    print('Partition/size prerequisites: ' + ('PASS' if ready else 'NOT READY'))
    print('This report does not establish TLS, authenticity, rollback, or updater functionality.')
    print('Do not install partitions.bin through an application OTA updater.')
    return ready


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit('Usage: ota_readiness.py firmware.bin partition-table.csv')
    report(sys.argv[1], sys.argv[2])
