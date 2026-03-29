#include "TestGenerators.hpp"

std::vector<std::byte> generate_sequence(const size_t size) {
   assert(size < UINT8_MAX);

   std::vector<std::byte> result(size);

   for(size_t i = 0; i < size; i++) {
       result[i] = (std::byte)(i + 1);
   }

   return result;
}
