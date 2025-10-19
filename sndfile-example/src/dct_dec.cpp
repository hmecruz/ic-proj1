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

static uint32_t read_u32(BitStream &bs){
    uint32_t b0 = static_cast<uint32_t>(bs.read_n_bits(8));
    uint32_t b1 = static_cast<uint32_t>(bs.read_n_bits(8));
    uint32_t b2 = static_cast<uint32_t>(bs.read_n_bits(8));
    uint32_t b3 = static_cast<uint32_t>(bs.read_n_bits(8));
    return (b0<<24) | (b1<<16) | (b2<<8) | b3;
}

static uint16_t read_u16(BitStream &bs){
    uint16_t b0 = static_cast<uint16_t>(bs.read_n_bits(8));
    uint16_t b1 = static_cast<uint16_t>(bs.read_n_bits(8));
    return static_cast<uint16_t>((b0<<8) | b1);
}

static float read_f32(BitStream &bs){
    uint32_t u = read_u32(bs);
    float f;
    memcpy(&f, &u, sizeof(f));
    return f;
}

static int32_t sign_extend(uint32_t v, int bits){
    if(bits == 32) return static_cast<int32_t>(v);
    uint32_t m = 1u << (bits-1);
    uint32_t mask = (1u<<bits) - 1u;
    v &= mask;
    if(v & m){
        return static_cast<int32_t>(v | (~mask));
    } else {
        return static_cast<int32_t>(v);
    }
}

int main(int argc, char* argv[]){
    bool verbose = false;
    if(argc < 3){
        cerr << "Usage: dct_dec [ -v ] input.dct output.wav\n";
        return 1;
    }
    for(int i=1;i<argc;i++) if(string(argv[i])=="-v") verbose=true;

    string inBin = argv[argc-2];
    string outWav = argv[argc-1];

    fstream fs(inBin, ios::binary | ios::in);
    if(!fs){ cerr << "Error: cannot open input file" << endl; return 1; }
    BitStream bs(fs, STREAM_READ);

    // Header
    string magic = bs.read_string();
    if(magic != "DCT1"){ cerr << "Error: invalid file (magic)" << endl; return 1; }
    uint16_t version = read_u16(bs); (void)version;
    uint32_t samplerate = read_u32(bs);
    uint32_t totalFrames = read_u32(bs);
    uint16_t blockSize = read_u16(bs);
    uint16_t keepK = read_u16(bs);
    uint16_t coeffBits = read_u16(bs);
    float qStep = read_f32(bs);

    if(keepK > blockSize){ cerr << "Corrupt header: K>N" << endl; return 1; }

    if(verbose){
        cout << "Decoding " << inBin << " -> " << outWav << "\n";
        cout << "Frames=" << totalFrames << ", Fs=" << samplerate << ", N=" << blockSize
             << ", K=" << keepK << ", bits/coeff=" << coeffBits << ", qStep=" << qStep << "\n";
    }

    size_t nBlocks = (static_cast<size_t>(totalFrames) + blockSize - 1) / blockSize;
    vector<double> x(blockSize, 0.0);
    vector<short> out(static_cast<size_t>(nBlocks) * blockSize);

    // Inverse DCT (REDFT01)
    fftw_plan planI = fftw_plan_r2r_1d(blockSize, x.data(), x.data(), FFTW_REDFT01, FFTW_ESTIMATE);

    for(size_t b=0;b<nBlocks;++b){
        for(size_t k=0;k<blockSize;k++) x[k]=0.0;
        for(size_t k=0;k<keepK;k++){
            uint32_t uq = static_cast<uint32_t>(bs.read_n_bits(coeffBits));
            int32_t q = sign_extend(uq, coeffBits);
            double ck = static_cast<double>(q) * static_cast<double>(qStep);
            x[k] = ck;
        }
        fftw_execute(planI);
        for(size_t i=0;i<blockSize;i++){
            long v = lround(x[i]);
            if(v>32767) v=32767;
            if(v<-32768) v=-32768;
            out[b*blockSize + i] = static_cast<short>(v);
        }
    }

    // Write WAV
    SndfileHandle sfOut{outWav, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, static_cast<int>(samplerate)};
    if(sfOut.error()){ cerr << "Error: cannot open output wav" << endl; return 1; }
    sfOut.writef(out.data(), totalFrames);

    bs.close();
    fftw_destroy_plan(planI);
    return 0;
}
