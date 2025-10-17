#include <iostream>
#include <vector>
#include <string>
#include <sndfile.hh>
#include "../../bit_stream/src/bit_stream.h"

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536;

static inline short quantize_sample(short s, int bits) {
    if(bits >= 16) return s;
    const int step = 1 << (16 - bits);
    int x = static_cast<int>(s);
    // Round to nearest multiple of step (symmetric rounding around 0)
    int q = ((x + (x >= 0 ? step/2 : -step/2)) / step) * step;
    // Clamp to 16-bit range just in case
    if(q > 32767) q = 32767;
    if(q < -32768) q = -32768;
    return static_cast<short>(q);
}

static inline uint16_t sample_to_code(short sample, int bits){
    int shifted = static_cast<int>(sample) + 32768;
    return static_cast<uint16_t>(shifted >> (16 - bits));
}

int main(int argc, char *argv[]) {
    if(argc < 5) {
        cerr << "Usage: wav_quant_enc -b bits input.wav output.qnt\n";
        cerr << "  bits: number of quantization bits (1..16).\n";
        return 1;
    }

    int bits { 0 };
    for (int i=1; i<argc - 2; i++){
        if(string(argv[i]) == "-b" && i+1 < argc){
            bits = atoi(argv[i+1]);
        }
    }

    if(bits <= 0 || bits > 16){
        cerr << "Error: number of bits must be between 1 and 16\n";
        return 1;
    }

    SndfileHandle sfIn { argv[argc-2] };
    if(sfIn.error()){
        cerr << "Error: invalid input file\n";
        return 1;
    }

    if((sfIn.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
        cerr << "Error: file is not in WAV format\n";
        return 1;
    }

    if((sfIn.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        cerr << "Error: file is not in PCM_16 format\n";
        return 1;
    }

    int channels = sfIn.channels();
    int sample_rate = sfIn.samplerate();
    sf_count_t total_frames = sfIn.frames();

    cout << "Encoding " << argv[argc-2] << " into " << argv[argc-1] << " using " << bits << " bits per sample...\n";

    fstream out(argv[argc-1], ios::out | ios::binary | ios::trunc);
    BitStream bs(out, STREAM_WRITE);

    bs.write_string("QNT1");
    bs.write_n_bits(sample_rate, 32);
    bs.write_n_bits(channels, 16);
    bs.write_n_bits(bits, 8);
    bs.write_n_bits(total_frames, 32);

    sf_count_t frames_count;
    vector<short> buffer(FRAMES_BUFFER_SIZE * channels);
    while((frames_count = sfIn.readf(buffer.data(), FRAMES_BUFFER_SIZE))){
        size_t count = frames_count * channels;
        for (size_t i = 0; i < count; i++){
            short q = quantize_sample(buffer[i], bits);
            uint16_t code = sample_to_code(q, bits);
            bs.write_n_bits(code, bits);
        }
    }

    cout << "Done! Encoded " << total_frames << " frames.\n";
    return 0;
}