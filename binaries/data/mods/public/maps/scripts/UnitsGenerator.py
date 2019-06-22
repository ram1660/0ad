import random
import os
import xml.etree.ElementTree as ET
#"D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\templates\\units"
#"D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\maps\\scenarios\\TestML.xml"
map_file = "E:\\ram-and-michael\\binaries\\data\\mods\\public\\maps\\scenarios\\TestML.xml"
units_file = os.listdir("E:\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\templates\\units") # all the units in the file. 
armies_types = ("athen", "brit", "cart", "gaul", "iber", "kush", "mace", "maur", "pers", "ptol", "rome", "sele", "spart")
map_tree = ET.parse(map_file)
key_words = ['support','ship','siege','boat']

def generate_map():
    army1 = generate_units() # returns a set of units
    army2 = generate_units()
    flag = 10 # id 
    map_root = map_tree.getroot()
    Entites = map_tree.find("Entities")
    Entites = delete_Entitites(Entites)

    for unit in army1:
        Entity = ET.SubElement(Entites,"Entity", uid= str(flag))
        Template = ET.SubElement(Entity,"Template").text = "units/" + unit
        Player = ET.SubElement(Entity,"Player").text = "1" 
        Position = ET.SubElement(Entity,"Position",x="326.66935" ,z="321.1384")
        Orientation = ET.SubElement(Entity,"Orientation",y="2.35621")
        flag += 1
        
        
    for unit in army2:
        Entity = ET.SubElement(Entites,"Entity", uid= str(flag))
        Template = ET.SubElement(Entity,"Template").text = "units/" + unit
        Player = ET.SubElement(Entity,"Player").text = "2"
        Position = ET.SubElement(Entity,"Position",x="326.66935" ,z="321.1384")
        Orientation = ET.SubElement(Entity,"Orientation",y="2.35621")
        flag += 1 
    indent(map_root)
    
    map_tree.write(map_file,encoding = "utf-8",xml_declaration = True)
    fix_cdata()


def delete_Entitites(Entites):
   
    for child in list(Entites):
        Entites.remove(child)
    map_tree.write(map_file,encoding = "utf-8",xml_declaration = True)
    return Entites
    

def generate_units():
    size = random.randint(1,100)
    army_units = []
    army_to_pick = random.choice(armies_types)
    list_units = get_units()
    army_list = [faction for faction in list_units if army_to_pick in faction] # filtering by faction
    for i in range(size):
        army_units.append(random.choice(army_list)) 
    return army_units  


def indent(elem, level=4):
    i = "\n" + level*"  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            indent(elem, level+1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i
def fix_cdata():
    data = ""
    cdata_index_begin = 0
    cdata_index_end = 0
    with open(map_file, 'r', newline="\n") as f:
        data = str(f.read())
        cdata_index_begin = data.find("{")
        cdata_index_end = data.find("}")
    with open(map_file, 'w', newline='\n') as f:
        data = data.replace("{", "<![CDATA[{", 1)
        data = data.replace("</ScriptSettings>", "]]></ScriptSettings>")
        f.writelines(data)

def get_units():
    army_list_units = [x for x in units_file if not any(word in x for word in key_words)] # getting only war units 
    army_list_units = [i.split('.')[0] for i in army_list_units] # for Process_manager
    return army_list_units
army_list = get_units() # units that can spawn in a game
generate_map()

