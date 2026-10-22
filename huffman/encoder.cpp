#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <queue>
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


// loop na arvore e salva os codigos
void generateCodes(const HuffmanNode* node, 
                   const std::string& currentCode, 
                   std::unordered_map<unsigned char, std::string>& codeTable) {
    if (!node) {
        return;
    }

    // se é uma folha, temos um codigo
    if (node->isLeaf()) {
        // If the tree has depth 0, currentCode is empty "", so assign at least "0"
        codeTable[node->data] = currentCode.empty() ? "0" : currentCode;
        return;
    }

    // Recursive Step:
    // Left branch adds '0', right branch adds '1'
    generateCodes(node->left, currentCode + "0", codeTable);
    generateCodes(node->right, currentCode + "1", codeTable);
}

void writeHeader(std::ofstream& outFile, 
                 uint64_t originalFileSize, 
                 const std::unordered_map<unsigned char, uint64_t>& frequencies) {
    
    // 1. Write the original uncompressed file size (8 bytes)
    // The decoder will use this count to know when to stop decoding.
    outFile.write(reinterpret_cast<const char*>(&originalFileSize), sizeof(originalFileSize));

    // 2. Count non-zero unique symbols and write as uint16_t (2 bytes)
    // uint16_t accommodates up to 256 unique symbols without overflowing.
    uint16_t uniqueSymbols = 0;
    for (const auto& [byteVal, count] : frequencies) {
        if (count > 0) {
            uniqueSymbols++;
        }
    }
    outFile.write(reinterpret_cast<const char*>(&uniqueSymbols), sizeof(uniqueSymbols));

    // 3. Write each symbol (1 byte) and its frequency (8 bytes)
    for (const auto& [byteVal, count] : frequencies) {
        if (count > 0) {
            // Write the character byte
            outFile.write(reinterpret_cast<const char*>(&byteVal), sizeof(byteVal));
            
            // Write its frequency count
            outFile.write(reinterpret_cast<const char*>(&count), sizeof(count));
        }
    }
}

#include <fstream>
#include <string>
#include <cstdint>

class BitWriter {
private:
    std::ofstream& out;
    uint8_t buffer;
    int bitCount;

public:
    explicit BitWriter(std::ofstream& outputStream) 
        : out(outputStream), buffer(0), bitCount(0) {}

    // Write a single bit (0 or 1)
    void writeBit(int bit) {
        buffer = (buffer << 1) | (bit & 1);
        bitCount++;

        // Once 8 bits accumulate, flush to file
        if (bitCount == 8) {
            out.put(static_cast<char>(buffer));
            buffer = 0;
            bitCount = 0;
        }
    }

    // Write an entire bitstring (e.g. "0110")
    void writeBits(const std::string& bitString) {
        for (char ch : bitString) {
            writeBit(ch == '1' ? 1 : 0);
        }
    }

    // Flush any leftover bits at the end of the file
    void flush() {
        if (bitCount > 0) {
            // Shift remaining bits to the most significant positions
            buffer <<= (8 - bitCount);
            out.put(static_cast<char>(buffer));
            buffer = 0;
            bitCount = 0;
        }
    }

    // Destructor ensures flush is always performed if user forgets
    ~BitWriter() {
        flush();
    }
};

void encodePayload(std::ifstream& inFile, 
                   std::ofstream& outFile, 
                   const std::unordered_map<unsigned char, std::string>& huffmanCodes) {

    // 1. Rewind the input file back to the very beginning
    inFile.clear();                 // Clear any EOF flags from previous reads
    inFile.seekg(0, std::ios::beg); // Seek back to byte 0

    // 2. Initialize the BitWriter attached to our binary output file
    BitWriter writer(outFile);

    // 3. Read input byte-by-byte and push codewords
    char ch;
    while (inFile.get(ch)) {
        unsigned char byteVal = static_cast<unsigned char>(ch);
        
        // Fast table lookup for the Huffman code
        const std::string& code = huffmanCodes.at(byteVal);
        
        // Feed the bits into the packing buffer
        writer.writeBits(code);
    }

    // 4. Flush any trailing partial byte to disk
    writer.flush();
}

int main()
{
   // abre a biblia em "file"
   std::ifstream file("data/biblia.txt", std::ios::binary);

   // cria map e variavel que vai segurar cada char no loop
   unordered_map<char, uint64_t> frequencies;
   char ch;

   uint64_t originalFileSize = 0;
   while (file.get(ch)) {
        // unsigned prevent negative indexing/keys
        unsigned char byte = static_cast<unsigned char>(ch);
        frequencies[byte]++;
        originalFileSize++;
   }

   file.close();

   // preciso ordenar pra criar o huffman, codigos curtos pra alta frequencia
   // na verdade posso skippar a ordenação, se eu adicionar em uma priority queue
   // "most efficient way to repeatedly extract the symbols with the smallest probabilities or counts"
   // priority_queue: o menor item sempre está no topo. 
   std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNodes> minHeap;

   // loopa pelas frequencias (unordered_map)
   for (const auto& [byte, count] : frequencies) {
      if (count > 0) { // only push if exist count
         // create leaf node
         HuffmanNode* leaf = new HuffmanNode(byte, count);

         // add to the minHeap
         minHeap.push(leaf);
      }
   }

   // print frequencies
   // for (const auto& [byte, count] : frequencies) {
   //    std::cout << byte << ": " << count << '\n';
   // }

   // tree building (N - 1 vezes)
   // enquanto a minHeap tem mais de um elemento, pop() left, pop() right
      // cria node pai, com frequencia (left>freq + right>freq)
      // adiciona left e right no node
      // adiciona o node na minHeap
   while (minHeap.size() > 1) {
      // extrai primeiro minimo
      HuffmanNode* left = minHeap.top();
      minHeap.pop();

      // extrai segundo 
      HuffmanNode* right = minHeap.top();
      minHeap.pop();

      // cria node Pai
      uint64_t count = left->frequency + right->frequency;
      HuffmanNode* father = new HuffmanNode(count, left, right);

      // insere pai na minHeap
      minHeap.push(father);
   }

   // get codes from tree to map
   // traversal pra esquerda < 0 
   // direita < 1
   // quando chega na folha, é o codigo huffman
   std::unordered_map<unsigned char, std::string> huffmanCodes;
   HuffmanNode* root = minHeap.top();

   // chama a funcao auxiliar de gerar a tabela de codigos
   if (root != nullptr) {
      generateCodes(root, "", huffmanCodes);
   }

   // print codigos
   // for (const auto& [byte, code] : huffmanCodes) {
   //    std::cout << "Byte " << static_cast<int>(byte) 
   //             << " -> Code: " << code << '\n';
   // }

   string outputPath = "";
   
   if (originalFileSize == 0) {
        std::ofstream outFile(outputPath, std::ios::binary);
        writeHeader(outFile, 0, frequencies);
        return;
    }

  
   // 5. Open output file in binary mode
   std::ofstream outFile(outputPath, std::ios::binary);

   // criar header 
   writeHeader(outFile, originalFileSize, frequencies);

   // encode
   encodePayload(inFile, outFile, huffmanCodes);

   // save file
   // 8. Clean up allocated tree nodes
   deleteTree(root);


}