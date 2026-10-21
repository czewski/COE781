#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <cstdint>
#include <unordered_map>

using namespace std;

int main()
{
   // abre a biblia em "file"
   std::ifstream file("data/biblia.txt", std::ios::binary);

   // cria map e variavel que vai segurar cada char no loop
   unordered_map<char, uint64_t> frequencies;
   char ch;

   while (file.get(ch)) {
        // unsigned prevent negative indexing/keys
        unsigned char byte = static_cast<unsigned char>(ch);
        frequencies[byte]++;
   }

   // preciso ordenar pra criar o huffman

   // loop de print
   for (const auto& [byte, count] : frequencies) {
      std::cout << byte << ": " << count << '\n';
   }
   
   file.close();
}