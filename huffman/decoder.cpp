#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <queue>
#include <iomanip>
using namespace std;

struct HuffmanNode {
    unsigned char data; //character 
    uint64_t frequency; 
    HuffmanNode* left; // 0 child node
    HuffmanNode* right; // 1 child node

    // constructor 1: holds a character and its count
    HuffmanNode(unsigned char byte, uint64_t freq)
        : data(byte), frequency(freq), left(nullptr), right(nullptr) {}

    // constructor 2: combines two children
    HuffmanNode(uint64_t freq, HuffmanNode* l, HuffmanNode* r)
        : data(0), frequency(freq), left(l), right(r) {}

    // check if node is a leaf
    bool isLeaf() const {
        return left == nullptr && right == nullptr;
    }
};

// define o comparador que a priority queue vai usar, por default ela nao tem como saber como ordenar meus HuffmanNodes
struct CompareNodes {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        return a->frequency > b->frequency;
    }
};

void deleteTree(HuffmanNode* node) {
    if (!node) {
        return;
    }

    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}

// usa as frequencias para reconstruir a arvore
HuffmanNode* buildTreeFromFrequencies(const std::unordered_map<unsigned char, uint64_t>& frequencies) {
    // minHeap
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNodes> minHeap;

    // cria node de folha para cada simbolo, cospe na heap
    for (const auto& [byteVal, count] : frequencies) {
        minHeap.push(new HuffmanNode(byteVal, count));
    }


    // combina os dois menores nos até ter uma arvore 
    while (minHeap.size() > 1) {
        HuffmanNode* left = minHeap.top();
        minHeap.pop();

        HuffmanNode* right = minHeap.top();
        minHeap.pop();

        // cria pai com frequencias combinadas e linka os filhos
        uint64_t combinedFreq = left->frequency + right->frequency;
        HuffmanNode* parent = new HuffmanNode(0, combinedFreq);
        parent->left = left;
        parent->right = right;

        minHeap.push(parent);
    }

    // o ultimo node é a raiz
    return minHeap.top();
}


// como é feito?
void decodeBitstream(std::ifstream& inFile, 
                     std::ofstream& outFile, 
                     HuffmanNode* root, 
                     uint64_t originalFileSize) {

    HuffmanNode* current = root;
    uint64_t bytesDecoded = 0;
    char ch;

    // le os bytes do arquivo
    while (inFile.get(ch) && bytesDecoded < originalFileSize) {
        auto byteVal = static_cast<unsigned char>(ch);

        // Extract bits from MSB (bit 7) down to LSB (bit 0)
        for (int i = 7; i >= 0; --i) {
            uint8_t bit = (byteVal >> i) & 1;

            // Traverse down the tree based on the bit
            if (bit == 0) {
                current = current->left;
            } else {
                current = current->right;
            }

            // Once we reach an external node (leaf), decode the character
            if (current->isLeaf()) {
                outFile.put(static_cast<char>(current->data));
                bytesDecoded++;

                // Return to the root to start decoding the next symbol
                current = root;

                // Stop immediately if all original bytes have been restored
                if (bytesDecoded == originalFileSize) {
                    break;
                }
            }
        }
    }
}

bool compareFiles(const std::string& originalPath, 
                  const std::string& compressedPath, 
                  const std::string& restoredPath) {
    
    // abre em modo binario os 3
    // cursor no fim (std::ios::ate) para obter os tamanhos
    std::ifstream fileOrig(originalPath, std::ios::binary | std::ios::ate);
    std::ifstream fileComp(compressedPath, std::ios::binary | std::ios::ate);
    std::ifstream fileRest(restoredPath, std::ios::binary | std::ios::ate);

    if (!fileOrig.is_open() || !fileComp.is_open() || !fileRest.is_open()) {
        std::cerr << "falha ao abrir arquivos.\n";
        return false;
    }

    // streamsize conta tamanho
    std::streamsize sizeOrig = fileOrig.tellg();
    std::streamsize sizeComp = fileComp.tellg();
    std::streamsize sizeRest = fileRest.tellg();

    std::cout << "\n--- Comparação: ---\n";
    std::cout << "Tamanho Original:   " << sizeOrig << " bytes\n";
    std::cout << "Tamanho Comprimido: " << sizeComp << " bytes\n";
    std::cout << "Tamanho Restaurado: " << sizeRest << " bytes\n";


    // seekg para loopar byte a byte
    fileOrig.seekg(0, std::ios::beg);
    fileRest.seekg(0, std::ios::beg);

    char byteOrig, byteRest;
    bool identical = true;
    uint64_t pos = 0;

    while (fileOrig.get(byteOrig) && fileRest.get(byteRest)) {
        if (byteOrig != byteRest) {
            std::cerr << "byte na posicao " << pos << " tem divergência \n";
            identical = false;
            break;
        }
        pos++;
    }

    if (!identical) {
        fileOrig.close();
        fileComp.close();
        fileRest.close();
        return false;
    }

    std::cout << "Compressão/decompressão realizads com sucesso! \n";

    // relatorio
    if (sizeOrig > 0 && sizeComp > 0) {
        // Taxa em bits por caractere (bits/char)
        double bitsPerChar = (static_cast<double>(sizeComp) * 8.0) / static_cast<double>(sizeOrig);

        double bitsPerCharOriginal = (static_cast<double>(sizeOrig) * 8.0) / static_cast<double>(sizeOrig);

        // Razao original / comprimido
        // double compressionRatio = static_cast<double>(sizeOrig) / static_cast<double>(sizeComp);

        // Porcentagem de reducao
        double reductionPercentage = (1.0 - (static_cast<double>(sizeComp) / static_cast<double>(sizeOrig))) * 100.0;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "---------------------------------------------------------\n";
        std::cout << "Taxa de bits (original):    " << bitsPerCharOriginal << " bits/caractere\n";
        std::cout << "Taxa de bits (comprimido):  " << bitsPerChar << " bits/caractere\n";
        //std::cout << "Razao (original/comprimido): " << compressionRatio << ":1\n";
        std::cout << "Reducao no armazenamento:   " << reductionPercentage << "%\n";
    }

    fileOrig.close();
    fileComp.close();
    fileRest.close();

    return true;
}
void freeTree(HuffmanNode* node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}

int main() {
    // read
    std::ifstream inFile("data/biblia.huf", std::ios::binary);

    // parse table from header
    uint64_t originalFileSize = 0;
    uint16_t uniqueSymbols = 0;
    std::unordered_map<unsigned char, uint64_t> frequencies;

    // le variaveis do arquivo
    inFile.read(reinterpret_cast<char*>(&originalFileSize), sizeof(originalFileSize));
    inFile.read(reinterpret_cast<char*>(&uniqueSymbols), sizeof(uniqueSymbols));

    // le (char, freq) e popula o map
    for (uint16_t i = 0; i < uniqueSymbols; ++i) {
            unsigned char byteVal = 0;
            uint64_t count = 0;

            // Read 1 byte for the character
            inFile.read(reinterpret_cast<char*>(&byteVal), sizeof(byteVal));

            // Read 8 bytes for its count
            inFile.read(reinterpret_cast<char*>(&count), sizeof(count));

            frequencies[byteVal] = count;
        }

    std::cout << "Tamnanho original: " << originalFileSize << " bytes\n";
    std::cout << "Contador de simbolos unicos: " << uniqueSymbols << "\n";

    // create tree (reconstruction)
    HuffmanNode* root = buildTreeFromFrequencies(frequencies);

    // decode (bitstream traversal)
    std::ofstream outFile("data/biblia_restored.txt", std::ios::binary);
    decodeBitstream(inFile, outFile, root, originalFileSize);


    std::cout << "Decodificado para: " << originalFileSize << " bytes.\n";

    freeTree(root);
    outFile.close();
    inFile.close();

    // compare
    compareFiles("data/biblia.txt", "data/biblia.huf", "data/biblia_restored.txt");
}