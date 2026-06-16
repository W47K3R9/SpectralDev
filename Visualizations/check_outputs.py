import numpy as np
from matplotlib import pyplot as plt

original_samples_array = np.zeros(1024)
resynth_samples_array = np.zeros(1024)


def fill_array_with_read_vals(array_to_fill, text_file):
    with open(text_file) as f:
        text_output = f.readlines()
        print(text_output[300])
        for index, value in enumerate(text_output):
            value = value.strip('\n')
            array_to_fill[index] = float(value)


# fill_array_with_read_vals(arr_with_sine, "../cmake-build-debug/raw_sine_values.txt")
# fill_array_with_read_vals(arr_with_fft_calc, "../cmake-build-debug/resynthesized_values.txt")

fill_array_with_read_vals(original_samples_array, "../cmake-build-debug/real_voice.txt")
fill_array_with_read_vals(resynth_samples_array, "../cmake-build-debug/resynthesized_voice.txt")

samplerate = 44100
samples = 1024

# Create a figure with 2 rows and 2 columns for subplots
plt.figure(figsize=(20, 12))  # Adjust figure size for better readability

# --- Original Waveform (Upper Left) ---
plt.subplot(2, 2, 1)  # (rows, columns, plot_number)
plt.plot(original_samples_array)
plt.title('Original Waveform')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.grid(True)

# --- Original FFT (Lower Left) ---
plt.subplot(2, 2, 3)  # (rows, columns, plot_number)
yf_original = np.fft.fft(original_samples_array)
xf = np.fft.fftfreq(samples, 1 / samplerate)
plt.plot(xf[:samples // 2], 2.0 / samples * np.abs(yf_original[0:samples // 2]))
plt.title('Original FFT Magnitude Spectrum')
plt.xlabel('Frequency (Hz)')
plt.xscale('log')
plt.ylabel('Magnitude')
plt.grid(True)

# --- Resynthesized Waveform (Upper Right) ---
plt.subplot(2, 2, 2)  # (rows, columns, plot_number)
plt.plot(resynth_samples_array)
plt.title('Resynthesized Waveform')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.grid(True)

# --- Resynthesized FFT (Lower Right) ---
plt.subplot(2, 2, 4)  # (rows, columns, plot_number)
yf_resynthesized = np.fft.fft(resynth_samples_array)
plt.plot(xf[:samples // 2], 2.0 / samples * np.abs(yf_resynthesized[0:samples // 2]))
plt.title('Resynthesized FFT Magnitude Spectrum')
plt.xlabel('Frequency (Hz)')
plt.xscale('log')
plt.ylabel('Magnitude')
plt.grid(True)

plt.tight_layout()  # Adjust layout to prevent subplots from overlapping
plt.show()  # Display the plots
