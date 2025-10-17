#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "../../bit_stream/src/bit_stream.h"
#include <sndfile.hh>

using namespace std;

int main(int argc, char *argv[]){
    if(argc < 3){
        cerr << "Usage: wav_quant_dec input.qnt output.wav\n";
        return 1;
    }

    fstream in(argv[1], ios::in | ios::binary);
    if(!in.is_open()){
        cerr << "Error: cannot open input file\n";
        return 1;
    }
    BitStream bs(in, STREAM_READ);

    string format = bs.read_string();
    if(format != "QNT1"){
        cerr << "Error: invalid input file format\n";
        return 1;
    }

    uint32_t sample_rate = static_cast<uint32_t>(bs.read_n_bits(32));
    uint16_t channels = static_cast<uint16_t>(bs.read_n_bits(16));
    uint8_t bits = static_cast<uint8_t>(bs.read_n_bits(8));
    uint32_t total_frames = static_cast<uint32_t>(bs.read_n_bits(32));

    vector<short> samples(total_frames * channels);
    for(size_t i=0; i<samples.size(); i++){
        uint32_t code = static_cast<uint32_t>(bs.read_n_bits(bits));

        short sample = static_cast<short>((code << (16-bits)) - 32768);
        samples[i] = sample;
    }

    SndfileHandle sfOut(argv[2], SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, channels, sample_rate);
    if(sfOut.error()){
        cerr << "Error: cannot open output WAV\n";
        return 1;
    }   

    sfOut.writef(samples.data(), total_frames);

    cout << "Decoded " << argv[1] << " into " << argv[2] << " successfully.\n";
    return 0;
}