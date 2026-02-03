/*
- key value store with dynamic size based on header
- implementing serializer logic to support multiple datatypes
*/
#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <cstdint>

template <typename T>
struct Serializer
{
    static uint32_t size(const T &obj)
    {
        return sizeof(T);
    }

    static void save(std::ofstream &file, const T &obj)
    {
        file.write(reinterpret_cast<const char *>(&obj), sizeof(T));
    }

    static T load(std::ifstream &file, const uint32_t &size)
    {
        T temp;
        file.read(reinterpret_cast<char *>(&temp), sizeof(T));
        return temp;
    }
};

template <>
struct Serializer<std::string>
{
    static uint32_t size(const std::string &obj)
    {
        return obj.size();
    }
    static void save(std::ofstream &file, const std::string &obj)
    {
        file.write(&obj[0], obj.size());
    }

    static std::string load(std::ifstream &file, const uint32_t &size)
    {
        std::string temp;
        temp.resize(size);
        file.read(&temp[0], size);
        return temp;
    }
};

struct RecordHeader
{
    uint32_t keySize;
    uint32_t valSize;
    bool isValid;
};

template <typename K, typename V>
class Database
{
private:
    std::map<K, long> indexMap;
    std::string filename;

    void rebuildIndex()
    {
        std::ifstream file(filename, std::ios::binary);
        RecordHeader rh;
        if (!file.is_open())
        {
            return;
        }

        long location = file.tellg();
        while (file.read(reinterpret_cast<char *>(&rh), sizeof(RecordHeader)))
        {
            K key = Serializer<K>::load(file, rh.keySize);

            if (rh.isValid)
            {
                indexMap[key] = location;
            }

            file.seekg(rh.valSize, std::ios::cur);
            location = file.tellg();
        }
    }

public:
    Database(std::string file) : filename(file)
    {
        rebuildIndex();
    }
    bool add(const K &key, const V &value)
    {
        RecordHeader rh;
        rh.keySize = Serializer<K>::size(key);
        rh.valSize = Serializer<V>::size(value);
        rh.isValid = true;
        std::ofstream file(filename, std::ios::binary | std::ios::app);
        if (!file.is_open())
        {
            return false;
        }
        file.seekp(0, std::ios::end);
        long location = file.tellp();
        indexMap[key] = location;
        file.write(reinterpret_cast<char *>(&rh), sizeof(RecordHeader));
        Serializer<K>::save(file, key);
        Serializer<V>::save(file, value);

        file.close();
        return true;
    }

    bool get(K &key, V &value)
    {
        if (indexMap.find(key) == indexMap.end())
        {
            std::cout << "Key not found!\n";
            return false;
        }
        else
        {
            long location = indexMap[key];
            RecordHeader rh;

            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open())
            {
                return false;
            }
            file.seekg(location);
            file.read(reinterpret_cast<char *>(&rh), sizeof(RecordHeader));

            file.seekg(rh.keySize, std::ios::cur);

            value = Serializer<V>::load(file, rh.valSize);

            file.close();
            return true;
        }
    }

    bool remove(const K &key)
    {
        if (indexMap.find(key) == indexMap.end())
        {
            std::cout << "No record found \n";
            return false;
        }

        std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);

        if (!file.is_open())
        {
            return false;
        }

        file.seekg(indexMap[key], std::ios::beg);
        RecordHeader rh;
        file.read(reinterpret_cast<char *>(&rh), sizeof(RecordHeader));
        rh.isValid = false;
        file.seekp(indexMap[key], std::ios::beg);
        file.write(reinterpret_cast<char *>(&rh), sizeof(RecordHeader));

        file.close();
        indexMap.erase(key);

        return true;
    }

    bool compact()
    {
        std::ifstream inFile(filename, std::ios::binary);
        std::ofstream outFile("temp.bin", std::ios::binary);
        if (!inFile.is_open() || !outFile.is_open())
        {
            return false;
        }
        int originalCount = 0;
        int newCount = 0;
        RecordHeader rh;
        while (inFile.read(reinterpret_cast<char *>(&rh), sizeof(RecordHeader)))
        {
            if (rh.isValid)
            {
                newCount++;
                uint32_t totalSize = rh.keySize + rh.valSize;
                std::vector<char> buffer(totalSize);

                outFile.write(reinterpret_cast<char *>(&rh), sizeof(RecordHeader));
                inFile.read(&buffer[0], totalSize);
                outFile.write(&buffer[0], totalSize);
            }
            else
            {
                inFile.seekg(rh.keySize + r.valSize, std::ios::cur);
            }
            originalCount++;
        }

        inFile.close();
        outFile.close();

        if (std::remove(filename.c_str()) != 0)
        {
            std::cout << "Failed to remove old file...\n";
            return false;
        }

        if (std::rename("temp.bin", filename.c_str()) != 0)
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
    Database<int, std::string> db("int_str_db.bin");

    std::cout << "Simple DB started. Commands: SET <id> <name> <salary>, GET <id>, EXIT\n";

    std::string command;

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
            int key;
            std::string value;
            std::cin >> key >> value;
            if (db.add(key, value))
            {
                std::cout << "Record added successfully.\n";
            }
            else
            {
                std::cout << "Failed to add the record. \n";
            }
        }
        else if (command == "GET")
        {
            int key;
            std::string value;
            std::cin >> key;
            if (db.get(key, value))
            {
                std::cout << "Key: " << key << " Value: " << value << "\n";
            }
            else
            {
                std::cout << "Failed to retrieve the record.\n";
            }
        }
    }
    return 0;
}