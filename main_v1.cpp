/*
Simple database
- read-write from console to txt file
*/

#include <iostream>
#include <map>
#include <string>
#include <fstream>

int main()
{
    std::map<std::string, std::string> database;
    std::string command, key, value;

    std::cout << "Simple DB started. Commands: SET <key> <value>, GET <key>, EXIT\n";

    std::ifstream inputFile("database.txt");
    if (inputFile.is_open())
    {
        std::string fileKey, fileValue;

        while (inputFile >> fileKey >> fileValue)
        {
            database[fileKey] = fileValue;
        }

        inputFile.close();
        std::cout << "Data loaded from disk. \n";
    }
    while (true)
    {
        std::cout << "> ";
        std::cin >> command;

        if (command == "EXIT")
        {
            break;
        }
        else if (command == "SET")
        {
            std::cin >> key >> value;
            database[key] = value;
            std::ofstream file("database.txt", std::ios::app);
            if (file.is_open())
            {
                file << key << " " << value << "\n";
                file.close();
                std::cout << "Saved to Disk \n";
            }
        }
        else if (command == "GET")
        {
            std::cin >> key;
            if (database.count(key))
            {
                std::cout << "Value: " << database[key] << "\n";
            }
            else
            {
                std::cout << "Key not found. \n";
            }
        }
    }

    return 0;
}