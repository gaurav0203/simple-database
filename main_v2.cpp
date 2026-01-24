/*
simple database
- storing information in binary instead of txt and using struct
*/

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <iomanip> // for std:setw safety to input doesnt exceed name buffer size

struct Employee
{
    int id;
    char name[50]; // fixed buffer. strings are apparently harder?
    double salary;
};

std::map<int, long> indexMap; // maps emp.id to location of details in data.bin

void buildIndex()
{
    std::ifstream file("data.bin", std::ios::binary);
    if (!file.is_open())
        return;

    Employee tempEmp;
    long location = 0;

    std::cout << "Indexing the file ...\n";

    while (file.read(reinterpret_cast<char *>(&tempEmp), sizeof(Employee)))
    {
        indexMap[tempEmp.id] = location;
        location = file.tellg();
    }
    file.close();

    std::cout << "Done! Indexed: " << indexMap.size() << " records.\n";
}

int main()
{
    buildIndex();
    Employee emp;
    std::string command;
    std::cout << "Simple DB started. Commands: SET <id> <name> <salary>, GET <id>, EXIT\n";

    while (true)
    {
        std::cout << "> ";
        std::cin >> command;

        if (command == "EXIT")
            break;

        if (command == "SET")
        {
            std::cin >> emp.id >> std::setw(50) >> emp.name >> emp.salary;

            // open file in binary + append mode
            std::ofstream file("data.bin", std::ios::binary | std::ios::app);

            if (file.is_open())
            {
                indexMap[emp.id] = file.tellp();

                file.write(reinterpret_cast<char *>(&emp), sizeof(Employee));
                file.close();

                std::cout << "Saved " << sizeof(Employee) << " bytes to the disk \n";
            }
        }
        else if (command == "GET")
        {
            int searchId;
            std::cin >> searchId;

            std::ifstream file("data.bin", std::ios::binary);
            if (indexMap.find(searchId) == indexMap.end())
            {
                std::cout << "ID not found. \n";
            }
            else
            {
                if (file.is_open())
                {
                    // random access jump to the index
                    file.seekg(indexMap[searchId], std::ios::beg);

                    if (file.read(reinterpret_cast<char *>(&emp), sizeof(Employee)))
                    {
                        std::cout << "ID: " << emp.id << ", Name: " << emp.name << ", Salary: " << emp.salary << "\n";
                    }
                    else
                    {
                        std::cout << "Record not found (index out of bounds)\n";
                    }
                    file.close();
                }
            }
        }
    }
}