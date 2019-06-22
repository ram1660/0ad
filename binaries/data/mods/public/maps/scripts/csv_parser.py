import csv, os
temp = 1
def main():
    global temp
    with open("E:\\ram-and-michael\\binaries\\data\\mods\\public\\maps\\scripts\\full.csv") as csv_file:
        csv_reader = csv.DictReader(csv_file)
        for row in csv_reader:
            print(str(temp))
            temp = temp + 1
            for col in row:
                if("ea1" in col):
                    if(row[col] > '0'):
                        append_winner('0', csv_reader, row)
                        break
                elif("ea2" in col):
                    if(row[col] > '0'):
                        append_winner('1', csv_reader, row)
                        break
        
def append_winner(player_winner, csv_reader, row):
    global temp
    new_csv_file = {}
    headers = csv_reader.fieldnames
    #headers.append('playerOne')
    #headers.append('playerTwo')
    new_csv_file = row
    if(player_winner == '0'):
        new_csv_file['result'] = 0
    else:
        new_csv_file['result'] = 1
    if(os.path.exists("E:\\ram-and-michael\\binaries\\data\\mods\\public\\maps\\scripts\\newFull.csv") == False):
        with open("E:\\ram-and-michael\\binaries\\data\\mods\\public\\maps\\scripts\\newFull.csv", mode='w', newline='') as file:
            headers.append('result')
            data_writer = csv.DictWriter(file, fieldnames=headers)
            data_writer.writeheader()
            data_writer.writerow(new_csv_file)
    else:
        with open("E:\\ram-and-michael\\binaries\\data\\mods\\public\\maps\\scripts\\newFull.csv", mode='a', newline='') as file:
            data_writer = csv.DictWriter(file, fieldnames=headers)
            data_writer.writerow(new_csv_file)

if __name__ == "__main__":
    main()