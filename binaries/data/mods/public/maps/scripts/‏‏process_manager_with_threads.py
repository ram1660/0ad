#import UnitsGenerator as us
from threading import Lock, Thread
from xml.dom import minidom
import time, json, os, subprocess, csv, pdb
INSTANCES_PARALLEL = 3
NUMBER_OF_SIMULATIONS = 30000
GAME_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath("")))))) + "\\system\\pyrogenesis.exe"
MAP_SIM_NAME = "TestML.xml"
STARTING_SIM_POS = 1
proccesses = []
csv_lock = Lock()
map_lock = Lock()
def main():
    #us.main() # Calls to generate units
    print("Starting data collecting!")
    handle_processes()
    print("Simulation sesson has ended!")



def start_new_process():
    try:
        p = subprocess.Popen([GAME_DIR, "-autostart=scenarios/TestML", "-autostart-aidiff=1:5", "-autostart-aidiff=2:5"])
    except PermissionError:
        print("Error: You are not running the script as an administrator!")
        print("Existing program...")
        exit()
    print("Started a new instance of the game with the PID of: " + str(p.pid))
    return p

def handle_process(total_fights):
    print("Getting current fight counter.")
    print("Starting from: " + str(total_fights))
    print("Getting starting units.")
    starting_units = get_starting_units()
    print("Success!")
    print("Trying to make a new game instance.")
    proc = start_new_process()
    print("Success!")
    print("Starting a new game handler instance.")
    handle_game(proc, total_fights)


def handle_processes():
    total_fights = 1 # How many fights have been done?
    with open(os.path.abspath("") + "\\fightsCounter.txt", "r") as f:
        total_fights = int(f.readline())
    while(total_fights < NUMBER_OF_SIMULATIONS):
        while (len(proccesses) != INSTANCES_PARALLEL):
            print("Starting a new thread!")
            t = Thread(target=handle_process, args=(total_fights,))
            t.start()
            proccesses.append(t)
            print("Updating fight counter file")
            total_fights += 1
            with open(os.path.abspath("") + "\\fightsCounter.txt", "w") as f:
                f.write(str(total_fights))
            time.sleep(5)


def handle_game(p, current_fight):
    file_data = 0
    while check_file(current_fight) != True:
        time.sleep(1)
    with open(os.path.abspath("") + "\\fightData\\fightData" + str(current_fight) + ".json", "r") as fd:
        file_data = json.load(fd)
    print("The game with the PID: " + str(p.pid) + " has ended. Killing process!")
    p.kill()
    print("Game killed!\n\n")
    print("------------")
    print("Updating results.csv!")
    add_results(file_data)
    print("Done!")
    print("------------")


def check_file(current_fight):
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
    new_dic = {} # Fix that shitty name
    winner_player = data["Winner"]

    # Looping through army 1
    for starting_units_1 in data["Player1_Units"]:
        new_dic["sa1/" + str(translate_units_id_to_template(starting_units_1))] = 0
    # Looping through army 2
    for starting_units_2 in data["Player2_Units"]:
        new_dic["sa2/" + translate_units_id_to_template(starting_units_2)] = 0
    # Looping through winner army
    for winner_units in data["UnitsLeft"]:
        new_dic["ea" + winner_player + "/" + translate_units_id_to_template(winner_units)] = 0

    # s = starting army
    # e = ending army
    # a1/a2= army 1/ army 2
    # Looping through army 1
    for starting_units_1 in data["Player1_Units"]:
        new_dic["sa1/" + str(translate_units_id_to_template(starting_units_1))] += 1
    # Looping through army 2
    for starting_units_2 in data["Player2_Units"]:
        new_dic["sa2/" + translate_units_id_to_template(starting_units_2)] += 1
    # Looping through winner army
    for winner_units in data["UnitsLeft"]:
        new_dic["ea" + winner_player + "/" + translate_units_id_to_template(winner_units)] += 1
    print("Looking for results.csv...")
    if os.path.exists(os.path.abspath("results.csv")) == True:
        print("File found!")
        print("Appending new data")
        csv_lock.acquire()
        with open(os.path.abspath("results.csv"), mode="a", newline='') as csv_file:
            fields_name = list(new_dic.keys())
            data_writer = csv.DictWriter(csv_file, fieldnames=fields_name)
            data_writer.writerow(new_dic)
        csv_lock.release()
    else:
        print("Couldn't find results.csv...")
        print("Creating new one.")

        csv_lock.acquire()
        with open(os.path.abspath("results.csv"), mode="w", newline='') as csv_file:
            fields_name = list(new_dic.keys())
            data_writer = csv.DictWriter(csv_file, fieldnames=fields_name)
            #pdb.set_trace()
            data_writer.writeheader()
            data_writer.writerow(new_dic)
        csv_lock.release()

def translate_units_id_to_template(id):
    map_xml = os.path.dirname((os.path.abspath(""))) + "\\scenarios\\" + MAP_SIM_NAME
    map_xml = minidom.parse(map_xml)
    entities = map_xml.getElementsByTagName("Entity")
    for entity in entities:
        e_id = entity.getAttribute("uid")
        if (str(id) == e_id):
            return entity.getElementsByTagName("Template")[0].firstChild.data

def get_starting_units():
    units = {}
    player1_units = []
    player2_units = []
    map_xml = os.path.dirname(os.path.abspath("")) + "\\scenarios\\" + MAP_SIM_NAME
    map_lock.acquire()
    map_xml = minidom.parse(map_xml)
    map_lock.release()
    entities = map_xml.getElementsByTagName("Entity")
    for entity in entities:
        p_num = entity.getElementsByTagName("Player")[0].firstChild.data
        print(p_num)
        e_id = entity.getAttribute("uid")
        if (p_num == '1'):
            player1_units.append(translate_units_id_to_template(e_id))
        else:
            player2_units.append(translate_units_id_to_template(e_id))
    units[1] = player1_units
    units[2] = player2_units
    return units
if __name__ == "__main__":
    main()