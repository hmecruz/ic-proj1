// Simple converter: any-channel PCM16 WAV -> mono by averaging channels
#include <sndfile.hh>
#include <vector>
#include <iostream>
#include <cmath>
using namespace std;

int main(int argc, char* argv[]){
    if(argc < 3){
        cerr << "Usage: wav_to_mono input.wav output.wav\n";
        return 1;
    }
    SndfileHandle in{argv[1]};
    if(in.error()){ cerr << "Error: cannot open input" << endl; return 1; }
    if((in.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV || (in.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16){
        cerr << "Error: input must be WAV PCM_16" << endl; return 1; }
    int C = in.channels();
    if(C == 1){ cerr << "Input already mono; copying" << endl; }
    vector<short> buf(static_cast<size_t>(in.frames()) * C);
    in.readf(buf.data(), in.frames());
    vector<short> out(static_cast<size_t>(in.frames()));
    for(sf_count_t n=0;n<in.frames();++n){
        long sum=0;
        for(int c=0;c<C;++c) sum += buf[static_cast<size_t>(n)*C + c];
        long v = lround(static_cast<double>(sum) / C);
        if(v>32767) v=32767; if(v<-32768) v=-32768;
        out[static_cast<size_t>(n)] = static_cast<short>(v);
    }
    SndfileHandle outH{argv[2], SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, in.samplerate()};
    if(outH.error()){ cerr << "Error: cannot open output" << endl; return 1; }
    outH.writef(out.data(), in.frames());
    return 0;
}
