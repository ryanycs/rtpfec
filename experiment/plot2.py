import numpy as np
from matplotlib import pyplot as plt

x = np.array([20, 40, 60, 80, 100, 120, 140])
y = np.array([0.9747, 0.9926, 0.9471, 0.9518, 0.9246, 0.9423, 0.9298])
plt.xlabel('Time (s)')
plt.ylabel('Recovery Rate (%)')
plt.grid(True)
plt.plot(x, y, 'r-o')
plt.legend(['RLNC'])
plt.show()
plt.savefig('plot2.png')