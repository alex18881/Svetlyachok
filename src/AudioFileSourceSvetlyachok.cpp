/*
  AudioFileSourceFunction
  Audio output generator which can generate WAV file data from function

  Copyright (C) 2021  Hideaki Tai

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AudioFileSourceSvetlyachok.h"

AudioFileSourceSvetlyachok::AudioFileSourceSvetlyachok(uint32_t pos_from, uint32_t pos_to) {
	uint16_t channels = 1U;
	uint32_t sample_per_sec = 4000U;
	uint32_t bits_per_sample = 8;
	uint32_t bytes_per_sec = sample_per_sec * channels * bits_per_sample / 8;
	uint32_t len = pos_to - pos_from;

	// RIFF chunk
	strncpy(wav_header.riff.chunk_id, "RIFF", 4);
	wav_header.riff.chunk_size = 4			// size of riff chunk w/o chunk_id and chunk_size
								 + 8 + 16	// size of format chunk
								 + 8 + len; // size of data chunk
	strncpy(wav_header.riff.format, "WAVE", 4);

	// format chunk
	strncpy(wav_header.format.chunk_id, "fmt ", 4);
	wav_header.format.chunk_size = 16;
	wav_header.format.format_tag = 0x0001; // PCM
	wav_header.format.channels = channels;
	wav_header.format.sample_per_sec = sample_per_sec;
	wav_header.format.avg_bytes_per_sec = bytes_per_sec;
	wav_header.format.block_align = channels * bits_per_sample / 8;
	wav_header.format.bits_per_sample = bits_per_sample;

	// data chunk
	strncpy(wav_header.data.chunk_id, "data", 4);
	wav_header.data.chunk_size = len;

	pos_start = pos_from;
	pos_end = pos_to;
	pos = 0;
	header_size = sizeof(WavHeader);
	size = header_size + len;
}

AudioFileSourceSvetlyachok::~AudioFileSourceSvetlyachok() {
  close();
}

uint32_t AudioFileSourceSvetlyachok::read(void* data, uint32_t len) {
  // callback size must be 1 or equal to channels

  uint8_t* d = reinterpret_cast<uint8_t*>(data);
  uint32_t i = 0;
  while (i < len) {
    uint32_t p = pos + i;
    if (p < header_size) {
      // header bytes
      d[i] = wav_header.bytes[p];
      i += 1;
    } else {
      // data bytes
      uint8_t serial_data[1];
      SerialFlash.read(pos_start + p - header_size, serial_data, 1);
      d[i] = serial_data[0];
      i += wav_header.format.block_align;
    }
  }
  pos += i;
  return (pos >= size) ? 0 : i;
}

bool AudioFileSourceSvetlyachok::seek(int32_t pos, int dir) {
  if (dir == SEEK_SET) {
    if (pos < 0 || (uint32_t)pos >= size)
      return false;
    this->pos = pos;
  } else if (dir == SEEK_CUR) {
    int32_t p = (int32_t)this->pos + pos;
    if (p < 0 || (uint32_t)p >= size)
      return false;
    this->pos = p;
  } else {
    int32_t p = (int32_t)this->size + pos;
    if (p < 0 || (uint32_t)p >= size)
      return false;
    this->pos = p;
  }
  return true;
}

bool AudioFileSourceSvetlyachok::close() {
  pos = 0;
  size = 0;
  return true;
}

bool AudioFileSourceSvetlyachok::isOpen() {
  return true;
}

uint32_t AudioFileSourceSvetlyachok::getSize() {
  return size;
}

uint32_t AudioFileSourceSvetlyachok::getPos() {
  return pos;
}
