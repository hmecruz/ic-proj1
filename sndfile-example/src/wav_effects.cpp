#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <sndfile.hh>
#include "wav_hist.h"

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
        cerr << "Usage: " << argv[0] << " <effect> <input.wav> <output.wav> [params...] [bin_size]\n";
        cerr << "Effects:\n"
             << "  echo <delay_sec> <decay>\n"
             << "  multiecho <delay_sec> <decay> <repeats>\n"
             << "  tremolo <freq_Hz> <depth>\n"
             << "  vib <max_delay_sec> <freq_Hz>\n";
        cerr << "Optional:\n"
             << "  bin_size (default = 1)\n";
        return 1;
    }

    string effect = argv[1];
    string inputFile = argv[2];
    string outputFile = argv[3];

    // open input
    SndfileHandle sndFileIn { inputFile };
    if(sndFileIn.error()) {
        cerr << "Error: cannot open input file\n";
        return 1;
    }

    if((sndFileIn.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV ||
       (sndFileIn.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        cerr << "Error: input must be 16-bit PCM WAV\n";
        return 1;
    }

    size_t nFrames = static_cast<size_t>(sndFileIn.frames());
    int channels = sndFileIn.channels();
    int samplerate = sndFileIn.samplerate();

    vector<short> samples(nFrames * channels);
    sndFileIn.readf(samples.data(), nFrames);

    // Determine bin size (last argument if numeric)
    size_t bin_size = 1;
    if(argc > 4) {
        try {
            bin_size = static_cast<size_t>(stoi(argv[argc - 1]));
            if(bin_size < 1) bin_size = 1;
        } catch(...) {
            bin_size = 1;
        }
    }

    // Construct base path for histogram output
    string basePath = outputFile;
    size_t dotPos = basePath.find_last_of('.');
    if(dotPos != string::npos)
        basePath = basePath.substr(0, dotPos);

    // ---- HISTOGRAM BEFORE EFFECT ----
    WAVHist histBefore(sndFileIn, bin_size);
    histBefore.update(samples);

    string fileBefore = basePath + "_hist_before_" + effect + ".txt";
    {
        ofstream out(fileBefore);
        if(!out) {
            cerr << "Error: cannot write histogram before file\n";
        } else {
            for(const auto& [value, count] : histBefore.getChannelCounts(0))
                out << value << '\t' << count << '\n';
        }
    }

    // ---- APPLY EFFECT ----
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

    // ---- HISTOGRAM AFTER EFFECT ----
    WAVHist histAfter(sndFileIn, bin_size);
    histAfter.update(samples);

    string fileAfter = basePath + "_hist_after_" + effect + ".txt";
    {
        ofstream out(fileAfter);
        if(!out) {
            cerr << "Error: cannot write histogram after file\n";
        } else {
            for(const auto& [value, count] : histAfter.getChannelCounts(0))
                out << value << '\t' << count << '\n';
        }
    }

    // ---- SAVE OUTPUT ----
    SndfileHandle sndFileOut { outputFile, SFM_WRITE, sndFileIn.format(), channels, samplerate };
    sndFileOut.writef(samples.data(), nFrames);

    cout << "Effect '" << effect << "' applied successfully!\n";
    cout << "Histograms written:\n  " << fileBefore << "\n  " << fileAfter << "\n";
    cout << "Using bin size = " << bin_size << " (only channel 0)\n";

    return 0;
}
