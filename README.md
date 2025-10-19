# Audio Processing Project

This project contains a set of C++ programs for audio analysis and processing, developed using the **libsndfile** library.
It includes tools to copy, compare, compress, quantize, generate histograms, and apply various audio effects to WAV files.

---


### Main Executables

| Executable      | Description                                                     |
| --------------- | --------------------------------------------------------------- |
| `wav_cp`        | Copies a WAV file                                               |
| `wav_cmp`       | Compares two WAV files                                          |
| `wav_dct`       | Applies DCT-based compression                                   |
| `wav_quant`     | Performs audio quantization (encoding/decoding)                 |
| `wav_hist`      | Generates and saves histograms for audio channels               |
| `wav_effects`   | Applies audio effects (echo, multiple echoes, tremolo, vibrato) |
| `wav_quant_enc` |                                                                 |
| `wav_quant_dec` |                                                                 |
| `dct_enc`       | Lossy encoder for mono WAV; writes compact .dct (BitStream)     |
| `dct_dec`       | Decoder for .dct; reconstructes mono WAV                        |

---

## 🛠️ Build Instructions

Before building, make sure **libsndfile** is installed on your system:

```bash
sudo apt-get install libsndfile1-dev
```

Then build the executables:

```bash
cd sndfile-example/src
make
cd ../..
```

All binaries will be available inside `sndfile-example/bin`.

---


## 🧪 Testing

This section describes how to test each executable program included in the project.
Each subsection corresponds to a specific binary located in the `sndfile-example/bin` directory.

---

### 🔹 wav_cp

Copies the contents of one WAV file to another.

```bash
../bin/wav_cp <input-file.wav> <output-file.wav>
```

This command duplicates the input audio file and writes the copy to the specified output file.

---

### 🔹 wav_cmp

Compares two WAV files to analyze differences in their audio data.

```bash
../bin/wav_cmp <file1.wav> <file2.wav> [options]
```

Optional flags such as `-v` (verbose) can be used to display detailed comparison results.

---

### 🔹 wav_dct

Applies block DCT per channel and keeps only a fraction of low-frequency coefficients, reconstructing a lossy WAV.

Usage:

```bash
../bin/wav_dct [ -v ] [ -bs blockSize ] [ -frac dctFraction ] <input.wav> <output.wav>
```

Works with mono or stereo PCM_16 WAV.
It outputs a WAV with high-frequency content reduced. File size remains similar to the input.

---

### 🔹 wav_quant

Uniformly quantizes PCM samples to a target bit depth and writes a new WAV (container stays 16‑bit PCM).

Usage:

```bash
../bin/wav_quant [ -v ] -b <bits:1..16> <input.wav> <output.wav>
```

Input must be WAV PCM_16; output remains PCM_16 but amplitudes are snapped to 2^bits levels.
Assess distortion with `../bin/wav_cmp sample.wav q8.wav`.

---

### 🔹 wav_hist

Generates histograms for one or more channels of a WAV file, including mono, stereo, mid, and side representations.
Results can be redirected to a text file for visualization.

```bash
../bin/wav_hist <input-file.wav> <channel|mid|side> [bin-size] > <output-histogram>.txt
```

To visualize the histogram:

```bash
python3 test/plot_hist.py data/<output-histogram>.txt
```

This allows you to analyze amplitude distributions and compare how bin size affects histogram coarseness.

---

### 🔹 wav_effects

This program applies various audio effects such as echo, multiple echoes, amplitude modulation (tremolo), and vibrato (time-varying delay).
It can also compute and save histograms of the audio signal **before and after** applying the effect, allowing for a visual and analytical comparison of how each transformation changes the amplitude distribution.

The general usage format is:

`../bin/wav_effects <effect> <input-file.wav> <output-file.wav> [parameters...] [bin_size]`

Where:

* `<effect>` — one of the supported effects: `echo`, `multiecho`, `tremolo`, or `vib`
* `<input-file.wav>` — path to the source audio file
* `<output-file.wav>` — path for saving the processed audio file
* `[parameters...]` — effect-specific parameters such as delay, decay, modulation frequency, or depth
* `[bin_size]` *(optional)* — number of bins to use when generating histograms (default is 64 if not specified)

After execution, two histogram text files are automatically generated in the output directory:

* `<effect>_hist_before.txt` — histogram data of the input signal before the effect
* `<effect>_hist_after.txt` — histogram data of the processed signal after the effect

These histogram files can be visualized using the provided Python plotting script to better understand how the effect influences the signal’s distribution.

---

### 🔹 wav_quant_enc

TO BE COMPLETED

---

### 🔹 wav_quant_dec

TO BE COMPLETED

---

### 🔹 dct_enc

Lossy encoder for mono WAV based on block DCT + quantization. Writes a compact `.dct` bitstream using BitStream.

Usage:

```bash
../bin/dct_enc [ -v ] [ -bs N ] [ -k K ] [ -b bits ] [ -q step ] <input_mono.wav> <output.dct>
```

Input must be mono PCM_16 WAV. Use `wav_to_mono` to downmix.
Tune quality/size: increase `-k` (keep more DCT coeffs) and/or decrease `-q` (finer quantization) for higher quality; ensure `-b` is large enough to avoid coefficient clipping (e.g., 14–16).

---

### 🔹 dct_dec

Decoder for `.dct` files produced by `dct_enc`. Reconstructs a mono PCM_16 WAV via inverse DCT.

Usage:

```bash
../bin/dct_dec [ -v ] <input.dct> <output.wav>
```

---

## 📊 Histogram Visualization

A Python script is provided to visualize histograms generated by the C++ histogram tool, enabling easier analysis of channel distributions or quantization effects.

### Set up Python Environment

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### Example Usage

```bash
cd sndfile-example
./bin/wav_hist <input-file.wav> <channel|mid|side> <bin-size> > ./data/<output-histogram>.txt
python3 test/plot_hist.py data/<output-histogram>.txt
```

This workflow computes and plots the histogram of the selected audio channel using the specified bin size. The resulting plot provides a visual interpretation of the audio signal’s amplitude distribution.

