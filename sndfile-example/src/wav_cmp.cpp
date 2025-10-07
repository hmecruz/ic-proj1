#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sndfile.hh>

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536;

struct Metrics{
    long long count_samples = 0;
    long double sum_sq_error = 0.0L;
    long double sum_sq_signal = 0.0L;
    int max_abs_error = 0;
};

static void accumulate_metrics(const short *orig, const short *test, size_t frames, int channels, vector<Metrics> &per_ch, Metrics &mid_metrics) {
    for(size_t f = 0; f < frames; ++f){
        int L = 0, R = 0;
        bool have_stereo = (channels == 2);

        for(int c = 0; c < channels; ++c){
            int idx = static_cast<int>(f * channels + c);
            int x = static_cast<int>(orig[idx]);
            int y = static_cast<int>(test[idx]);
            int e = y - x;
            Metrics &m = per_ch[c];
            m.count_samples++;
            m.sum_sq_error += static_cast<long double>(e) * e;
            m.sum_sq_signal += static_cast<long double>(x) * x;
            int ae = std::abs(e);

            if(ae > m.max_abs_error){
                m.max_abs_error = ae;
            }

            if(have_stereo){
                if(c == 0){
                    L = x; 
                }else if(c == 1){
                    R = x;
                }
            }
        }

        // MID metrics: average of channels (L+R)/2 for original and test
        if (have_stereo){
            int idxL = static_cast<int>(f * channels + 0);
            int idxR = static_cast<int>(f * channels + 1);
            int x_mid = (static_cast<int>(orig[idxL]) + static_cast<int>(orig[idxR])) / 2; // integer division
            int y_mid = (static_cast<int>(test[idxL]) + static_cast<int>(test[idxR])) / 2; // integer division
            int e_mid = y_mid - x_mid;
            mid_metrics.count_samples++;
            mid_metrics.sum_sq_error += static_cast<long double>(e_mid) * e_mid;
            mid_metrics.sum_sq_signal += static_cast<long double>(x_mid) * x_mid;
            int ae_mid = std::abs(e_mid);
            
            if(ae_mid > mid_metrics.max_abs_error){
                mid_metrics.max_abs_error = ae_mid;
            }
        }
    }
}

static void print_metrics(const string &label, const Metrics &m) {
    if(m.count_samples == 0){
        return;
    }

    long double mse = m.sum_sq_error / static_cast<long double>(m.count_samples);
    long double snr_db;

    if(m.sum_sq_signal <= 0.0L || mse <= 0.0L){
        snr_db = INFINITY;
    }else{
        long double snr = m.sum_sq_signal / m.sum_sq_error;
        snr_db = 10.0L * log10(snr);
    }
    
    cout << label << "\n";
    cout << "  L2 (MSE): " << static_cast<double>(mse) << "\n";
    cout << "  L_inf (max abs err): " << m.max_abs_error << "\n";

    if(isinf(static_cast<double>(snr_db))){
        cout << "  SNR: inf dB\n";
    }else{
        cout << "  SNR: " << static_cast<double>(snr_db) << " dB\n";
    }
}

int main(int argc, char *argv[]) {
    bool verbose { false };

    if(argc < 3){
        cerr << "Usage: wav_cmp [ -v ] wavFileOriginal wavFileTest\n";
        return 1;
    }

    for(int n = 1; n < argc; ++n){
        if(string(argv[n]) == "-v"){ 
            verbose = true; 
            break; 
        }
    }

    SndfileHandle sfOrig { argv[argc-2] };
    if(sfOrig.error()){ 
        cerr << "Error: invalid original file\n"; 
        return 1; 
    }

    SndfileHandle sfTest { argv[argc-1] };
    if(sfTest.error()){ 
        cerr << "Error: invalid test file\n"; 
        return 1; 
    }

    auto check_wav16 = [](const SndfileHandle &sf) -> bool {
        if((sf.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV){
            return false;
        }

        if ((sf.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16){
            return false;
        }

        return true;
    };

    if(!check_wav16(sfOrig) || !check_wav16(sfTest)){
        cerr << "Error: both files must be WAV PCM_16\n";
        return 1;
    }

    if(sfOrig.channels() != sfTest.channels() || sfOrig.samplerate() != sfTest.samplerate()){
        cerr << "Error: files must have same number of channels and sample rate\n";
        return 1;
    }

    if(sfOrig.frames() != sfTest.frames()){
        cerr << "Warning: frame counts differ; comparing up to min length\n";
    }

    const int channels = sfOrig.channels();
    const sf_count_t total_frames = std::min(sfOrig.frames(), sfTest.frames());

    if(verbose){
        cout << "Comparing up to " << total_frames << " frames, " << channels << " channels, " << sfOrig.samplerate() << " Hz\n";
    }

    vector<Metrics> per_ch(channels);
    Metrics mid_metrics; // stereo only
    vector<short> bufOrig(FRAMES_BUFFER_SIZE * channels);
    vector<short> bufTest(FRAMES_BUFFER_SIZE * channels);
    sf_count_t frames_remaining = total_frames;
    
    while(frames_remaining > 0){
        sf_count_t to_read = std::min<sf_count_t>(frames_remaining, FRAMES_BUFFER_SIZE);
        sf_count_t r1 = sfOrig.readf(bufOrig.data(), to_read);
        sf_count_t r2 = sfTest.readf(bufTest.data(), to_read);
        sf_count_t r = std::min(r1, r2);
        if (r <= 0) break;
        accumulate_metrics(bufOrig.data(), bufTest.data(), static_cast<size_t>(r), channels, per_ch, mid_metrics);
        frames_remaining -= r;
    }

    // Print per-channel metrics
    for(int c = 0; c < channels; ++c){
        print_metrics("Channel " + to_string(c), per_ch[c]);
    }

    // Print MID metrics if stereo
    if(channels == 2){
        print_metrics("MID ( (L+R)/2 )", mid_metrics);
    }

    return 0;
}