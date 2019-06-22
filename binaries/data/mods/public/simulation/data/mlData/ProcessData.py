import pickle
import json
import os
import pandas as pd
import numpy as np
import time

import atexit

#D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\\\
#E:\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData
#D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\\\
with open("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\classifier.pkl", 'rb') as f:
    model = pickle.load(f)
with open("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\proba_rescale.pkl", 'rb') as f:
    y_proba_rescaling = pickle.load(f)
gamestate_counter = 0 # times that we removed gamestate. 

def exit_handler():
    if(os.path.exists("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\answer.txt")):
        os.remove("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\answer.txt")
    if(os.path.exists("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\gameState.txt")):
        os.remove("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\gameState.txt")
    if(os.path.exists("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json")):
        os.remove("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json")


def proc_response(res):
    num_to_write = ' '.join(map(str, res))
    num_to_write = str(round(float(num_to_write), 2))
    return num_to_write

def Create_dataset():
        json_lines = ""
        while not (os.path.exists("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json")):
            pass
        while True:
            try:
                os.rename("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json","D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json")
                break
            except Exception as e:
                print("Failed to Rename retards!" + str(e)) 
        with open("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json",'r') as json_text:
            json_lines = json_text.readline()
            json_lines = json_lines.replace("\0","")
        json_file = json.loads(json_lines)
        x = np.array(list(json_file.values()))
        x.shape = (1,832)    
        return x
    
    
def linear_extrapolation(x1, y1, x2, y2, x):
    t = (x - x1) / (x2 - x1)
    return (1 - t) * y1 + t * y2


def apply_model():
    x_data = Create_dataset()
   
    rescaling_inputs = list(sorted(y_proba_rescaling.keys()))


    y_proba = model.predict_proba(x_data)
    print(y_proba)
    y_proba = y_proba[:, 1]
    
    for i, rescaling_input in enumerate(rescaling_inputs):
        if rescaling_input >= y_proba:
            break
            
    if i == 0:
        return y_proba_rescaling[rescaling_inputs[0]]
    else:
        return linear_extrapolation(rescaling_inputs[i - 1], y_proba_rescaling[rescaling_inputs[i - 1]], rescaling_inputs[i], \
                y_proba_rescaling[rescaling_inputs[i]], y_proba)
        
atexit.register(exit_handler)
while(True):
        print("!!!!!!!!!!!!!!!!!!!!!!!     " + str(gamestate_counter))
        while True:
            if(os.path.exists("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json")):
                try:
                    os.rename("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json","D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json")
                    print("Units can be used!")
                    break
                except Exception as e:
                    print(e)
                    print("Cannot use unitsData.json, game is still using it.")
        res = apply_model()
        res = proc_response(res)
        while True:
            try:
                os.remove("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\unitsData.json") # Remove unitsData AFTER we applied to our model.
                print("Deleting units")
                break
            except Exception as e:
                print("Cannot delete units, someone is using it?")
        
        with open("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\answer.txt",'w') as res_file:
            res_file.write(res)
        
        print("Answer created waiting for gameState.txt")
        while not (os.path.isfile("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\gameState.txt")):
            pass
        print("FOUND GAMESTATE! Removing Answer.txt.....") 
        while True:
            try:
                os.remove("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\answer.txt")
              
                break
            except Exception as e:
                print("Failed to remove answer, still in use\n" + str(e))
        
        while True:
            try: 
                os.remove("D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\gameState.txt") 
                gamestate_counter += 1
                break
            except Exception as e:
                print("Failed to remove gameState, still in use\n" + str(e))
        print("GameState removed!")