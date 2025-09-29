#include <iostream>
#include <vector>
#include <string>
#include <sndfile.hh>

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading/writing frames

static inline short quantize_sample(short s, int bits) {
    if(bits >= 16) return s; // No change
    const int step = 1 << (16 - bits); // power-of-two step size
    int x = static_cast<int>(s);
    // Round to nearest multiple of step (symmetric rounding around 0)
    int q = ((x + (x >= 0 ? step/2 : -step/2)) / step) * step;
    // Clamp to 16-bit range just in case
    if(q > 32767) q = 32767;
    if(q < -32768) q = -32768;
    return static_cast<short>(q);
}

int main(int argc, char *argv[]) {
    bool verbose { false };
    int bits { -1 };

    if(argc < 4) {
        cerr << "Usage: wav_quant [ -v ] -b bits wavFileIn wavFileOut\n";
        cerr << "  bits: number of quantization bits (1..16).\n";
        return 1;
    }

    for(int n = 1 ; n < argc ; n++){
        if(string(argv[n]) == "-v") {
            verbose = true;
            break;
        }
    }

    for(int n = 1 ; n < argc-1 ; n++){
        if(string(argv[n]) == "-b" && n+1 < argc) {
            bits = atoi(argv[n+1]);
            break;
        }
    }

    if(bits <= 0 || bits > 16) {
        cerr << "Error: bits must be in 1..16\n";
        return 1;
    }

    SndfileHandle sfhIn { argv[argc-2] };
    if(sfhIn.error()) {
        cerr << "Error: invalid input file\n";
        return 1;
    }

    if((sfhIn.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
        cerr << "Error: file is not in WAV format\n";
        return 1;
    }

    if((sfhIn.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        cerr << "Error: file is not in PCM_16 format\n";
        return 1;
    }

    SndfileHandle sfhOut { argv[argc-1], SFM_WRITE, sfhIn.format(), sfhIn.channels(), sfhIn.samplerate() };
    if(sfhOut.error()) {
        cerr << "Error: invalid output file\n";
        return 1;
    }

    if(verbose) {
        cout << "Input file has:\n";
        cout << '\t' << sfhIn.frames() << " frames\n";
        cout << '\t' << sfhIn.samplerate() << " samples per second\n";
        cout << '\t' << sfhIn.channels() << " channels\n";
        cout << "Quantizing to " << bits << " bits per sample (uniform).\n";
    }

    size_t nFrames;
    vector<short> samples(FRAMES_BUFFER_SIZE * sfhIn.channels());
    while((nFrames = sfhIn.readf(samples.data(), FRAMES_BUFFER_SIZE))) {
        // Quantize all samples in-place
        size_t count = nFrames * static_cast<size_t>(sfhIn.channels());
        for(size_t i = 0; i < count; ++i) {
            samples[i] = quantize_sample(samples[i], bits);
        }
        sfhOut.writef(samples.data(), nFrames);
    }

    return 0;
}
