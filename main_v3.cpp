/*
simple database
- storing information in binary instead of txt and using struct
- adding soft delete feature
- refactor
- clean up : soft delete -> hard delete (i.e. reclaim space)
*/

#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <iomanip> // for std:setw safety to input doesnt exceed name buffer size

struct Employee
{
    int id;
    char name[50]; // fixed buffer. strings are apparently harder?
    double salary;
    bool isValid;
};

class Database
{
private:
    std::map<int, long> indexMap; // maps emp.id to location of details in data.bin
    std::string fileName;
    void rebuildIndex()
    {
        std::ifstream file(fileName, std::ios::binary);
        if (!file.is_open())
            return;

        Employee tempEmp;
        long location = 0;

        std::cout << "Indexing the file ...\n";

        while (file.read(reinterpret_cast<char *>(&tempEmp), sizeof(Employee)))
        {
            if (tempEmp.isValid)
            {
                indexMap[tempEmp.id] = location;
            }
            location = file.tellg();
        }
        file.close();

        std::cout << "Done! Indexed: " << indexMap.size() << " records.\n";
    }

public:
    Database(std::string fname) : fileName(fname)
    {
        rebuildIndex();
    }

    bool add(int id, std::string name, double salary)
    {
        if (indexMap.count(id))
            return false;
        Employee emp;
        emp.id = id;
        std::strncpy(emp.name, name.c_str(), 49);
        emp.name[49] = '\0';
        emp.salary = salary;
        emp.isValid = true;
        // open file in binary + append mode
        std::ofstream file(fileName, std::ios::binary | std::ios::app);

        if (file.is_open())
        {
            file.seekp(0, std::ios::end);
            indexMap[emp.id] = file.tellp();

            file.write(reinterpret_cast<char *>(&emp), sizeof(Employee));
            file.close();

            return true;
        }
        else
        {
            return false;
        }
    }
    bool get(int id, Employee &emp)
    {
        std::ifstream file(fileName, std::ios::binary);
        if (indexMap.find(id) == indexMap.end())
        {
            return false;
        }
        else
        {
            if (file.is_open())
            {
                // random access jump to the index
                file.seekg(indexMap[id], std::ios::beg);

                if (file.read(reinterpret_cast<char *>(&emp), sizeof(Employee)) && emp.isValid)
                {
                    file.close();
                    return true;
                }
                else
                {
                    file.close();
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }
    bool remove(int id)
    {
        Employee emp;
        if (indexMap.find(id) == indexMap.end())
        {
            return false;
        }
        else
        {
            long location = indexMap[id];
            std::fstream file(fileName, std::ios::binary | std::ios::in | std::ios::out);

            if (file.is_open())
            {
                file.seekg(location, std::ios::beg);
                file.read(reinterpret_cast<char *>(&emp), sizeof(Employee));

                emp.isValid = false;

                file.seekp(location, std::ios::beg);
                file.write(reinterpret_cast<char *>(&emp), sizeof(Employee));
                file.close();

                indexMap.erase(id);
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    bool compact()
    {
        std::cout << "Compacting file ...\n";

        std::ifstream inFile(fileName, std::ios::binary);
        std::ofstream outFile("temp.bin", std::ios::binary);

        if (!inFile.is_open() || !outFile.is_open())
            return false;
        int originalCount = 0;
        int newCount = 0;
        Employee tempEmp;

        while (inFile.read(reinterpret_cast<char *>(&tempEmp), sizeof(Employee)))
        {
            if (tempEmp.isValid)
            {
                outFile.write(reinterpret_cast<char *>(&tempEmp), sizeof(Employee));
                newCount++;
            }
            originalCount++;
        }

        inFile.close();
        outFile.close();

        if (std::remove(fileName.c_str()) != 0)
        {
            std::cout << "Failed to remove old file...\n";
            return false;
        }

        if (std::rename("temp.bin", fileName.c_str()) != 0)
        {
            std::cout << "Failed to rename new file...\n";
            return false;
        }

        std::cout << "Rebuilding index for new file layout...\n";
        indexMap.clear();
        rebuildIndex();

        std::cout << "Removed: " << originalCount - newCount << " zombine records. Done!\n";
        return true;
    }
};

int main()
{
    Database db("data.bin");

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
            int id;
            std::string name;
            double salary;
            std::cin >> id >> name >> salary;

            db.add(id, name, salary);
        }
        else if (command == "GET")
        {
            int searchId;
            std::cin >> searchId;

            Employee emp;
            if (db.get(searchId, emp))
            {
                std::cout << "ID: " << emp.id << ", Name: " << emp.name << ", Salary: " << emp.salary << "\n";
            }
            else
            {
                std::cout << "Record not found (index out of bounds)\n";
            }
        }
        else if (command == "DELETE")
        {
            int deleteId;
            std::cin >> deleteId;

            if (db.remove(deleteId))
            {
                std::cout << "Record with Id: " << deleteId << " is deleted.\n";
            }
            else
            {

                std::cout << "Id not found.\n";
            }
        }
        else if (command == "COMPACT")
        {
            if (!db.compact())
            {
                std::cout << "Failed to compacted ...\n";
            }
        }
    }
    return 0;
}