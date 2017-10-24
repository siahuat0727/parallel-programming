import matplotlib.pyplot as plt

x = [1, 2, 4, 8, 13, 15]
y = [283.82, 141.75, 106.72, 60.90, 58.86, 48.19]
plt.xlabel("number of cores")
plt.ylabel("time (seconds)")
plt.title("plot of time against number of cores")
plt.plot(x, y)
plt.plot(x, y, 'bo')
plt.show()
