#include <iostream>
#include <fstream>
#include "Fixed.hpp"
#include "FastFixed.hpp"
#include "Simulator.hpp"

template <typename type_p, typename type_v, typename type_vf, size_t Rows = mx_size, size_t Columns = mx_size>
class Simulator;

int main() {
    std::ifstream fin;
    fin.open("input.txt", std::ios_base::in);
    if (!fin) {
        std::cerr << "error during file opening\n";
        return 1;
    }

    int n, m;
    fin >> n >> m;
    float rho_air, g;
    int rho_fluid;
    std::vector field(n, std::vector<char>(m + 1));
    fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string s;
    for (int i = 0; i < n; ++i) {
        getline(fin, s);
        for (int j = 0; j < m + 1; ++j) {
            field[i][j] = s[j];
            if (j == m) {
                field[i][j] = '\0';
            }
        }
    }

    fin >> rho_air >> rho_fluid >> g;
    fin.close();

    Simulator<float, Fixed<32, 16>, FastFixed<32, 15>, 36, 84> fluid(field, rho_air, rho_fluid, g);
    fluid.Run();
}