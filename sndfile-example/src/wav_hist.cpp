//------------------------------------------------------------------------------
//
// Copyright 2025 University of Aveiro, Portugal, All Rights Reserved.
//
// These programs are supplied free of charge for research purposes only,
// and may not be sold or incorporated into any commercial product. There is
// ABSOLUTELY NO WARRANTY of any sort, nor any undertaking that they are
// fit for ANY PURPOSE WHATSOEVER. Use them at your own risk. If you do
// happen to find a bug, or have modifications to suggest, please report
// the same to Armando J. Pinho, ap@ua.pt. The copyright notice above
// and this statement of conditions must remain an integral part of each
// and every copy made of these files.
//
// Armando J. Pinho (ap@ua.pt)
// IEETA / DETI / University of Aveiro
//
#include <iostream>
#include <vector>
#include <string>
#include <sndfile.hh>
#include "wav_hist.h"

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading frames

int main(int argc, char *argv[]) {

    if(argc < 3) {
        cerr << "Usage: " << argv[0] << " <input file> <channel|mid|side> [bin_size]\n";
        return 1;
    }

    // open input WAV
    SndfileHandle sndFile { argv[1] };
    if(sndFile.error()) {
        cerr << "Error: invalid input file\n";
        return 1;
    }

    if((sndFile.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
        cerr << "Error: file is not in WAV format\n";
        return 1;
    }

    if((sndFile.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        cerr << "Error: file is not in PCM_16 format\n";
        return 1;
    }

    // parse channel argument
    string chanArg { argv[2] };
    bool dumpMid = false, dumpSide = false;
    int channel = -1; // default = invalid

    if(chanArg == "mid") {
        dumpMid = true;
    } else if(chanArg == "side") {
        dumpSide = true;
    } else {
        try {
            channel = stoi(chanArg);
            if(channel < 0 || channel >= sndFile.channels()) {
                cerr << "Error: invalid channel requested\n";
                return 1;
            }
        } catch(...) {
            cerr << "Error: invalid channel argument (must be number, 'mid' or 'side')\n";
            return 1;
        }
    }

    // optional bin size
    size_t bin_size = 1;
    if(argc >= 4) {
        try {
            bin_size = static_cast<size_t>(stoi(argv[3]));
            if(bin_size < 1) {
                cerr << "Error: bin_size must be >= 1\n";
                return 1;
            }
        } catch(...) {
            cerr << "Error: invalid bin_size argument\n";
            return 1;
        }
    }

    // build histogram
    size_t nFrames;
    vector<short> samples(FRAMES_BUFFER_SIZE * sndFile.channels());
    WAVHist hist { sndFile, bin_size };

    while((nFrames = sndFile.readf(samples.data(), FRAMES_BUFFER_SIZE))) {
        samples.resize(nFrames * sndFile.channels());
      	if(dumpMid) {
        	hist.updateMid(samples);
		} else if(dumpSide) {
			hist.updateSide(samples);
		} else {
			hist.update(samples); // per-channel
		}
    }

    // output histogram
    if(dumpMid) {
        hist.dumpMid();
    } else if(dumpSide) {
        hist.dumpSide();
    } else {
        hist.dump(channel);
    }

    return 0;
}
