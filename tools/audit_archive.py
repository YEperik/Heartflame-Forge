#!/usr/bin/env python3
"""Audit a ZIP of game assets without extracting file payloads."""
from __future__ import annotations
import argparse, collections, csv, json, pathlib, zipfile


def classify(prefix: bytes) -> str:
    if prefix.startswith(b"SL01"): return "SL01"
    if prefix.startswith(b"TIM2"): return "TIM2"
    if prefix.startswith(b"\x00\x00\x01\xba"): return "MPEG-PS/PSS-like"
    if prefix.startswith(b"\x7fELF"): return "ELF"
    if prefix.startswith(b"MIG.00.1PSP"): return "Sony-GIM"
    if prefix.startswith(b"SDPK"): return "SDPK"
    if prefix.startswith(b"ADSC"): return "ADSC"
    if prefix.startswith(b".GIS"): return "GIS-text"
    if b"ROMDIR" in prefix: return "ROMDIR-marker"
    return "unidentified/other"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("archive", type=pathlib.Path)
    ap.add_argument("--out", type=pathlib.Path, default=pathlib.Path("audit-output"))
    ap.add_argument("--test-zip", action="store_true", help="read all entries to verify ZIP CRCs")
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    rows = []
    ext_count = collections.Counter()
    sig_count = collections.Counter()
    dir_count = collections.Counter()
    sl01_count = sl01_match = 0
    total_size = 0
    zip_bad = None
    directory_entry_count = 0

    with zipfile.ZipFile(args.archive, "r") as z:
        all_infos = z.infolist()
        directory_entry_count = sum(i.is_dir() for i in all_infos)
        infos = [i for i in all_infos if not i.is_dir()]
        for info in infos:
            path = info.filename
            ext = pathlib.PurePosixPath(path).suffix.lower() or "[none]"
            ext_count[ext] += 1
            parts = path.split("/")
            top = parts[1] if len(parts) > 1 and parts[0] == "dataimg" else parts[0]
            dir_count[top] += 1
            total_size += info.file_size
            with z.open(info) as f:
                prefix = f.read(64)
            sig = classify(prefix)
            sig_count[sig] += 1
            field04 = field08 = match = ""
            if prefix.startswith(b"SL01"):
                sl01_count += 1
                if len(prefix) >= 12:
                    field04 = f"0x{int.from_bytes(prefix[4:8], 'little'):08X}"
                    field08_val = int.from_bytes(prefix[8:12], "little")
                    field08 = str(field08_val)
                    match = str(field08_val == info.file_size - 12).lower()
                    sl01_match += field08_val == info.file_size - 12
            rows.append({
                "path": path, "extension": ext, "size_bytes": info.file_size,
                "magic_hex_16": prefix[:16].hex(" ").upper(), "signature_class": sig,
                "sl01_field_0x04_hex_uninterpreted": field04,
                "sl01_field_0x08_u32le": field08,
                "sl01_field_0x08_equals_size_minus_12": match,
            })
        if args.test_zip:
            zip_bad = z.testzip()

    csv_path = args.out / "asset_inventory.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()) if rows else ["path"])
        writer.writeheader(); writer.writerows(rows)
    summary = {
        "archive": str(args.archive),
        "file_count_excluding_directories": len(infos),
        "uncompressed_size_bytes": total_size,
        "directory_entry_count": directory_entry_count,
        "extension_counts": dict(ext_count.most_common()),
        "signature_counts": dict(sig_count.most_common()),
        "top_level_path_group_counts": dict(dir_count.most_common()),
        "sl01_count": sl01_count,
        "sl01_offset_0x08_equals_file_size_minus_12": sl01_match,
        "zip_crc_test": "not_run" if not args.test_zip else ("ok" if zip_bad is None else f"failed: {zip_bad}"),
    }
    json_path = args.out / "archive_summary.json"
    json_path.write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Wrote {csv_path}")
    print(f"Wrote {json_path}")
    print(f"Files: {len(infos)}; uncompressed bytes: {total_size}; SL01 invariant: {sl01_match}/{sl01_count}")
    if args.test_zip: print(f"ZIP CRC integrity: {'OK' if zip_bad is None else 'FAILED: ' + zip_bad}")
    return 1 if args.test_zip and zip_bad is not None else 0

if __name__ == "__main__":
    raise SystemExit(main())
