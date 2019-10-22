import os

directory = "/home/jlucas/Documents/bmps/"

files = os.listdir(directory[:-1])
files.sort()

for file in files:
    print(directory+file)