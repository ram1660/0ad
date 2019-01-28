import math
import threading
import time
import os, shutil, inspect
data_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) + '\\DataFiles'
answer_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) + '\\AnswerFiles'
file = 0
def main():
	current_file_index = 0
	#threading._start_new_thread()
	print("Starting ML loop")
	clear_folders_content()
	while True:
		time.sleep(2)
		try:
			file = open("DataFiles/data" + str(current_file_index) + ".txt", "r")
			print("File found analysing")
			current_file_index = handle_data(file, current_file_index)
			file.close()
		except IOError as identifier:
			print("File doesn't exists yet!")
			continue
		
def handle_data(file, current_file_index):
	print("Checking data")
	with open("AnswerFiles/answer" + str(current_file_index) + ".txt", "w") as af:
		for line in file:
			print("Data of file: " + line)
			if(line == "I want to kill you\n"):	
				af.writelines("Please don't :)")
		print("The current answer file index is: " + str(current_file_index))
		af.close()
	current_file_index += 1
	return current_file_index

def clear_folders_content():
	print("Cleaning ML folders")
	for the_file in os.listdir(data_dir):
		file_path = os.path.join(data_dir, the_file)
		try:
			if os.path.isfile(file_path):
				os.unlink(file_path)
		except Exception as e:
			print(e)
	for the_file in os.listdir(answer_dir):
		file_path = os.path.join(answer_dir, the_file)
		try:
			if os.path.isfile(file_path):
				os.unlink(file_path)
		except Exception as e:
			print(e)

if __name__ == '__main__':
	main()