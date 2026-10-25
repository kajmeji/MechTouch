import importlib.util
import pathlib
import struct
import unittest
import tempfile
import json
import hashlib

spec = importlib.util.spec_from_file_location("release", pathlib.Path(__file__).resolve().parents[1] / "scripts/prepare_ota_release.py")
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class ReleaseTests(unittest.TestCase):
    def image(self):
        data = bytearray(512)
        data[0] = 0xE9
        struct.pack_into("<H", data, 12, 9)
        struct.pack_into("<I", data, 32, 0xABCD5432)
        data[48:53] = b"1.0.5"
        identity = b"MechTouch-OTA:1.0.5:basic\0"
        data[300:300+len(identity)] = identity
        return data

    def table(self, slot=0x1e0000):
        return b"".join(struct.pack("<HBBII16sI", 0x50AA, kind, subtype, address, size, b"", 0)
                        for kind, subtype, address, size in [(1, 0, 0xe000, 8192),
                            (0, 0x10, 0x10000, slot), (0, 0x11, 0x1f0000, slot)])

    def test_valid_image(self):
        self.assertEqual(release.validate_image(self.image(), self.table()), ("1.0.5", "basic"))

    def test_oversized_image(self):
        with self.assertRaises(ValueError): release.validate_image(self.image(), self.table(256))

    def test_no_ota_metadata(self):
        with self.assertRaises(ValueError): release.validate_image(self.image(), self.table()[32:])

    def test_single_slot(self):
        with self.assertRaises(ValueError): release.validate_image(self.image(), self.table()[:64])

    def test_wrong_chip(self):
        image = self.image(); image[12] = 0
        with self.assertRaises(ValueError): release.validate_image(image, self.table())

    def test_bootloader_rejected(self):
        image = self.image(); image[32:36] = bytes(4)
        with self.assertRaises(ValueError): release.validate_image(image, self.table())

    def test_missing_identity(self):
        image = self.image(); image[300:] = bytes(len(image)-300)
        with self.assertRaises(ValueError): release.validate_image(image, self.table())

    def fixture(self, root):
        (root / "include").mkdir()
        (root / "include/config.h").write_text('#define FLUIDTOUCH_VERSION "1.0.5"')
        build = root / ".pio/build/elecrow-crowpanel-7-basic"
        build.mkdir(parents=True)
        (build / "firmware.bin").write_bytes(self.image())
        (build / "partitions.bin").write_bytes(self.table())
        return build

    def test_package_hash_size_and_url(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory); self.fixture(root)
            release.package(root, root / "out", "1.0.5", ["basic"])
            manifest = json.loads((root / "out/ota-basic.json").read_text())
            self.assertEqual(manifest["sha256"], hashlib.sha256(self.image()).hexdigest())
            self.assertEqual(manifest["size"], len(self.image()))
            self.assertEqual(manifest["url"], "https://github.com/kajmeji/MechTouch/releases/download/v1.0.5/mechtouch-basic-1.0.5.bin")

    def test_stale_build_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory); self.fixture(root)
            (root / "include/config.h").write_text('#define FLUIDTOUCH_VERSION "1.0.6"')
            with self.assertRaises(ValueError): release.package(root, root / "out", "1.0.6", ["basic"])

    def test_wrong_model_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory); build = self.fixture(root)
            image = bytes(self.image()).replace(b":basic\0", b":advance-v1.2\0")
            (build / "firmware.bin").write_bytes(image)
            with self.assertRaises(ValueError): release.package(root, root / "out", "1.0.5", ["basic"])


if __name__ == "__main__": unittest.main()
