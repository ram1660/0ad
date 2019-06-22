import UnitsGenerator as us
from xml.dom import minidom
import time, json, os, subprocess, csv, sys, threading
#E:\\ram-and-michael\\binaries\\data\\mods\public\\simulation\\templates\\units
#E:\\ram-and-michael\\binaries\\data\\mods\\public\\maps\\scenarios\\TestML.xml
NUMBER_OF_SIMULATIONS = 30000
GAME_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath("")))))) + "\\system\\pyrogenesis.exe"
UNITS_DIRECTORY = os.path.dirname(os.path.dirname(os.path.abspath(""))) + "\\simulation\\templates\\units"
MAP_SIM_NAME = "TestML.xml"
proc = 0
def main():
    print("Starting data collecting!")
    handle_processes()
    print("Simulation sesson has ended!")



def start_new_process():
    try:
        # p = subprocess.Popen([GAME_DIR, "-autostart-nonvisual", "-autostart=scenarios/TestML", "-autostart-aidiff=1:5", "-autostart-aidiff=2:5"])
        p = subprocess.Popen([GAME_DIR, "-autostart=scenarios/TestML", "-autostart-aidiff=1:5", "-autostart-aidiff=2:5"])
    except PermissionError:
        print("Error: You are not running the script as an administrator!")
        print("Existing program...")
        exit()
    print("Started a new instance of the game with the PID of: " + str(p.pid))
    return p

def handle_process(total_fights):
    global proc
    print("Trying to make a new game instance.")
    proc = start_new_process()
    print("Success!")
    print("Starting a new game handler instance.")
    handle_game(proc, total_fights)

def check_fight():
    current_fight = 0
    while True:
        with open("fightsCounter.txt", "r") as f:
            current_fight = f.readline()
        time.sleep(400)
        with open("fightsCounter.txt", "r") as f:
            if current_fight == f.readline():
                print("Something went wrong restarting script...")
                proc.kill()
                os.execv(sys.executable, ['python'] + sys.argv)

def handle_processes():
    total_fights = 1 # How many fights have been done?
    with open(os.path.abspath("") + "\\fightsCounter.txt", "r") as f:
        total_fights = int(f.readline())
    t = threading.Thread(target=check_fight)
    t.start()
    print("Getting the current fight counter.")
    print("Starting from: " + str(total_fights))
    while(total_fights < NUMBER_OF_SIMULATIONS):
        print("Starting new game!")
        print("Generating new map with random units!")
        us.generate_map()
        print("Done!")
        handle_process(total_fights)
        print("Updating fight counter file")
        total_fights += 1
        with open(os.path.abspath("") + "\\fightsCounter.txt", "w") as f:
            f.write(str(total_fights))


def handle_game(p, current_fight):
    file_data = 0
    while is_game_over(current_fight) != True:
        time.sleep(2)
    with open(os.path.abspath("") + "\\fightData\\fightData" + str(current_fight) + ".json", "r") as fd:
        file_data = json.load(fd)
    print("The game with the PID: " + str(p.pid) + " has ended. Killing process!")
    p.kill()
    print("Game killed!\n\n")
    print("------------")
    print("Updating results.csv!")
    add_results(file_data)
    print("Done!")
    print("------------\n\n")
    print("Cleaning old fighting data")
    delete_old_data(current_fight)
    print("Done!")

def delete_old_data(fight_id):
    os.remove(os.path.abspath("") + "\\fightData\\fightData" + str(fight_id) + ".json")
    os.remove(os.path.abspath("") + "\\fightStatus\\gameState" + str(fight_id) + ".txt")

def is_game_over(current_fight):
    if(os.path.exists(os.path.abspath("") + "\\fightStatus\\gameState" + str(current_fight) + ".txt") == False):
        print("Game state file hasn't been created yet!")
        return False
    with open(os.path.abspath("") + "\\fightStatus\\gameState" + str(current_fight) + ".txt", "r") as game_state_file:
        game_state = game_state_file.readlines()
    for line in game_state:
        if(line == "Ready\n"):
            return True
    return False

def add_results(data):
    csv_final_results = {}
    winner_player = str(data["Winner"])
    # Initializing the dictionary to 0
    # Looping through army 1
    for starting_units_1 in us.army_list:
        csv_final_results["sa1/units/" + starting_units_1] = 0
    # Looping through army 2
    for starting_units_2 in us.army_list:
        csv_final_results["sa2/units/" + starting_units_2] = 0
    # Looping through winner army
    for winner_units in us.army_list:
        csv_final_results["ea1/units/" + winner_units] = 0
    for winner_units in us.army_list:
        csv_final_results["ea2/units/" + winner_units] = 0
    # s = starting army
    # e = ending army
    # a1/a2= army 1/ army 2
    # Looping through winner army
    # Looping through army 1
    for starting_units_1 in data["Player1_Units"]:
        unit = translate_units_id_to_template(starting_units_1)
        if(unit != None):
            csv_final_results["sa1/" + unit] += 1
    # Looping through army 2
    for starting_units_2 in data["Player2_Units"]:
        unit = translate_units_id_to_template(starting_units_2)
        if(unit != None):
            csv_final_results["sa2/" + translate_units_id_to_template(starting_units_2)] += 1
    for winner_units in data["UnitsLeft"]:
        unit = translate_units_id_to_template(winner_units)
        if(unit != None):
            csv_final_results["ea" + winner_player + "/" + translate_units_id_to_template(winner_units)] += 1

    print("Looking for results.csv...")
    if os.path.exists(os.path.abspath("results.csv")) == True:
        print("File found!")
        print("Appending new data")
        with open(os.path.abspath("results.csv"), mode="a", newline='') as csv_file:
            fields_name = list(csv_final_results.keys())
            data_writer = csv.DictWriter(csv_file, fieldnames=fields_name)
            data_writer.writerow(csv_final_results)
    else:
        print("Couldn't find results.csv...")
        print("Creating new one.")

        with open(os.path.abspath("results.csv"), mode="w", newline='') as csv_file:
            fields_name = list(csv_final_results.keys())
            data_writer = csv.DictWriter(csv_file, fieldnames=fields_name)
            data_writer.writeheader()
            data_writer.writerow(csv_final_results)

def translate_units_id_to_template(id):
    map_xml = os.path.dirname((os.path.abspath(""))) + "\\scenarios\\" + MAP_SIM_NAME
    map_xml = minidom.parse(map_xml)
    entities = map_xml.getElementsByTagName("Entity")
    for entity in entities:
        e_id = entity.getAttribute("uid")
        if (str(id) == e_id):
            return entity.getElementsByTagName("Template")[0].firstChild.data

if __name__ == "__main__":
    main()