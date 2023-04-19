//! not handling valid bit
#include <iostream>
#include <bits/stdc++.h>
#include <fstream>
using namespace std;

class Cache;
class Dram;

Cache *L1, *L2;
Dram *DRAM;
// int count_write_miss, count_read_miss ;

struct cacheBlock
{
public:
    unsigned long long lru_counter;
    unsigned long long valid_bit;
    unsigned long long tag;
    unsigned long long dirty_bit;
    cacheBlock(int tag, int lru)
    {
        this->tag = tag;
        this->lru_counter = lru;
        valid_bit = 1;
        dirty_bit = 0;
    }
};

// class Base
// {
// public:

// };

class Dram
{
public:
    unsigned long long reads;
    unsigned long long writes;
    Dram()
    {
        reads = 0;
        writes = 0;
    }

    void handleRead(unsigned long long address)
    {
        reads++;
    }

    void handleWrite(unsigned long long address)
    {
        writes++;
    }
};

class Cache
{
public:
    vector<vector<cacheBlock *>> cache;
    unsigned long long reads;
    unsigned long long read_misses;
    unsigned long long writes;
    unsigned long long write_misses;
    double miss_rate;
    unsigned long long writeBacks;
    unsigned long long CACHE_SIZE;
    unsigned long long ASSOCIATIVITY;
    unsigned long long BLOCKSIZE;
    unsigned long long sets;
    unsigned long long indexBits;
    unsigned long long tagBits;
    unsigned long long offsetBits;
    unsigned long long address;
    string name;

    //*Constructor
    Cache(int CACHE_SIZE, int ASSOCIATIVITY, int BLOCKSIZE, string n)
    {
        this->CACHE_SIZE = CACHE_SIZE;
        this->ASSOCIATIVITY = ASSOCIATIVITY;
        this->BLOCKSIZE = BLOCKSIZE;
        reads = 0;
        read_misses = 0;
        write_misses = 0;
        writes = 0;
        writeBacks = 0;
        sets = CACHE_SIZE / (ASSOCIATIVITY * BLOCKSIZE);
        cache.resize(sets, vector<cacheBlock *>(ASSOCIATIVITY, NULL));
        name = n;
    }

    //*functions
    void updateBits()
    {
        this->indexBits = (address / BLOCKSIZE) % sets;
        this->tagBits = address / (BLOCKSIZE * sets);
        this->offsetBits = address % BLOCKSIZE;
    }

    int LRU_findIndex()
    {
        int maxCount = -1;
        int index = 0;
        for (int i = 0; i < ASSOCIATIVITY; i++)
        {
            if (cache[indexBits][i] == NULL)
                return i;
            if (cache[indexBits][i]->lru_counter > maxCount)
            {
                maxCount = cache[indexBits][i]->lru_counter;
                index = i;
            }
        }
        return index;
    }

    void updateLRU(int indexBits, int accessIndex)
    {
        int current_ct = cache[indexBits][accessIndex]->lru_counter;
        // update LRU bits for all blocks in the set
        for (int i = 0; i < ASSOCIATIVITY; i++)
        {
            if (cache[indexBits][i] != NULL)
            {
                if (i == accessIndex)
                {
                    // accessed block should have LRU count of 0
                    cache[indexBits][i]->lru_counter = 0;
                }
                else
                {
                    if (cache[indexBits][i]->lru_counter < current_ct)
                        // increment LRU count for other blocks
                        cache[indexBits][i]->lru_counter++;
                }
            }
        }
    }

    void handleRead(unsigned long long address)
    {
        this->address = address;
        updateBits();
        for (int i = 0; i < ASSOCIATIVITY; i++)
        {
            if (cache[indexBits][i] != NULL && cache[indexBits][i]->tag == tagBits && cache[indexBits][i]->valid_bit == 1)
            {
                updateLRU(indexBits, i);
                reads++;
                return;
            }
        }
        handleReadMiss();
        return;
    }

    void handleReadMiss()
    {

        read_misses++;
        this->writes++; //! maybe remove
        int p = LRU_findIndex();
        // findNextCache();
        // nextCache->handleRead(address);
        if (name == "L1")
            L2->handleRead(address);
        else
            DRAM->handleRead(address);

        if (cache[indexBits][p] == NULL)
        {
            cache[indexBits][p] = new cacheBlock(tagBits, 0); // Here, 0 represents the LRU counter

            for (int j = 0; j < ASSOCIATIVITY; j++) // to increase lru_counter of all other blocks in the row(set)
            {
                if (cache[indexBits][j] != NULL && p != j)
                {
                    cache[indexBits][j]->lru_counter++;
                }
            }

            return;
        }

        // If all blocks in the set are occupied, find the LRU block and replace it
        int replaceIndex = LRU_findIndex();

        // Check if the block being replaced is dirty and needs to be written back to nextCache cache
        if (cache[indexBits][replaceIndex]->dirty_bit)
        {
            writeBacks++; //! maybe wrong..fix
            // Write back to nextCache cache
            // nextCache->handleWrite(address);
            if (name == "L1")
                L2->handleWrite(address);
            else
                DRAM->handleWrite(address);
        }

        // Replace the LRU block with the new block
        updateLRU(indexBits, replaceIndex);
        cache[indexBits][replaceIndex]->tag = tagBits;
        cache[indexBits][replaceIndex]->lru_counter = 0;
        cache[indexBits][replaceIndex]->dirty_bit = 0;
        cache[indexBits][replaceIndex]->valid_bit = 1;

        return;
    }

    bool handleWrite(unsigned long long address)
    {
        this->address = address;
        updateBits();

        // Check if the block is present in the cache
        for (int i = 0; i < ASSOCIATIVITY; i++)
        {
            if (cache[indexBits][i] != NULL && cache[indexBits][i]->tag == tagBits && cache[indexBits][i]->valid_bit)
            {
                // Write the data to the cache block
                writes++;
                // cache[indexBits][i]->data = getData(address);
                cache[indexBits][i]->dirty_bit = 1;

                // Update LRU bits
                updateLRU(indexBits, i);
                return true;
            }
        }

        // Write miss: call handleWriteMiss to fetch the block from L2 cache
        handleWriteMiss();

        return true;
    }

    void handleWriteMiss()
    {

        write_misses++;
        writes++; //! maybe remove
        writes++; //! for writing the final new value...since its write-allocate so no need to remove a write from L2

        // Fetch the block from L2 cache
        if (name == "L1")
            L2->handleRead(address);
        else
            DRAM->handleRead(address);

        // Write the data to the cache block
        int p = LRU_findIndex();
        if (cache[indexBits][p] == NULL)
        {
            cache[indexBits][p] = new cacheBlock(tagBits, 0); // Here, 0 represents the LRU counter

            for (int j = 0; j < ASSOCIATIVITY; j++) // to increase lru_counter of all other blocks in the row(set)
            {
                if (cache[indexBits][j] != NULL && p != j)
                {
                    cache[indexBits][j]->lru_counter++;
                }
            }
            return;
        }

        // If all blocks in the set are occupied, find the LRU block and replace it
        int replaceIndex = LRU_findIndex();

        // Check if the block being replaced is dirty and needs to be written back to nextCache cache
        if (cache[indexBits][replaceIndex]->dirty_bit)
        {
            writeBacks++; //! maybe wrong..fix
            // Write back to nextCache cache
            // nextCache->handleWrite(address);
            if (name == "L1")
                L2->handleWrite(address);
            else
                DRAM->handleWrite(address);
        }

        // Replace the LRU block with the new block
        updateLRU(indexBits, replaceIndex);
        cache[indexBits][replaceIndex]->tag = tagBits;
        cache[indexBits][replaceIndex]->lru_counter = 0;
        cache[indexBits][replaceIndex]->dirty_bit = 0;
        cache[indexBits][replaceIndex]->valid_bit = 1;
        return;
    }

    bool handleInstruction(string instruction, unsigned long long address)
    {
        this->address = address;
        if (instruction == "r")
        {
            handleRead(address);
        }
        else if (instruction == "w")
        {
            handleWrite(address);
        }
        else
        {
            cout << "Incorrect instruction. Terminating" << endl;
            exit(0);
        }
    }
};

void printSimulationResults()
{
    cout << "i. number of L1 reads: " << L1->reads << endl;
    cout << "ii. number of L1 read misses: " << L1->read_misses << endl;
    cout << "iii. number of L1 writes: " << L1->writes << endl;
    cout << "iv. number of L1 write misses: " << L1->write_misses << endl;
    cout << "v. L1 miss rate: " << fixed << setprecision(2) << (double)L1->read_misses / L1->reads * 100 << "%" << endl;
    cout << "vi. number of writebacks from L1 memory: " << L1->writeBacks << endl;
    cout << "vii. number of L2 reads: " << L2->reads << endl;
    cout << "viii. number of L2 read misses: " << L2->read_misses << endl;
    cout << "ix. number of L2 writes: " << L2->writes << endl;
    cout << "x. number of L2 write misses: " << L2->write_misses << endl;
    cout << "xi. L2 miss rate: " << fixed << setprecision(2) << (double)L2->read_misses / L2->reads * 100 << "%" << endl;
    cout << "xii. number of writebacks from L2 memory: " << L2->writeBacks << endl;
}

int main(int argc, char *argv[])
{

    if (argc != 7)
    {
        std::cerr << "Please provide all the arguments!\n";
        return 0;
    }
    ifstream file(argv[6]);

    int BLOCKSIZE = stoi(argv[1]);
    int L1_SIZE = stoi(argv[2]);
    int L1_ASSOC = stoi(argv[3]);
    int L2_SIZE = stoi(argv[4]);
    int L2_ASSOC = stoi(argv[5]);

    L1 = new Cache(L1_SIZE, L1_ASSOC, BLOCKSIZE, "L1");
    L2 = new Cache(L2_SIZE, L2_ASSOC, BLOCKSIZE, "L2");
    DRAM = new Dram();

    if (file.is_open())
    {
        string line;
        int ct = 0;
        while (getline(file, line))
        {
            stringstream ss(line);
            string instruction, hex;
            ss >> instruction >> hex;                                        // Tokenize the line
            unsigned long long address = strtoull(hex.c_str(), nullptr, 16); // convert hexadecimal to decimal

            L1->handleInstruction(instruction, address);
            // if (count_write_miss >= 2 || count_read_miss >= 2) //! forgot why I did this
            // {
            //     L2->reads--; //! assuming L1 write-miss and then L2 write-miss...so read just one time from DRAM
            // }

            // count_write_miss = 0;
            // count_read_miss = 0;
        }

        printSimulationResults();

        file.close();
    }
    else
    {
        std::cerr << "File could not be opened. Terminating...\n";
        return 0;
    }

    return 0;
}