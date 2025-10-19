//------------------------------------------------------------------------------
// DCT-based lossy encoder for mono PCM16 WAV
// Block DCT, uniform quantization of first K coefficients, BitStream output
//------------------------------------------------------------------------------
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <fftw3.h>
#include <sndfile.hh>

#include "../../bit_stream/src/bit_stream.h"

using namespace std;

static inline uint32_t to_u32(int32_t v, int bits){
    // two's complement packing into 'bits' LSBs
    uint32_t mask = (bits >= 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
    return static_cast<uint32_t>(v) & mask;
}

static inline int32_t from_u32(uint32_t u, int bits){
    // not used here (decoder side) but kept for symmetry
    if(bits == 32) return static_cast<int32_t>(u);
    uint32_t sign = 1u << (bits-1);
    if(u & sign){
        // negative number
        int32_t val = static_cast<int32_t>(u | (~((1u<<bits)-1u)));
        return val;
    } else {
        return static_cast<int32_t>(u);
    }
}

static void write_u32(BitStream &bs, uint32_t v){
    bs.write_n_bits((v >> 24) & 0xFF, 8);
    bs.write_n_bits((v >> 16) & 0xFF, 8);
    bs.write_n_bits((v >> 8) & 0xFF, 8);
    bs.write_n_bits((v) & 0xFF, 8);
}

static void write_u16(BitStream &bs, uint16_t v){
    bs.write_n_bits((v >> 8) & 0xFF, 8);
    bs.write_n_bits((v) & 0xFF, 8);
}

static void write_f32(BitStream &bs, float f){
    static_assert(sizeof(float)==4, "float not 32-bit");
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    write_u32(bs, u);
}

int main(int argc, char* argv[]){
    bool verbose = false;
    size_t blockSize = 1024;  // N
    size_t keepK = 256;       // K (low-frequency coefficients)
    int coeffBits = 12;       // bits per quantized coefficient
    float qStep = 8.0f;       // uniform quantization step

    if(argc < 3){
        cerr << "Usage: dct_enc [ -v ] [ -bs N ] [ -k K ] [ -b bits ] [ -q step ] input.wav output.dct\n";
        return 1;
    }

    for(int i=1;i<argc;i++) if(string(argv[i])=="-v") verbose=true;
    for(int i=1;i+1<argc;i++) if(string(argv[i])=="-bs") blockSize = static_cast<size_t>(atoi(argv[i+1]));
    for(int i=1;i+1<argc;i++) if(string(argv[i])=="-k") keepK = static_cast<size_t>(atoi(argv[i+1]));
    for(int i=1;i+1<argc;i++) if(string(argv[i])=="-b") coeffBits = atoi(argv[i+1]);
    for(int i=1;i+1<argc;i++) if(string(argv[i])=="-q") qStep = static_cast<float>(atof(argv[i+1]));

    string inWav = argv[argc-2];
    string outBin = argv[argc-1];

    if(keepK > blockSize){
        cerr << "Error: K cannot exceed block size" << endl;
        return 1;
    }
    if(coeffBits < 2 || coeffBits > 24){
        cerr << "Error: bits must be in [2,24]" << endl;
        return 1;
    }

    SndfileHandle sfIn{inWav};
    if(sfIn.error()){ cerr << "Error: invalid input file" << endl; return 1; }
    if((sfIn.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV || (sfIn.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16){
        cerr << "Error: input must be WAV PCM_16" << endl; return 1;
    }
    if(sfIn.channels() != 1){
        cerr << "Error: mono only (1 channel)" << endl; return 1;
    }

    const size_t nFrames = static_cast<size_t>(sfIn.frames());
    vector<short> samples(nFrames);
    sfIn.readf(samples.data(), sfIn.frames());

    size_t nBlocks = (nFrames + blockSize - 1) / blockSize;
    vector<double> x(blockSize, 0.0);

    // DCT plans (in-place)
    fftw_plan planD = fftw_plan_r2r_1d(static_cast<int>(blockSize), x.data(), x.data(), FFTW_REDFT10, FFTW_ESTIMATE);

    // Open bitstream for writing
    fstream fs(outBin, ios::binary | ios::out | ios::trunc);
    if(!fs){ cerr << "Error: cannot open output file" << endl; return 1; }
    BitStream bs(fs, STREAM_WRITE);

    // Header
    bs.write_string("DCT1");
    write_u16(bs, 1); // version
    write_u32(bs, static_cast<uint32_t>(sfIn.samplerate()));
    write_u32(bs, static_cast<uint32_t>(nFrames));
    write_u16(bs, static_cast<uint16_t>(blockSize));
    write_u16(bs, static_cast<uint16_t>(keepK));
    write_u16(bs, static_cast<uint16_t>(coeffBits));
    write_f32(bs, qStep);

    if(verbose){
        cout << "Encoding " << inWav << " -> " << outBin << "\n";
        cout << "Frames=" << nFrames << ", Fs=" << sfIn.samplerate() << ", N=" << blockSize
             << ", K=" << keepK << ", bits/coeff=" << coeffBits << ", qStep=" << qStep << "\n";
    }

    // Process blocks
    for(size_t b=0; b<nBlocks; ++b){
        // load block with zero-padding
        size_t start = b * blockSize;
        size_t len = std::min(blockSize, nFrames - start);
        for(size_t i=0;i<blockSize;i++){
            if(i < len) x[i] = static_cast<double>(samples[start + i]);
            else x[i] = 0.0;
        }

        // DCT-II
        fftw_execute(planD);
        // Scale as in wav_dct: coefficients are divided by 2N
        double scale = 1.0 / (static_cast<double>(blockSize) * 2.0);
        for(size_t k=0;k<keepK;k++){
            double ck = x[k] * scale;
            int32_t q = static_cast<int32_t>( llround( ck / static_cast<double>(qStep) ) );
            // pack as unsigned bits two's complement
            uint32_t uq = to_u32(q, coeffBits);
            bs.write_n_bits(uq, coeffBits);
        }
    }

    bs.close();
    fftw_destroy_plan(planD);
    return 0;
}
