import numpy as np
from matplotlib import pyplot as plt

arr_with_sine = np.zeros(1024)
arr_with_fft_calc = np.zeros(1024)

def fill_array_with_read_vals(array_to_fill, text_file):
    with open(text_file) as f:
        text_output = f.readlines()
        print(text_output[300])
        for index, value in enumerate(text_output):
            value = value.strip('\n')
            array_to_fill[index] = float(value)


fill_array_with_read_vals(arr_with_sine, "../cmake-build-debug/raw_sine_values.txt")
fill_array_with_read_vals(arr_with_fft_calc, "../cmake-build-debug/resynthesized_values.txt")

# arr_with_fft_calc = - 20 * np.log10(arr_with_fft_calc)
x_axis = np.arange(0, 1024, 1)

lib_algo_fft = np.fft.fft(arr_with_sine)
lib_algo_fft = np.abs(lib_algo_fft)

fig,ax = plt.subplots(2)
ax[0].plot(x_axis, arr_with_sine)
ax[1].plot(x_axis, arr_with_fft_calc)
plt.show()
