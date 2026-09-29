// Compare the flattened RGBA pixels of two AE "Multi-Machine Sequence" PSD frames.
// PSD resource metadata may differ between render processes; compare decoded pixels.
const fs = require('fs');
const crypto = require('crypto');

function decode(path) {
  const file = fs.readFileSync(path);
  if (file.length < 26 || file.toString('ascii', 0, 4) !== '8BPS' ||
      file.readUInt16BE(4) !== 1) throw Error('not PSD v1');
  const channels = file.readUInt16BE(12);
  const height = file.readUInt32BE(14);
  const width = file.readUInt32BE(18);
  const depth = file.readUInt16BE(22);
  const color = file.readUInt16BE(24);
  if (channels !== 4 || depth !== 8 || color !== 3) throw Error('expected 8-bit RGBA PSD');
  if (width === 0 || height === 0 || width * height > 200_000_000) throw Error('invalid frame dimensions');
  let offset = 26;
  for (let i = 0; i < 3; i++) {
    if (offset + 4 > file.length) throw Error('truncated PSD section');
    const size = file.readUInt32BE(offset);
    offset += 4 + size;
    if (offset > file.length) throw Error('truncated PSD section');
  }
  if (offset + 2 + 2 * channels * height > file.length) throw Error('truncated image header');
  if (file.readUInt16BE(offset) !== 1) throw Error('expected PackBits compression');
  offset += 2;
  const lengths = [];
  for (let i = 0; i < channels * height; i++, offset += 2) lengths.push(file.readUInt16BE(offset));
  const output = Buffer.alloc(channels * height * width);
  for (let channel = 0; channel < channels; channel++) {
    for (let row = 0; row < height; row++) {
      const end = offset + lengths[channel * height + row];
      if (end > file.length) throw Error('truncated PackBits row');
      let column = 0;
      while (offset < end) {
        const control = file.readInt8(offset++);
        if (control >= 0) {
          const count = control + 1;
          if (offset + count > end || column + count > width) throw Error('invalid PackBits literal');
          file.copy(output, (channel * height + row) * width + column, offset, offset + count);
          offset += count;
          column += count;
        } else if (control > -128) {
          const count = 1 - control;
          if (offset >= end || column + count > width) throw Error('invalid PackBits run');
          output.fill(file[offset++], (channel * height + row) * width + column,
                      (channel * height + row) * width + column + count);
          column += count;
        }
      }
      if (offset !== end || column !== width) throw Error(`bad PackBits row ${channel}/${row}`);
    }
  }
  if (offset !== file.length) throw Error('trailing image bytes');
  const plane = height * width;
  const sample = (x, y) => Array.from({length: 4}, (_, c) => output[c * plane + y * width + x]);
  const corner = sample(0, 0);
  let different = 0;
  let alphaLessThan255 = 0;
  for (let pixel = 0; pixel < plane; pixel++) {
    if (output[pixel] !== corner[0] || output[plane + pixel] !== corner[1] ||
        output[2 * plane + pixel] !== corner[2]) different++;
    if (output[3 * plane + pixel] !== 255) alphaLessThan255++;
  }
  return {path, width, height, pixels: output, corner, center: sample(Math.floor(width / 2), Math.floor(height / 2)),
          pixelsDifferentFromCorner: different, alphaLessThan255,
          channelSha256: Array.from({length: 4}, (_, c) =>
            crypto.createHash('sha256').update(output.subarray(c * plane, (c + 1) * plane)).digest('hex'))};
}

try {
  if (process.argv.length !== 4) throw Error('usage: node tools/Compare-PsdFrames.cjs frame-a.psd frame-b.psd');
  const a = decode(process.argv[2]);
  const b = decode(process.argv[3]);
  if (a.width !== b.width || a.height !== b.height) throw Error('frame dimensions differ');
  const plane = a.width * a.height;
  let differingPixels = 0;
  let firstDifference = null;
  if (!a.pixels.equals(b.pixels)) {
    for (let pixel = 0; pixel < plane; pixel++) {
      if (a.pixels[pixel] !== b.pixels[pixel] ||
          a.pixels[plane + pixel] !== b.pixels[plane + pixel] ||
          a.pixels[2 * plane + pixel] !== b.pixels[2 * plane + pixel] ||
          a.pixels[3 * plane + pixel] !== b.pixels[3 * plane + pixel]) {
        differingPixels++;
        if (firstDifference === null) firstDifference = [pixel % a.width, Math.floor(pixel / a.width)];
      }
    }
  }
  console.log(JSON.stringify({samePixels: differingPixels === 0, width: a.width, height: a.height,
    differingPixels, firstDifference, source: [a.path, b.path],
    pixelsDifferentFromCorner: [a.pixelsDifferentFromCorner, b.pixelsDifferentFromCorner],
    alphaLessThan255: [a.alphaLessThan255, b.alphaLessThan255],
    channelSha256: [a.channelSha256, b.channelSha256]}, null, 2));
  if (differingPixels !== 0) process.exitCode = 1;
} catch (error) {
  console.error(error.message);
  process.exitCode = 2;
}
