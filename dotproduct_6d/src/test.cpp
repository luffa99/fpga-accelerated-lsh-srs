// Your First C++ Program

#include <iostream>

void load_chunk ( float * array_of_size_16) {

    for(int j=0; j<16; j++) {
        float t = 0.3;
        array_of_size_16[j] = *((float*)&t);
    }

}

int main() {
    std::cout << "Hello World!";
    float temp_orig[16];
    load_chunk(temp_orig);

    for(int j=0;j<16;j++){
        std::cout << (temp_orig[j]) << std::endl;
    }
    return 0;
}
