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
#ifndef WAVHIST_H
#define WAVHIST_H

#include <iostream>
#include <vector>
#include <map>
#include <sndfile.hh>

class WAVHist {
  private:
	std::vector<std::map<short, size_t>> counts; // Array of maps <sample value, count> - one per channel
	// counts[0][100] --> number of times Left channel saw sample 100 --> 2
	// counts[1][-50] --> number of times Right channel saw sample -50 --> 5
	
	std::map<short, size_t> mid_counts;   // histogram for MID channel  - only one channel
	// mid_counts[0] --> number of times MID channel saw sample 0 --> 10

	std::map<short, size_t> side_counts;  // histogram for SIDE channel - only one channel
	// side_counts[0] --> number of times SIDE channel saw sample 0 --> 15

  public:
	WAVHist(const SndfileHandle& sfh) {
		counts.resize(sfh.channels());
	}

	void update(const std::vector<short>& samples) {
		size_t n { };
		for(auto s : samples)
			counts[n++ % counts.size()][s]++;
	}

	void updateMid(const std::vector<short>& samples) {
		size_t n { };
		short previous_sample;
		short computed_sample;
		for(auto s : samples)
			if (n++ % 2 == 0){ //Left Channel
				previous_sample = s;
			} 
			else{ //Right Channel
				computed_sample = (s + previous_sample) / 2;
				mid_counts[computed_sample]++;
			}
	}

	void updateSide(const std::vector<short>& samples) {
		size_t n { };
		short previous_sample;
		short computed_sample;
		for(auto s : samples)
			if (n++ % 2 == 0){ //Left Channel
				previous_sample = s;
			} 
			else{ //Right Channel
				computed_sample = (previous_sample - s) / 2;
				side_counts[computed_sample]++;
			}
	}

	void dump(const size_t channel) const {
		for(auto [value, counter] : counts[channel])
			std::cout << value << '\t' << counter << '\n';
	}

	void dumpMid() const {
		for(auto [value, counter] : mid_counts)
			std::cout << value << '\t' << counter << '\n';
	}

	void dumpSide() const {
		for(auto [value, counter] : side_counts)
			std::cout << value << '\t' << counter << '\n';
	}



};

#endif

