#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sndfile.hh>

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // buffer for reading/writing frames

//------------------------------------------------------------------------------
// Echo effect
//------------------------------------------------------------------------------
void applyEcho(vector<short>& samples, int channels, int samplerate, double delaySec, double decay) {
    size_t delaySamples = static_cast<size_t>(delaySec * samplerate);
    vector<short> original = samples;

    for(size_t i = delaySamples; i < samples.size() / channels; i++) {
        for(int c = 0; c < channels; c++) {
            size_t idx = i * channels + c;
            size_t delayedIdx = (i - delaySamples) * channels + c;
            int val = static_cast<int>(samples[idx]) + static_cast<int>(original[delayedIdx] * decay);
            if(val > 32767) val = 32767;
            if(val < -32768) val = -32768;
            samples[idx] = static_cast<short>(val);
        }
    }
}

//------------------------------------------------------------------------------
// Multiple echoes
//------------------------------------------------------------------------------
void applyMultiEcho(vector<short>& samples, int channels, int samplerate,
                    double delaySec, double decay, int repeats) {
    for(int i = 0; i < repeats; i++) {
        applyEcho(samples, channels, samplerate, delaySec * (i + 1), pow(decay, i + 1));
    }
}

//------------------------------------------------------------------------------
// Tremolo (Amplitude modulation)
//------------------------------------------------------------------------------
void applyAmplitudeMod(vector<short>& samples, int channels, int samplerate, double freq, double depth) {
    for(size_t i = 0; i < samples.size() / channels; i++) {
        double mod = 1.0 + depth * sin(2 * M_PI * freq * i / samplerate);
        for(int c = 0; c < channels; c++) {
            size_t idx = i * channels + c;
            int val = static_cast<int>(samples[idx] * mod);
            if(val > 32767) val = 32767;
            if(val < -32768) val = -32768;
            samples[idx] = static_cast<short>(val);
        }
    }
}

//------------------------------------------------------------------------------
// Vibrato (Time-varying delay)
//------------------------------------------------------------------------------
void applyTimeVaryingDelay(vector<short>& samples, int channels, int samplerate,
                           double maxDelaySec, double freq) {
    size_t maxDelaySamples = static_cast<size_t>(maxDelaySec * samplerate);
    vector<short> original = samples;

    for(size_t i = maxDelaySamples; i < samples.size() / channels; i++) {
        double delaySamples = maxDelaySamples * (0.5 + 0.5 * sin(2 * M_PI * freq * i / samplerate));
        size_t delayedIdx = static_cast<size_t>(i - delaySamples);
        for(int c = 0; c < channels; c++) {
            size_t idx = i * channels + c;
            if(delayedIdx * channels + c < original.size())
                samples[idx] = original[delayedIdx * channels + c];
        }
    }
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if(argc < 4) {
        cerr << "Usage: " << argv[0] << " <effect> <input.wav> <output.wav> [params...]\n";
        cerr << "Effects:\n"
             << "  echo <delay_sec> <decay>\n"
             << "  multiecho <delay_sec> <decay> <repeats>\n"
             << "  tremolo <freq_Hz> <depth>\n"
             << "  vib <max_delay_sec> <freq_Hz>\n";
        return 1;
    }

    string effect = argv[1];
    string inputFile = argv[2];
    string outputFile = argv[3];

    SndfileHandle sndFileIn { inputFile };
    if(sndFileIn.error()) {
        cerr << "Error: cannot open input file\n";
        return 1;
    }

    if((sndFileIn.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
        cerr << "Error: input is not a WAV file\n";
        return 1;
    }

    if((sndFileIn.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        cerr << "Error: input must be PCM_16 format\n";
        return 1;
    }

    size_t nFrames = static_cast<size_t>(sndFileIn.frames());
    int channels = sndFileIn.channels();
    int samplerate = sndFileIn.samplerate();

    vector<short> samples(nFrames * channels);
    sndFileIn.readf(samples.data(), nFrames);

    // Apply effect
    if(effect == "echo" && argc >= 6) {
        double delay = atof(argv[4]);
        double decay = atof(argv[5]);
        applyEcho(samples, channels, samplerate, delay, decay);
    }
    else if(effect == "multiecho" && argc >= 7) {
        double delay = atof(argv[4]);
        double decay = atof(argv[5]);
        int repeats = atoi(argv[6]);
        applyMultiEcho(samples, channels, samplerate, delay, decay, repeats);
    }
    else if(effect == "tremolo" && argc >= 6) {
        double freq = atof(argv[4]);
        double depth = atof(argv[5]);
        applyAmplitudeMod(samples, channels, samplerate, freq, depth);
    }
    else if(effect == "vib" && argc >= 6) {
        double maxDelay = atof(argv[4]);
        double freq = atof(argv[5]);
        applyTimeVaryingDelay(samples, channels, samplerate, maxDelay, freq);
    }
    else {
        cerr << "Error: invalid effect or missing parameters\n";
        return 1;
    }

    SndfileHandle sndFileOut { outputFile, SFM_WRITE, sndFileIn.format(), channels, samplerate };
    if(sndFileOut.error()) {
        cerr << "Error: cannot create output file\n";
        return 1;
    }

    sndFileOut.writef(samples.data(), nFrames);

    cout << "Effect '" << effect << "' applied successfully!\n";
    return 0;
}