import matplotlib.pyplot as plt
import numpy as np
from scipy.fftpack import fft, fftfreq

f = 10  # Frequency, in cycles per second, or Hertz
f_s = 100  # Sampling rate, or number of measurements per second

t = np.linspace(0, 2, 2 * f_s, endpoint=False)
y = np.sin(f * 2 * np.pi * t)


fig, ax = plt.subplots()
ax.plot(t, y)


X = fft(y)
freqs = fftfreq(len(y), d = 1/f_s) # f_s could be 1/baudrate

 # print the positive frequency that has the most correlation
print(freqs[np.argmax(np.abs(X[0:int(len(X)/2)]))])

fig2, ax2 = plt.subplots()

ax2.stem(freqs, np.abs(X))
ax2.set_xlabel('Frequency in Hertz [Hz]')
ax2.set_ylabel('Frequency Domain (Spectrum) Magnitude')
ax2.set_xlim(-f_s / 2, f_s / 2)
ax2.set_ylim(-5, 110)

# plt.show()
