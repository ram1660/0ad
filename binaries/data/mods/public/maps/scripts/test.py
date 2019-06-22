import os
from os import listdir
from os.path import isfile, join
test = os.path.dirname(os.path.dirname(os.path.abspath(""))) + "\\simulation\\templates\\units"
print(test)
print(os.path.exists(test))
onlyfiles = [f for f in listdir(test) if isfile(join(test, f))]
print(onlyfiles)