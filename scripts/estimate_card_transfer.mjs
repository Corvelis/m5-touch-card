// Optimistic lower-bound model, NOT a benchmark or end-to-end completion promise.
// Metadata/envelope assumed512 bytes; KB here is1024 bytes. NFC-A106 kbit/s,
// 8 data bits + parity, 13-byte DATA header, 13-byte reply, two CRC bytes each.
// Excludes detection, I2C/register traffic, receiver processing/polling, frame
// delimiters/turnaround, persistence, retries, FINISH, and display refresh.
function dataTime(bytes, chunk, loopMs, minIntervalMs) {
  let ms = 0, chunks = 0;
  for (let pos = 0; pos < bytes; pos += chunk) {
    const n = Math.min(chunk, bytes - pos);
    ms += Math.max(minIntervalMs, (n + 30) * 9 / 106000 * 1000 + loopMs);
    ++chunks;
  }
  return { bytes, chunks, dataLowerBoundSeconds: +(ms / 1000).toFixed(3) };
}
for (const jpegKB of [0, 8, 10, 20, 30]) {
  const jpeg = jpegKB * 1024;
  console.log(JSON.stringify({ jpegKB,
    before: dataTime(512 + 4 * Math.ceil(jpeg / 3), 128, 5, 12),
    compact: dataTime(12 + 512 + jpeg, 240, 1, 2),
  }));
}
