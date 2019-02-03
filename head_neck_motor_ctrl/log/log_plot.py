

import numpy as np
import matplotlib
import matplotlib.pyplot as plt

data = np.loadtxt('./log.txt', delimiter=',')

ts = data[:, 0]

ctrl_pos = data[:, 3:8]

pwm = data[:, 9:14]

pos = data[:, 15:21]

fig, ax = plt.subplots()
ax.plot(ts, pos)

plt.show()
