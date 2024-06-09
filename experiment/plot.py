import numpy as np
from matplotlib import pyplot as plt

x = np.array([0, 5, 10, 15, 20, 25, 30, 35, 40])
y = np.array([x / 1000 for x in [1000, 1000, 1000, 978, 973, 916, 848, 804, 740]])
y2 = np.array([x / 1000 for x in [1000, 957, 912, 865, 818, 763, 691, 643, 590]])
plt.xlabel('Packet Loss (%)')
plt.ylabel('Recovery Rate (%)')
plt.grid(True)
plt.plot(x, y, 'r-o')
plt.plot(x, y2, 'b-o')
plt.legend(['RLNC(12, 8)', 'without FEC'])
plt.show()
plt.savefig('plot.png')