"""Build a fixed, trusted outline font for firmware. Requires fonttools.

Source: Google Fonts ofl/notosansjp/NotoSansJP[wght].ttf (SIL OFL).
Keep Japanese JIS X 0208 + CP932 extensions, Latin, kana and UI symbols.
Never load fonts supplied by NFC into the embedded rasterizer.
"""
import argparse
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
from fontTools import subset

parser = argparse.ArgumentParser()
parser.add_argument("source")
parser.add_argument("output")
args = parser.parse_args()
font = TTFont(args.source)
font = instantiateVariableFont(font, {"wght": 500}, inplace=True)
chars = set(range(32, 256)) | set(range(0x3000, 0x3100)) | set(range(0xFF00, 0xFFF0))
for hi in range(0x81, 0xFD):
    for lo in range(0x40, 0xFD):
        try:
            chars.update(map(ord, bytes([hi, lo]).decode("cp932")))
        except UnicodeDecodeError:
            pass
chars.update(map(ord, "←→↑↓●○✓…−–—・髙﨑"))
options = subset.Options()
options.layout_features = []
options.name_IDs = [0, 1, 2, 4, 5, 6]
options.hinting = False
subsetter = subset.Subsetter(options=options)
subsetter.populate(unicodes=chars)
subsetter.subset(font)
out = Path(args.output)
out.parent.mkdir(parents=True, exist_ok=True)
font.save(out)
print(f"{out}: {out.stat().st_size:,} bytes; {len(font.getBestCmap())} characters")
